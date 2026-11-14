/* SPDX-License-Identifier: GPL-3.0-only
 * MT-40X web emulator: the AudioWorklet hosts the whole firmware (wasm).
 * process() renders audio with mix_block; every few blocks a UI frame runs
 * and the framebuffer + LED + MT-40X state are posted to the main thread.
 * (The layout follows altessproduction's SLOOP Dub reference_emu/worklet.js,
 * GPL-3.0, with the MT-40X state and the four tracks' readback in its place.) */
class Mt40xProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
        const mod = new WebAssembly.Module(options.processorOptions.wasmBytes);
        this.e = new WebAssembly.Instance(mod, {}).exports;
        const img = options.processorOptions.flashImage;
        if (img && img.byteLength) {        // restore the saved flash before boot
            const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size());
            fl.set(new Uint8Array(img, 0, Math.min(img.byteLength, fl.length)));
        }
        this.lastFlashGen = 0;
        this.e.emu_init();
        this.lastFlashGen = this.e.emu_flash_gen();   // boot-time writes need no save
        this.fbPtr = this.e.emu_fb_ptr();
        this.audioPtr = this.e.emu_audio_ptr();
        this.ledPtr = this.e.emu_led_ptr();
        this.ledDimPtr = this.e.emu_led_dim_ptr();
        this.lastGen = -1;
        this.blocks = 0;
        this.running = true;
        this.port.onmessage = (ev) => {
            const m = ev.data;
            if (m.t === 'input') this.e.emu_input(m.notes, m.buttons);
            else if (m.t === 'enc') this.e.emu_enc(m.e, m.steps);
            else if (m.t === 'start') this.e.emu_start();
            else if (m.t === 'adc') this.e.emu_adc_set(m.ch, m.v);
            else if (m.t === 'midi') this.e.emu_midi(m.p);
            else if (m.t === 'mt40-set') this.e.emu_mt40_set(m.on ? 1 : 0);   // the AUDIO > MT-40X row
            else if (m.t === 'mt40-load') this.e.emu_mt40_load(m.rid | 0);    // the RIDDIM row's load
            else if (m.t === 'names') this.postNames();                       // the bank, for the page's chips
            else if (m.t === 'dbg') {          // the tests' state probe: emu_dbg(idx), the firmware's own state
                const v = typeof this.e.emu_dbg === 'function' ? this.e.emu_dbg(m.idx | 0) : null;
                this.port.postMessage({ t: 'dbg', idx: m.idx, id: m.id, v });
            }
            else if (m.t === 'stop') this.running = false;
        };
        this.postFrame();           // the splash, before audio starts
    }

    seqDump() {                     // the four tracks' steps for the pattern view
        const e = this.e;
        const notes = new Uint8Array(4 * 64);
        const lens = new Uint8Array(4);
        for (let tr = 0; tr < 4; tr++) {
            lens[tr] = e.emu_seq_track_len(tr);
            for (let st = 0; st < 64; st++)
                notes[tr * 64 + st] = e.emu_seq_note(tr, st);
        }
        return { lens, notes, pos: e.emu_seq_pos(), playing: !!e.emu_seq_playing() };
    }

    postNames() {                   // the riddim bank, the wasm's own strings
        const e = this.e;
        if (typeof e.emu_mt40_count !== 'function') { this.port.postMessage({ t: 'names', names: [], bpms: [] }); return; }
        const m = new Uint8Array(e.memory.buffer);
        const names = [], bpms = [];
        for (let i = 0; i < e.emu_mt40_count(); i++) {
            let s = '';
            for (let p = e.emu_mt40_name(i); m[p]; p++) s += String.fromCharCode(m[p]);
            names.push(s);
            bpms.push(e.emu_mt40_bpm(i));
        }
        this.port.postMessage({ t: 'names', names, bpms });
    }

    postFrame() {
        const e = this.e;
        const gen = e.emu_fb_gen();
        const leds = new Uint8Array(e.memory.buffer, this.ledPtr, 11).slice();
        const dims = new Uint8Array(e.memory.buffer, this.ledDimPtr, 11).slice();
        const mt = typeof e.emu_mt40_state === 'function' ? e.emu_mt40_state() : 0;
        const seq = this.seqDump();
        if (gen !== this.lastGen) {
            this.lastGen = gen;
            const fb = new Uint16Array(e.memory.buffer, this.fbPtr, 240 * 240).slice();
            this.port.postMessage({ t: 'frame', fb, leds, dims, mt, seq }, [fb.buffer]);
        } else {
            this.port.postMessage({ t: 'leds', leds, dims, mt, seq });
        }
    }

    process(inputs, outputs) {
        const out = outputs[0];
        const n = out[0].length;             // 128
        this.e.emu_render(n);
        const a = new Float32Array(this.e.memory.buffer, this.audioPtr, 2 * n);
        const L = out[0], R = out[1] || out[0];
        for (let i = 0; i < n; i++) {
            L[i] = a[2 * i];
            R[i] = a[2 * i + 1];
        }
        if (++this.blocks % 6 === 0) {       // a UI frame every ~17 ms
            this.e.emu_frame();
            this.postFrame();
        }
        if (this.blocks % 256 === 0) {       // ~0.75 s: ship changed flash to be saved
            const gen = this.e.emu_flash_gen();
            if (gen !== this.lastFlashGen) {
                this.lastFlashGen = gen;
                const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size()).slice();
                this.port.postMessage({ t: 'flash', data: fl.buffer }, [fl.buffer]);
            }
        }
        return this.running;
    }
}
registerProcessor('mt40x', Mt40xProcessor);
