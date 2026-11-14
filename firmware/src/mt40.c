/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 MT-40X contributors (Felucca by Leo Kuroshita / Hugelton Instruments,
 * riddim bank by altessproduction's SLOOP Dub 1.6, both GPL-3.0-only) */
/* MT-40X: mt40.h's side. One riddim = the machine's three tracks:
 *   T4 the drums (the DRUM engine, one velocity a step: a single lane hit, or two --
 *      drum_lane's low and high nibbles -- when the transcription's backbeat strikes
 *      its kick and its snare together, velocities within twenty of each other),
 *   T3 the bass (the groove rides the bar's root and never holds -- every note re-strikes
 *      as the machine plucks it, tools/mt40_bass.py; a rest HOLDS the re-strike, written
 *      as a TIE; MT40_CUT releases: the plucked tails the MT-40's bass lives on. The
 *      degrees 240..254 are the semitones below the bar's root, the sub-root walks of
 *      the BaroqueDub transcriptions, v0.6).
 *   T2 the skank (the stabs, real chords: the bar's root with the chord's quality).
 *   T1 stays the user's: the melody was SLOOP Dub's toaster's voice, the machine has none
 *   (v0.5 dropped it; the load empties the track, its sound and its settings stay).
 * The keyboard: key n (53 + k) is a chord: n the root MAJ, n + 1 MIN of n - 1, n + 2
 * DOM7 of n - 2; the same key again takes MAJ back (every root is reachable). The roots
 * ride the bass range (28..50, one octave down as needed); the whole accompaniment is
 * re-voiced on the press. Nothing pressed: the riddim plays as written, its four bars'
 * own roots. With the transport stopped the first chord SYNCHRO-STARTs it (the MT-40's). */

#include "mt40.h"
#include "mt40_riddims.h"

uint8_t mt40_on;
int32_t mt40_rid = -1;
int32_t mt40_root = -1;
uint8_t mt40_qual = MT40_Q_MIN;

static uint8_t mt40_last_k = 27u;         /* the last chord's key (the MAJ escape of +1 / +2) */
static int32_t mt40_last_n = -1;          /* .. its raw pitch (53 + k) */

void mt40_set(uint32_t on)
{
    mt40_on = (uint8_t)(on != 0u);
}

/* the step's writing, in the order live recording's own edits use: the count last, so a
 * step the audio ISR reads mid-edit is read empty rather than torn */
static void mt40_rest(track_t *t, uint32_t s)
{
    step_t *st = &t->step[s];
    st->n = 0;
    st->hit = 0;
    st->acc = 0;
    st->time = ST_REST;
    st->flags = 0;
    st->vel = 0;
    st->probability = 0;
}

static void mt40_note(track_t *t, uint32_t s, uint8_t note, uint8_t vel, uint8_t tie)
{
    step_t *st = &t->step[s];
    st->hit = 0;
    st->acc = 0;
    st->note[0] = note;
    st->n = 1;
    st->time = tie ? ST_TIE : ST_NOTE;
    st->flags = 0;
    st->vel = vel;
    st->probability = 0;
}

static void mt40_chord(track_t *t, uint32_t s, uint32_t root, uint32_t qual, uint8_t vel)
{
    static const uint8_t IV[3][4] = {{0, 4, 7, 10}, {0, 3, 7, 10}, {0, 4, 7, 10}};
    static const uint8_t NC[3] = {3, 3, 4};          /* MAJ, MIN, DOM7 (the b7 only on DOM7) */
    step_t *st = &t->step[s];
    uint32_t i;
    st->hit = 0;
    st->acc = 0;
    for (i = 0; i < NC[qual]; i++)
        st->note[i] = (uint8_t)(root + IV[qual][i]);
    st->n = (uint8_t)NC[qual];
    st->time = ST_NOTE;
    st->flags = 0;
    st->vel = vel;
    st->probability = 0;
}

static uint32_t mt40_clamp_root(uint32_t n)          /* a key's pitch into the bass' range */
{
    while (n > 50u)
        n -= 12u;
    while (n < 28u)
        n += 12u;
    return n;
}

/* the variant of a degree as the keyboard's quality wants it: MIN as written, MAJ raises
 * the minor third, DOM7 is MAJ with the bar's last fifth flat (the turnaround). The same
 * rules tools/gen_mt40_riddims.py documents; CALIBRATION.md is where they get checked.
 * in7: the step rides the bar's last fifth -- the note AND its re-strikes (the pluck is
 * the same note, it goes flat with it) */
static uint32_t mt40_deg(uint32_t deg, uint32_t qual, uint32_t in7)
{
    if (deg == 3u && qual != MT40_Q_MIN)
        return 4u;
    if (qual == MT40_Q_DOM7 && deg == 7u && in7)
        return 10u;
    return deg;
}

/* the three tracks re-voiced: root < 0 as written (the riddim's own four bar roots),
 * root >= 0 the keyboard's chord (all four bars its root, its quality). noinline: a
 * chord press re-voices 128 steps, out of the audio ISR's keyboard loop */
static __attribute__((noinline)) void mt40_voice(int32_t root)
{
    const mt40_riddim_t *r;
    uint32_t s, b;
    if (mt40_rid < 0 || (uint32_t)mt40_rid >= MT40_RIDDIM_N)
        return;
    r = MT40_RIDDIMS[mt40_rid];
    for (b = 0; b < 4u; b++) {
        uint32_t rroot = root >= 0 ? (uint32_t)root : r->roots[b];
        uint32_t qual = root >= 0 ? mt40_qual : r->skank_qual;
        uint32_t last7 = 0xFFFFFFFFu, last7end = 0xFFFFFFFFu;   /* the bar's last fifth's
                                          * run (DOM7's turnaround): the strike, its
                                          * re-strikes with it */
        for (s = b * 16u; s < b * 16u + 16u; s++)
            if (r->bass_deg[s] == 7u)
                last7 = s;
        if (last7 != 0xFFFFFFFFu) {
            last7end = last7;
            while (last7end + 1u < b * 16u + 16u && r->bass_deg[last7end + 1u] == 7u)
                last7end++;                  /* forward: the plucks of that fifth */
            while (last7 > b * 16u && r->bass_deg[last7 - 1u] == 7u)
                last7--;                     /* back: the strike of the run (a pluck at 15) */
        }
        /* T3 the bass: a written note strikes, a rest holds it (TIE), MT40_CUT ends it.
         * The degrees 240..254 are the semitones below the bar's root (239 - byte): the
         * transcription's sub-root walks, Heavenless's G E G G# over its A */
        for (s = b * 16u; s < b * 16u + 16u; s++) {
            uint32_t deg = r->bass_deg[s];
            track_t *t = &trk[2];
            if (deg == MT40_CUT) {
                mt40_rest(t, s);
            } else if (deg == MT40_REST) {
                mt40_rest(t, s);
                t->step[s].time = ST_TIE;            /* a hold the engine keeps while it sounds */
            } else {
                int32_t d;
                if (deg >= 240u)                     /* the sub-root walk, no variant */
                    d = 239 - (int32_t)deg;
                else
                    d = (int32_t)mt40_deg(deg, qual, s >= last7 && s <= last7end);
                mt40_note(t, s, (uint8_t)((int32_t)rroot + d), r->bass_vel[s], 0);
            }
        }
        /* T2 the skank: the bar's chord, the stab's velocity; the rests release */
        for (s = b * 16u; s < b * 16u + 16u; s++) {
            track_t *t = &trk[1];
            if (!r->skank_vel[s]) {
                mt40_rest(t, s);
            } else {
                uint32_t cr = root >= 0 ? 57u + rroot % 12u : 57u + r->chords[b] % 12u;
                mt40_chord(t, s, cr, qual, r->skank_vel[s]);   /* the stab's register stays the bank's */
            }
        }
    }
}

/* the riddim's drums into T4, and, the first time, its engine and its grid */
static void mt40_drums(const mt40_riddim_t *r)
{
    track_t *t = &trk[3];
    uint32_t s;
    static const int16_t KIT[8] = DRUM_KIT_E;        /* core.h: the DRUM engine's defaults */
    if (t->engine != ENGI_DRUM && t->eng_req != ENGI_DRUM) {
        t->eng_req = ENGI_DRUM;
        for (s = 0; s < 8u; s++)
            t->p[P_E0 + (int32_t)s] = KIT[s];
    }
    for (s = 0; s < MT40_STEPS; s++) {
        mt40_rest(t, s);
        uint32_t lv = r->drum_lane[s];
        if (lv != MT40_REST) {
            step_t *st = &t->step[s];
            st->hit = (uint8_t)(1u << (lv & 15u));   /* 240 + lane: the single hit */
            if ((lv >> 4) != 15u)                    /* the high nibble: the step's second
                                                      * lane, the backbeats' kick and snare
                                                      * unisons (one velocity for both) */
                st->hit |= (uint8_t)(1u << (lv >> 4));
            st->time = ST_NOTE;
            st->vel = r->drum_vel[s];
        }
    }
    t->p[P_SLEN] = 64;
    t->p[P_SDIV] = 2;                                /* 1/16: the riddims' grid */
    t->p[P_SGATE] = 100;
}

/* the MT-40's kind of tones, posed on every load: the Casiotone timbres are simple
 * attack-decay waves, ANALOG approximates them well enough to play a riddim right away
 * (the faithful 22-tone bank is the next calibration, the riddims' own). The user's
 * LEVEL, PAN, sends and INSERT stay; an engine change lands at a block start (voice.c) */
static void mt40_tone(track_t *t, const int16_t e[8], int16_t atk, int16_t dec, int16_t sus, int16_t rel)
{
    uint32_t i;
    t->eng_req = 0u;                                 /* ENGI_ANALOG (ENGINES[0]) */
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + (int32_t)i] = e[i];
    t->p[P_ATK] = atk;
    t->p[P_DEC] = dec;
    t->p[P_SUS] = sus;
    t->p[P_REL] = rel;
    t->user = 0u;                                    /* the sound is ours, no user slot behind it */
}

