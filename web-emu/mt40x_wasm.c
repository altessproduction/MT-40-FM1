/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 MT-40X contributors (Felucca by Leo Kuroshita / Hügelton Instruments,
 * the web HAL model after altessproduction's SLOOP Dub reference_emu, both GPL-3.0-only) */
/* MT-40X web emulator: the real MT-40X firmware sources (Felucca 1.8.2 + the MT-40X fork)
 * compiled to wasm32, with a browser HAL in place of the FM-1 hardware — the same layout
 * as tests/hostsim.c for the DSP alone, but with the whole UI: screen, layers, LEDs.
 *
 * The firmware is one compilation unit (firmware/src/felucca.c); this file plays that role
 * for the web: the HAL below replaces the fm1_*.h headers, then the same sources are
 * included in the same order. No firmware source is modified.
 *
 *   audio   emu_render(n): mix_block into a float buffer (an AudioWorklet)
 *   screen  an ST7789 panel model behind lcd.c: 240x240 RGB565 framebuffer
 *   input   emu_input(notes, buttons) / emu_enc(e, steps) from JS events
 *   boot    emu_init (splash) then emu_start, emu_frame() ~60/s (ui loop)
 *   mt40    emu_mt40_* : the MT-40X riddim bank, its state, the four tracks' readback
 */
#include <stdint.h>

#define WASM_EXPORT(n) __attribute__((export_name(n), used))

/* ------------------------------------------------------------- time HAL --- */
#define FM1_TICKS_PER_US 24u
static uint64_t emu_samples;                   /* total frames rendered */
static uint32_t fm1_ticks(void) { return (uint32_t)(emu_samples * 544u); } /* ~24 MHz */
static void fm1_delay_ms(uint32_t ms) { (void)ms; }   /* never busy-wait in a worklet */
static void fm1_wdt_feed(void) {}

/* -------------------------------------------------------------- irq HAL --- */
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}

/* ------------------------------------------------------------ input HAL --- */
/* as bsp/boards/fm1/fm1_input.h, without the matrix scan: JS writes the debounced state */
#define FM1_NCOL 11u
#define FM1_NKEY 41u              /* ids: 0..13 buttons, 14..40 note keys */
#define FM1_NENC 7u
enum { FM1_BTN_OCT_DOWN = 0, FM1_BTN_OCT_UP = 1 };

static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

static volatile struct {
    uint32_t notes;              /* bit n = note key n (0 = F3 .. 26 = G5) */
    uint32_t buttons;            /* bit i = matrix button i (0..13) */
    uint32_t pressed, released;  /* button edges since the last fm1_input_edges() */
    uint32_t notes_pressed;      /* note-key press edges */
    uint32_t note_t0[27];        /* the press's tick (seq.c's stats; the browser: the event's) */
    uint8_t enc_rest[FM1_NENC];
    int16_t enc_steps[FM1_NENC];
    uint32_t frames;
} fm1_in;
/* Held MIDI-remote buttons are part of the real FM-1 input HAL. */
static volatile uint32_t fm1_remote_btn;

static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[FM1_NCOL];
static uint8_t fm1_led_breath[FM1_NCOL];
static uint8_t fm1_led_mid[FM1_NCOL];

/* the key matrix's analog readings (fm1_input.h fm1_an; keytouch.c reads the master pot
 * and the battery through it while its scan runs): the browser's scan never runs, kt_on
 * stays 0 and keytouch falls back to fm1_adc_read */
static volatile struct {
    uint16_t con;
    uint8_t drive, probe, pend, col, rowbit, driven, probing, phase;
    uint16_t master, timeouts;
} fm1_an;

static void fm1_input_init(void) {}

static int32_t fm1_enc_take(uint32_t e)
{
    int32_t s = fm1_in.enc_steps[e];
    fm1_in.enc_steps[e] = 0;
    return s;
}

static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t p = fm1_in.pressed;
    if (released)
        *released = fm1_in.released;
    fm1_in.pressed = fm1_in.released = 0;
    return p;
}

static uint32_t fm1_input_note_edges(void)
{
    uint32_t p = fm1_in.notes_pressed;
    fm1_in.notes_pressed = 0;
    return p;
}

static void fm1_led_key(uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    fm1_led[p] |= (uint8_t)(1u << r);
                else
                    fm1_led[p] &= (uint8_t)~(1u << r);
            }
}

