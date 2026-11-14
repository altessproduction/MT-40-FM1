/* SPDX-License-Identifier: GPL-3.0-only
 * MT-40X web emulator: the shipped check. node wasm_smoke_test.js dist/mt40x.wasm
 * Boots the real firmware, walks its own UI state (emu_dbg) and the MT-40X state,
 * renders audio and ends with AUDIO-PATH-OK.
 * Input encoding, as the page's: emu_input(notes, buttons) — bit n of notes = note
 * key n (0..26, F3..G5), bit i of buttons = matrix button i (panel.btn: PLAY = 12,
 * HOME = 8, OCT- = 0, OCT+ = 1). A key down while MT-40X is armed SYNCHRO-STARTs
 * the transport, as on the MT-40 — the tests order themselves around it. */
'use strict';
const fs = require('fs');
const path = require('path');

const file = process.argv[2] || path.join(__dirname, 'dist', 'mt40x.wasm');
const mod = new WebAssembly.Module(fs.readFileSync(file));
const inst = new WebAssembly.Instance(mod, {});
const e = inst.exports;
let fails = 0;

function check(what, ok) {
    console.log(`smoke: ${what.padEnd(74)} ${ok ? 'ok' : 'FAIL'}`);
    if (!ok) fails++;
}

/* the exports the page needs */
const NEED = ['memory', 'emu_init', 'emu_start', 'emu_frame', 'emu_render', 'emu_input',
              'emu_enc', 'emu_adc_set', 'emu_midi', 'emu_fb_ptr', 'emu_fb_gen',
              'emu_audio_ptr', 'emu_led_ptr', 'emu_led_dim_ptr', 'emu_flash_ptr',
              'emu_flash_size', 'emu_flash_gen', 'emu_mt40_count', 'emu_mt40_name',
              'emu_mt40_bpm', 'emu_mt40_load', 'emu_mt40_set', 'emu_mt40_state',
              'emu_seq_len', 'emu_seq_note', 'emu_seq_pos', 'emu_seq_playing', 'emu_dbg'];
for (const n of NEED)
    check(`export ${n}`, typeof e[n] === 'function' || n === 'memory' && e.memory instanceof WebAssembly.Memory);

/* boot: the splash draws, then the home screen */
e.emu_init();
check('boot: the splash drew pixels', e.emu_fb_gen() > 0);
const fb = () => new Uint16Array(e.memory.buffer, e.emu_fb_ptr(), 240 * 240);
let nonwhite = 0;
{ const v = fb(); for (let i = 0; i < v.length; i++) if (v[i] !== 0xFFFF) nonwhite++; }
check('boot: the splash is not blank', nonwhite > 10000);

e.emu_start();
/* one UI frame every ~17 ms: 6 audio blocks of 128, then emu_frame */
function frames(n) {
    for (let f = 0; f < n; f++) {
        for (let b = 0; b < 6; b++) e.emu_render(128);
        e.emu_frame();
    }
}
frames(30);                                    /* ~0.5 s of home screen */
check('ui: the home screen drew', e.emu_fb_gen() > 0);

/* the riddim bank: the SLOOP Dub 1.6 bank, 28 riddims, names readable */
const N = e.emu_mt40_count();
check('mt40: the bank has 28 riddims', N === 28);
{
    const u8 = () => new Uint8Array(e.memory.buffer);
    const name = (i) => { const p = e.emu_mt40_name(i); let s = ''; const m = u8();
                          for (let k = p; m[k]; k++) s += String.fromCharCode(m[k]); return s; };
    const n0 = name(0);
    check(`mt40: riddim 0 has a name ("${n0}")`, n0.length > 2 && n0.length < 24);
    let bpm_ok = true;
    for (let i = 0; i < N; i++) { const b = e.emu_mt40_bpm(i); if (b < 60 || b > 200) bpm_ok = false; }
    check('mt40: every riddim has a tempo 60..200', bpm_ok);
}