static void mt40_sounds(void)
{
    /* E0 WAVE (0 SAW, 1 SQR, 2 TRI, 3 SIN, 4 PWM, 5 SYNC, 6 SUB), E1 DTN, E2 MIX, E3 NOIS,
     * E4 CUT, E5 RES, E6 DRV, E7 KTR; then the amp env ATK DEC SUS REL (SUS %). As
     * mt40_drums' kit: written in place, the engine switch lands at a block start (voice.c)
     * T1 keeps whatever the user plays on it: the load only empties its steps */
    static const int16_t BASS[8] = {1, 0, 0, 0, 50, 45, 30, 100};   /* the MT-40's hollow square pluck */
    static const int16_t SKNK[8] = {4, 0, 0, 0, 78, 12, 10, 64};    /* the stab's pluck, shorter */
    mt40_tone(&trk[2], BASS, 0, 55, 35, 30);         /* T3: plucky, it decays even on its TIEs */
    mt40_tone(&trk[1], SKNK, 0, 50, 25, 25);         /* T2: the stab dies within its short gate */
}

void mt40_load(uint32_t rid)
{
    const mt40_riddim_t *r;
    uint32_t i;
    if (rid >= MT40_RIDDIM_N)
        return;
    mt40_rid = (int32_t)rid;
    mt40_root = -1;
    mt40_last_n = -1;
    mt40_last_k = 27u;
    mt40_qual = MT40_Q_MIN;
    r = MT40_RIDDIMS[rid];
    mt40_drums(r);
    for (i = 0; i < MT40_STEPS; i++)                 /* T1 emptied: the riddim has no melody,
                                                      * the track stays the user's (its sound too) */
        mt40_rest(&trk[0], i);
    for (i = 0; i < 2u; i++) {                       /* T2 skank, T3 bass: their grid */
        track_t *t = &trk[1 + i];
        t->p[P_SLEN] = 64;
        t->p[P_SDIV] = 2;
    }
    trk[1].p[P_SGATE] = 40;                          /* the skank stabs, short (P_VOICE POLY below) */
    trk[1].p[P_VOICE] = V_POLY;                      /* the stabs are real chords */
    trk[2].p[P_SGATE] = 127;                         /* the bass rings over its TIEs */
    trk[2].p[P_VOICE] = V_MONO;                      /* one note, as a bass */
    mt40_sounds();                                   /* the MT-40's kind of tones on T2, T3 */
    mt40_voice(-1);                                  /* as written, until a key is pressed */
    song.g[G_BPM] = (int16_t)r->bpm;                 /* the riddim's tempo (MENU > AUDIO > BPM) */
}