/* the power-on LED sweep and the two idle glows (MENU > LEDS): a stub each, the web
 * faceplate shows the LEDs the UI writes, the sweep is the splash's business */
static void fm1_led_dim_level(uint32_t lo) { (void)lo; }
static void fm1_led_anim_start(uint32_t glow) { (void)glow; }
static int fm1_led_anim_on(void) { return 0; }
static void fm1_led_idle_start(uint32_t glow) { (void)glow; }
static int fm1_led_idle_on(void) { return 0; }
static void fm1_led_anim_stop(void) {}

/* 1.8 AFTERTOUCH, the scan's side (src/keytouch.h; before it, felucca.c has the input HAL):
 * the browser's scan never runs, kt_on stays 0 and the pressures never move */
#include "keytouch.h"

/* the SARADC's clock setup macro (bsp/boards/fm1/fm1_adc.h): pure arithmetic, keytouch.c
 * stores it in fm1_an.con; no conversion ever runs here */
#define FM1_ADC_CON(div, delay) ((uint32_t)((div) & 7u) | 8u | ((uint32_t)((delay) & 15u) << 12))

/* -------------------------------------------------------------- adc HAL --- */
enum { FM1_ADC_BATT = 3, FM1_ADC_MASTER = 4 };
static volatile int32_t emu_adc_v[8] = {0, 0, 0, 800, 580, 0, 0, 0};
static int32_t fm1_adc_read(uint32_t ch) { return emu_adc_v[ch & 7u]; }
static void fm1_adc_init(void) {}

/* -------------------------------------------------- lcd HAL (ST7789 model) */
/* lcd.c speaks the real panel protocol; this consumes it into a framebuffer.
 * Only CASET (2A), RASET (2B) and RAMWR (2C) matter; init commands are ignored.
 * Pixels arrive RGB565 big-endian (gfx.c pre-swaps). */
static uint16_t emu_fb[240 * 240];
static volatile uint32_t emu_fb_gen;           /* bumped on every pixel write */
static uint32_t fm1_lcd_timeouts;
static uint8_t st_cmd;
static uint32_t st_argn;
static uint8_t st_arg[4];
static uint16_t st_x0, st_x1 = 239, st_y0, st_y1 = 239, st_cx, st_cy;

static void fm1_lcd_hw_init(void) {}
static void fm1_lcd_backlight(uint32_t on) { (void)on; }
static void fm1_lcd_baud(uint32_t b) { (void)b; }
static void fm1_lcd_wait(void) {}
static void fm1_lcd_deselect(void) {}

static void fm1_lcd_send_cmd(uint8_t c)
{
    st_cmd = c;
    st_argn = 0;
    if (c == 0x2Cu) {                           /* RAMWR resets the cursor */
        st_cx = st_x0;
        st_cy = st_y0;
    }
}

static void fm1_lcd_send_data(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    if (st_cmd == 0x2Au || st_cmd == 0x2Bu) {
        while (n-- && st_argn < 4u)
            st_arg[st_argn++] = *b++;
        if (st_argn == 4u) {
            uint16_t a = (uint16_t)(st_arg[0] << 8 | st_arg[1]);
            uint16_t e = (uint16_t)(st_arg[2] << 8 | st_arg[3]);
            if (st_cmd == 0x2Au) {
                st_x0 = a;
                st_x1 = e;
            } else {
                st_y0 = a;
                st_y1 = e;
            }
        }
        return;
    }
    if (st_cmd != 0x2Cu && st_cmd != 0x3Cu)
        return;
    for (; n >= 2u; n -= 2u, b += 2u) {
        if (st_cx < 240u && st_cy < 240u)
            emu_fb[st_cy * 240u + st_cx] = (uint16_t)(b[0] << 8 | b[1]);
        if (++st_cx > st_x1) {
            st_cx = st_x0;
            if (++st_cy > st_y1)
                st_cy = st_y0;
        }
    }
    emu_fb_gen++;
}

/* ------------------------------------------------------------ audio HAL --- */
/* audio.c's ISR glue compiles against these but is never driven; the worklet
 * pulls mix_block through emu_render instead. */