/* the MT-40X arm + a riddim load, the RIDDIM page's own code path */
e.emu_mt40_set(1);
frames(4);
check('mt40: armed (the AUDIO > MT-40X row)', e.emu_dbg(7) === 1);
e.emu_mt40_load(0);
frames(4);
check('mt40: riddim 0 loaded', e.emu_dbg(6) === 1);
check('mt40: the load put its tempo in', e.emu_dbg(2) === e.emu_mt40_bpm(0));
let drums = 0, bass = 0, skank = 0, mel = 0;
for (let s = 0; s < 64; s++) {
    if (e.emu_seq_note(3, s) !== 255) drums++;
    if (e.emu_seq_note(2, s) !== 255 && e.emu_seq_note(2, s) !== 254) bass++;
    if (e.emu_seq_note(1, s) !== 255 && e.emu_seq_note(1, s) !== 254) skank++;
    if (e.emu_seq_note(0, s) !== 255 && e.emu_seq_note(0, s) !== 254) mel++;
}
check(`mt40: the four tracks loaded (drums ${drums}, bass ${bass}, skank ${skank}, melody ${mel})`,
      drums > 4 && bass > 4 && skank > 2 && mel > 2);

/* transport: PLAY from stop, then the audio path (before any key: no synchro start yet) */
e.emu_input(0, 1 << 12);                       /* PLAY (panel.btn[B_PLAY] = 12) */
frames(3);
e.emu_input(0, 0);
frames(3);
check('seq: PLAY starts the transport', e.emu_dbg(0) === 1);
const pos0 = e.emu_dbg(11);
frames(20);
check('seq: the playhead moves', e.emu_dbg(11) !== pos0);

/* audio: real render, finite and loud enough */
let peak = 0, finite = true;
for (let f = 0; f < 60; f++) {                 /* ~1 s of the riddim */
    for (let b = 0; b < 6; b++) {
        e.emu_render(128);
        const a = new Float32Array(e.memory.buffer, e.emu_audio_ptr(), 256);
        for (let i = 0; i < a.length; i++) {
            const v = a[i];
            if (!Number.isFinite(v)) finite = false;
            const m = Math.abs(v);
            if (m > peak) peak = m;
        }
        e.emu_frame();
    }
}
check('seq: the audio path renders (peak ' + peak.toFixed(3) + ')', peak > 0.001);
check('seq: the render is finite', finite);
check('seq: the transport still plays', e.emu_dbg(0) === 1);

/* the bass keyboard, the transport running: a key re-roots, the next key is its MIN,
 * two keys on (the rule is against the last press, mt40.c) its DOM7 */
const KEY = 16;
e.emu_input(1 << KEY, 0);
frames(2);
check('mt40: a key re-roots the riddim', e.emu_dbg(8) > 0);
check('mt40: the key itself is MAJ', e.emu_dbg(9) === 0);
e.emu_input(0, 0);
frames(2);
e.emu_input(1 << (KEY + 1), 0);
frames(2);
check('mt40: the next key is MIN', e.emu_dbg(9) === 1);
e.emu_input(0, 0);
frames(2);
e.emu_input(1 << (KEY + 3), 0);                /* the last press + 2: its DOM7 */
frames(2);
check('mt40: two keys on is DOM7', e.emu_dbg(9) === 2);
e.emu_input(0, 0);
frames(2);
check('seq: it still plays under the chords', e.emu_dbg(0) === 1);

/* a second riddim: the bank switches, the tempo follows */
const b1 = e.emu_mt40_bpm(1);
e.emu_mt40_load(1);
frames(4);
check('mt40: riddim 1 loads over 0', e.emu_dbg(6) === 2 && e.emu_dbg(2) === b1);

/* the screen keeps living (the menu redraws the framebuffer) */
const g0 = e.emu_fb_gen();
e.emu_input(0, 1 << 8);                        /* HOME held opens the menu */
frames(3);
e.emu_input(0, 0);
frames(3);
check('ui: the framebuffer keeps refreshing', e.emu_fb_gen() !== g0);

console.log(fails ? `smoke: ${fails} FAILED` : 'AUDIO-PATH-OK');
process.exit(fails ? 1 : 0);