/* a key down while MT-40X is armed: the bass keyboard. n = 53 + k: n is the root (MAJ),
 * n + 1 is MIN of n - 1, n + 2 is DOM7 of n - 2; the same key again takes MAJ back, so
 * every root stays reachable. 1 = the key was taken (it sounds nothing). noinline: the
 * re-voicing and the SYNCHRO START are out of the ISR's key loop */
__attribute__((noinline)) uint32_t mt40_key_press(uint32_t k)
{
    int32_t n;
    if (!mt40_on || k >= 27u || song.grid)
        return 0;
    n = 53 + (int32_t)k;
    if (mt40_root >= 0 && n == mt40_last_n + 1 && k != mt40_last_k) {
        mt40_qual = MT40_Q_MIN;                      /* the key above the root: minor of it */
    } else if (mt40_root >= 0 && n == mt40_last_n + 2 && k != mt40_last_k) {
        mt40_qual = MT40_Q_DOM7;                     /* two above: seventh of it */
    } else {
        mt40_root = (int32_t)mt40_clamp_root((uint32_t)n);
        mt40_qual = MT40_Q_MAJ;
    }
    mt40_last_n = n;
    mt40_last_k = (uint8_t)k;
    if (!song.playing)
        transport_req = 1u;                          /* SYNCHRO START: the first chord starts it */
    mt40_voice(mt40_root);
    return 1u;
}