#define FM1_AUDIO_HALF 0x80u
static void fm1_audio_init(int32_t *b, uint32_t n, void (*i)(void), uint32_t p)
{
    (void)b; (void)n; (void)i; (void)p;
}
static uint8_t fm1_audio_pending(void) { return 0; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return 0; }
static void fm1_audio_ack_half(void) {}
static void fm1_audio_stop(void) {}
void isr_alnk0(void) {}

/* ----------------------------------------------------------- system HAL --- */
static void fm1_reboot(void) {}
static void fm1_enter_uboot(void) {}

/* ------------------------------------------------------ firmware sources --- */
/* the same unity as firmware/src/felucca.c, the web build options in place of build.py's */
#define FELUCCA_FLASH 1          /* storage.c on the RAM image below: settings, projects, user presets */
#define FELUCCA_OTA 0            /* M-UPGRADE and the SysEx editor: stubbed on the web */
#define FELUCCA_CDC 0            /* USB serial console: no */
#define FELUCCA_UAC 0            /* USB audio input: no */
#define FELUCCA_UART 0           /* TRS MIDI IN: the page feeds emu_midi instead */
#define FELUCCA_CPU2 0           /* one core, as the machine's default: the render is synchronous */
#define FM1_CPU1 FELUCCA_CPU2
#define FM1_CPU1_FALLBACK 1
#define FM1_CRASH_CTX_N 1u
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "v1.8.2-mt40x"
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_900"
#endif

#include "felucca_tables.h"      /* generated (tools/gen_tables.py et al.) */
#include "../firmware/bsp/chip/libc.c"
#include "lcd.c"
#include "gfx.c"

#include "core.h"
#include "mt40.h"                /* the MT-40X fork: the MT-40 auto-accompaniment */

/* the flash image (before engines.c: eng_sample.c reads the user sample slots through
 * SMP_USER_XIP, which on hardware is the memory-mapped XIP window — here it points into
 * the same RAM image the storage uses) */
#define EMU_FLASH_BASE 0x90000u
#define EMU_FLASH_SIZE 0x70000u
static uint8_t emu_flash[EMU_FLASH_SIZE];
static volatile uint32_t emu_flash_gen;
#define SMP_USER_XIP(k) (emu_flash + (SMP_USER_BASE - EMU_FLASH_BASE) + (k) * SMP_USER_SIZE)

#include "engines.c"
#include "params.c"
#include "mod.c"                 /* the modulation matrix, used by voice.c and fx.c */
#include "voice.c"
#include "slicer.c"
#include "fx.c"

#include "../firmware/bsp/chip/usb.c"    /* the MIDI queues; usb_start is never called */
#include "song_chain.c"
#include "seq.c"
#include "mt40.c"                /* the MT-40X fork: the riddim bank, the bass keyboard */
#include "audio.c"

#include "panel.c"
#include "ui.c"
#include "menu_items.c"          /* MENU's rows and settings (ui_menu.c, editor_menu.c) */
#include "icons.c"
#include "ui_graph.c"
#include "ui_draw.c"
#include "ui_menu.c"
#include "ui_input.c"
#include "ui_layer.c"

/* storage.c's A/B sector scheme runs unchanged on this array; JS loads a saved image into
 * it before boot and writes it back to IndexedDB whenever emu_flash_gen moves, so projects,
 * autosave, settings and user presets survive a reload. Offsets are the real flash map
 * (storage.c's comment: FL_DATA 0x97000..0xDFFFF, FL_GLOB 0xFC000.., FL_AUTO 0xE5000..,
 * FL_VBANK 0xE7000). persist_boot's FL_FAR(fl_jedec_ram) / fl_plain_window_init come from
 * the HAL here; the chip header's own copies never enter the wasm build. */
static uint8_t flash_ok;
#define FL_FAR(f) f
#define ST_XIP_PTR(off) (emu_flash + ((uint32_t)(off) - EMU_FLASH_BASE))
static uint32_t fl_jedec_ram(void) { return 0x856014u; }   /* the expected part: flash_ok */
static void fl_plain_window_init(void) {}
static int emu_fl_ok(uint32_t off, uint32_t n)
{
    return off >= EMU_FLASH_BASE && n <= EMU_FLASH_SIZE && off - EMU_FLASH_BASE <= EMU_FLASH_SIZE - n;
}
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    if (!emu_fl_ok(off, n))
        return -8;
    memcpy(dst, emu_flash + (off - EMU_FLASH_BASE), n);
    return 0;
}
static int st_erase(uint32_t off)
{
    if (!emu_fl_ok(off, 4096u))
        return -8;
    memset(emu_flash + (off - EMU_FLASH_BASE), 0xFF, 4096u);
    emu_flash_gen++;
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    if (!emu_fl_ok(off, n))
        return -8;
    memcpy(emu_flash + (off - EMU_FLASH_BASE), src, n);
    emu_flash_gen++;
    return 0;
}
#include "storage.c"
#include "upreset.c"
#include "project.c"

#include "keytouch.c"            /* 1.8 AFTERTOUCH: its scans never run, its ADC falls back */
#include "master.c"              /* the MASTER pot */

/* ------------------------------------------------------------- web entry --- */
static float emu_audio_f[2u * 512u];
static int32_t emu_mix[2u * CTL];
static uint32_t emu_ms_acc;

/* power-on, as main.c felucca_init() */
static void emu_felucca_init(void)
{
    uint32_t i;
    chain_defaults(&chain_config);
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    undo_depth++;                             /* (no undo copy of the power-on loads) */
    fm6_init();                               /* every track's FM6 patch: the init voice */
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        set_engine_of(t, TRK_DEF[i][0]);
        apply_preset_to(t, TRK_DEF[i][1]);    /* with its sends */
        t->engine = t->eng_req;
        eng_mem_claim(t);                     /* (zeros already: no note can come before) */
        track_defaults_steps(t);
        if (TRK_DEF[i][2])
            load_pat16(t, PATTERNS[TRK_DEF[i][2] - 1u].note, PATTERNS[TRK_DEF[i][2] - 1u].flags);
        pat_sig[i] = steps_sig(t);
        pat_last[i] = TRK_DEF[i][2];
    }
    undo_depth--;
    song.sel = 0;
    song.master_q12 = 2048;
    transport_req = 0;
    panic_req = 0;
    ui.home = 1;
    ui.force = 1;
}

WASM_EXPORT("emu_init") void emu_init(void)
{
    persist_boot();
    settings_init();
    lcd_init();
    draw_splash();                            /* ui_draw.c: the MT-40X splash */
    fm1_input_init();
    fm1_adc_init();
    panel_init();
    emu_felucca_init();
    master_boot();
    kb_boot_hold = 1;                         /* #137: no notes from the keys before the UI */
}

WASM_EXPORT("emu_start") void emu_start(void)   /* after the splash has shown */
{
    kb_boot_hold = 0;
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.force = 1;
}

WASM_EXPORT("emu_frame") void emu_frame(void)   /* the main loop's UI pass */
{
    uint32_t m = fm1_ms;
    kt_poll(m);                               /* 1.8 AFTERTOUCH (its readings never move here) */
    /* (usb_retry stays out, as the reference emulator's frame: nothing references usb_start,
     * the whole USB-device layer drops out at -O2 — its registers and the pi32 barriers
     * never reach the wasm; the page feeds emu_midi instead) */
    {   /* battery: a steady healthy reading */
        int32_t b = fm1_adc_read(FM1_ADC_BATT);
        if (b > 0)
            song.batt_raw = song.batt_raw ? song.batt_raw + (b - song.batt_raw) / 32 : b;
    }
    master_poll();
    felucca_dbg.ui_frames++;
    felucca_dbg.page = ui.page;
    felucca_dbg.home = ui.home;
    ui_input();
    settings_poll();                          /* queued settings save: only while stopped */
    autosave_poll();                          /* (1.2) the music, stopped and idle: project.c */
    ui_leds();
    ui_draw();
    (void)m;
}

WASM_EXPORT("emu_render") void emu_render(uint32_t n)
{
    uint32_t k, i;
    if (n > 512u)
        n = 512u;
    for (k = 0; k + CTL <= n; k += CTL) {
        mix_block(emu_mix, CTL);
        for (i = 0; i < CTL; i++) {
            int32_t l = emu_mix[2u * i], r = emu_mix[2u * i + 1u];
            if (i & 1u)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
            emu_audio_f[2u * (k + i)] = (float)l * (1.0f / 32768.0f);
            emu_audio_f[2u * (k + i) + 1u] = (float)r * (1.0f / 32768.0f);
        }
    }
    emu_samples += n;
    emu_ms_acc += n * 1000u;
    while (emu_ms_acc >= FS) {
        emu_ms_acc -= FS;
        fm1_ms++;                             /* timer5.c's tick, the audio drives it here */
    }
}

WASM_EXPORT("emu_input") void emu_input(uint32_t notes, uint32_t buttons)
{
    uint32_t pn = fm1_in.notes, pb = fm1_in.buttons, k;
    fm1_in.notes = notes & 0x7FFFFFFu;
    fm1_in.buttons = buttons & 0x3FFFu;
    fm1_in.notes_pressed |= notes & ~pn;
    fm1_in.pressed |= buttons & ~pb;
    fm1_in.released |= pb & ~buttons;
    for (k = 0; k < 27u; k++)                 /* the press's tick, for the latency stats */
        if ((notes & ~pn) >> k & 1u)
            fm1_in.note_t0[k] = fm1_ticks();
}

WASM_EXPORT("emu_enc") void emu_enc(uint32_t e, int32_t steps)
{
    if (e < FM1_NENC)
        fm1_in.enc_steps[e] = (int16_t)(fm1_in.enc_steps[e] + steps);
}

WASM_EXPORT("emu_adc_set") void emu_adc_set(uint32_t ch, int32_t v)
{
    emu_adc_v[ch & 7u] = v;
}

WASM_EXPORT("emu_midi") void emu_midi(uint32_t pkt)   /* a USB-MIDI event packet */
{
    uint32_t at = mi_w % MQ;
    if (mi_w - mi_r >= MQ) {                  /* usb.c's enqueue, its overflow rule */
        midi_in_overflow++;
        mi_r++;
    }
    midi_in_q[at] = pkt;
    mi_w++;
}

/* the console's state probe for the smoke test: 0 playing, 1 tick, 2 bpm, 3 the menu open,
 * 4 the menu's row cursor, 5 the selected track, 6 the loaded riddim (-1 none), 7 MT-40X on,
 * 8 the keyboard's root (-1 none), 9 the keyboard's quality, 10 a 24-bit hash of the four
 * tracks' first 64 steps, 11 the selected track's playhead, 12 G_BPM as the tracks play it */
WASM_EXPORT("emu_dbg") uint32_t emu_dbg(uint32_t idx)
{
    switch (idx) {
    case 0: return song.playing;
    case 1: return song.tick;
    case 2: return (uint32_t)song.g[G_BPM];
    case 3: return ui.menu;
    case 4: return ui.menu_sel;
    case 5: return song.sel;
    case 6: return (uint32_t)(mt40_rid + 1);
    case 7: return mt40_on;
    case 8: return (uint32_t)(mt40_root + 1);
    case 9: return mt40_qual;
    case 10: {
        uint32_t t, i, h = 0;
        for (t = 0; t < 4u; t++)
            for (i = 0; i < NSTEP; i++)
                h = h * 131u + (trk[t].step[i].n ? trk[t].step[i].note[0] : 255u);
        return h & 0xFFFFFFu;
    }
    case 11: return (uint32_t)trk[song.sel].seq_idx;
    case 12: return (uint32_t)song.g[G_BPM];
    default: return 0;
    }
}

WASM_EXPORT("emu_fb_ptr") uint32_t emu_fb_ptr(void) { return (uint32_t)(uintptr_t)emu_fb; }
WASM_EXPORT("emu_fb_gen") uint32_t emu_fb_gen_get(void) { return emu_fb_gen; }
WASM_EXPORT("emu_audio_ptr") uint32_t emu_audio_ptr(void) { return (uint32_t)(uintptr_t)emu_audio_f; }
WASM_EXPORT("emu_led_ptr") uint32_t emu_led_ptr(void) { return (uint32_t)(uintptr_t)fm1_led; }
WASM_EXPORT("emu_led_dim_ptr") uint32_t emu_led_dim_ptr(void) { return (uint32_t)(uintptr_t)fm1_led_dim; }
WASM_EXPORT("emu_led_breath_ptr") uint32_t emu_led_breath_ptr(void) { return (uint32_t)(uintptr_t)fm1_led_breath; }
WASM_EXPORT("emu_led_mid_ptr") uint32_t emu_led_mid_ptr(void) { return (uint32_t)(uintptr_t)fm1_led_mid; }
WASM_EXPORT("emu_flash_ptr") uint32_t emu_flash_ptr(void) { return (uint32_t)(uintptr_t)emu_flash; }
WASM_EXPORT("emu_flash_size") uint32_t emu_flash_size(void) { return EMU_FLASH_SIZE; }
WASM_EXPORT("emu_flash_gen") uint32_t emu_flash_gen_get(void) { return emu_flash_gen; }

/* ---------------------------------------------------------- MT-40X controls ---
 * The page's only control surface stays the firmware's own menu (hold HOME, SELECT, KNOB
 * 1-4, exactly as on the machine); these entry points are the web sugar around it: the
 * riddim list, a one-click load (the RIDDIM page's own code), and the four tracks'
 * readback for the pattern view. */
WASM_EXPORT("emu_mt40_count") uint32_t emu_mt40_count(void)
{
    return MT40_RIDDIM_N;
}

WASM_EXPORT("emu_mt40_name") uint32_t emu_mt40_name(uint32_t rid)   /* const char *, a wasm offset */
{
    if (rid >= MT40_RIDDIM_N)
        return 0;
    return (uint32_t)(uintptr_t)MT40_RIDDIMS[rid]->name;
}

WASM_EXPORT("emu_mt40_bpm") uint32_t emu_mt40_bpm(uint32_t rid)
{
    return rid < MT40_RIDDIM_N ? MT40_RIDDIMS[rid]->bpm : 0u;
}

WASM_EXPORT("emu_mt40_load") void emu_mt40_load(uint32_t rid)   /* the RIDDIM page's OCT+ */
{
    if (rid < MT40_RIDDIM_N)
        mt40_load(rid);
}

WASM_EXPORT("emu_mt40_set") void emu_mt40_set(uint32_t on)      /* the MT-40X row's OFF / ON */
{
    mt40_set(on ? 1u : 0u);
}

WASM_EXPORT("emu_mt40_state") uint32_t emu_mt40_state(void)
{   /* bit 0 on, bits 1..8 the loaded riddim + 1 (0 none), bits 9..16 the root + 1 (0 none),
     * bits 17..18 the quality */
    return (uint32_t)((mt40_on ? 1u : 0u)
                    | (((uint32_t)(mt40_rid + 1) << 1) & 0x1FEu)
                    | (((uint32_t)(mt40_root + 1) << 9) & 0x1FE00u)
                    | ((uint32_t)(mt40_qual & 3u) << 17));
}

/* the four tracks' readback, the pattern view: 255 a rest, 254 a TIE (a held note going on),
 * else the step's first note. Track 4 is the DRUM engine: its notes are the lanes'
 * (eng_drum.c DRUM_LANE_NOTE). */
WASM_EXPORT("emu_seq_len") uint32_t emu_seq_len(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < NTRK; i++)
        if (trk[i].p[P_SLEN] > (int16_t)n)
            n = (uint32_t)trk[i].p[P_SLEN];
    return n;
}

WASM_EXPORT("emu_seq_track_len") uint32_t emu_seq_track_len(uint32_t track)
{
    return track < NTRK ? (uint32_t)trk[track].p[P_SLEN] : 0u;
}

WASM_EXPORT("emu_seq_note") uint32_t emu_seq_note(uint32_t track, uint32_t step)
{
    const step_t *sp;
    if (track >= NTRK || step >= NSTEP)
        return 255u;
    sp = &trk[track].step[step];
    if (track == 3u) {                        /* the DRUM engine: a step's hit is its lane mask */
        uint32_t m = sp->hit, l = 0;
        if (!m)
            return 255u;
        while (!(m >> l & 1u))
            l++;
        return DRUM_LANE_NOTE[l & 7u];
    }
    if (sp->time == ST_TIE)
        return 254u;
    if (sp->time == ST_NOTE && sp->n)
        return sp->note[0];
    return 255u;
}

WASM_EXPORT("emu_seq_pos") uint32_t emu_seq_pos(void)      /* the selected track's playhead */
{
    return song.playing ? (uint32_t)trk[song.sel].seq_idx : 0u;
}

WASM_EXPORT("emu_seq_pos_track") uint32_t emu_seq_pos_track(uint32_t track)
{
    return track < NTRK && song.playing ? (uint32_t)trk[track].seq_idx : 0u;
}

WASM_EXPORT("emu_seq_playing") uint32_t emu_seq_playing(void)
{
    return song.playing ? 1u : 0u;
}
