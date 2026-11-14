/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 MT-40X contributors (Felucca by Leo Kuroshita / Hugelton Instruments,
 * riddim bank by altessproduction's SLOOP Dub 1.6, both GPL-3.0-only) */
/* MT-40X: mt40.h's side. One riddim = the four tracks:
 *   T4 the drums (the DRUM engine, one lane hit a step at the most, velocities kept),
 *   T3 the bass (the groove rides the bar's root; a rest HOLDS, written as a TIE;
 *      MT40_CUT releases: the plucked tails the MT-40's bass lives on),
 *   T2 the skank (the stabs, real chords: the bar's root with the chord's quality),
 *   T1 the melody (as written; the keyboard transposes it with the chord).
 * The keyboard: key n (53 + k) is a chord: n the root MAJ, n + 1 MIN of n - 1, n + 2
 * DOM7 of n - 2; the same key again takes MAJ back (every root is reachable). The roots
 * ride the bass range (28..50, one octave down as needed); the whole accompaniment is
 * re-voiced on the press, the melody by the root's distance to the riddim's own. Nothing
 * pressed: the riddim plays as written, its four bars' own roots. With the transport
 * stopped the first chord SYNCHRO-STARTs it (the MT-40's). */

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
 * rules tools/gen_mt40_riddims.py documents; CALIBRATION.md is where they get checked */
static uint32_t mt40_deg(uint32_t deg, uint32_t qual, uint32_t last7, uint32_t s)
{
    if (deg == 3u && qual != MT40_Q_MIN)
        return 4u;
    if (qual == MT40_Q_DOM7 && deg == 7u && s == last7)
        return 10u;
    return deg;
}

/* the four tracks re-voiced: root < 0 as written (the riddim's own four bar roots),
 * root >= 0 the keyboard's chord (all four bars its root, its quality). noinline: a
 * chord press re-voices 192 steps, out of the audio ISR's keyboard loop */
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
        int32_t mel_d = root >= 0 ? (int32_t)rroot - (int32_t)r->roots[0] : 0;
        uint32_t last7 = 0xFFFFFFFFu;                /* the bar's last fifth (DOM7's turnaround) */
        for (s = b * 16u; s < b * 16u + 16u; s++)
            if (r->bass_deg[s] == 7u)
                last7 = s;
        /* T3 the bass: a written note strikes, a rest holds it (TIE), MT40_CUT ends it */
        for (s = b * 16u; s < b * 16u + 16u; s++) {
            uint32_t deg = r->bass_deg[s];
            track_t *t = &trk[2];
            if (deg == MT40_CUT) {
                mt40_rest(t, s);
            } else if (deg == MT40_REST) {
                mt40_rest(t, s);
                t->step[s].time = ST_TIE;            /* a hold the engine keeps while it sounds */
            } else {
                mt40_note(t, s, (uint8_t)(rroot + mt40_deg(deg, qual, last7, s)), r->bass_vel[s], 0);
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
        /* T1 the melody: as written, the keyboard's distance applied */
        for (s = b * 16u; s < b * 16u + 16u; s++) {
            track_t *t = &trk[0];
            uint32_t n = r->mel_note[s];
            if (n == MT40_REST) {
                mt40_rest(t, s);
                t->step[s].time = ST_TIE;
            } else {
                int32_t x = (int32_t)n + mel_d;
                mt40_note(t, s, (uint8_t)(x < 0 ? 0 : x > 127 ? 127 : x), r->mel_vel[s], 0);
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
        if (r->drum_lane[s] != MT40_REST) {
            step_t *st = &t->step[s];
            st->hit = (uint8_t)(1u << r->drum_lane[s]);
            st->time = ST_NOTE;
            st->vel = r->drum_vel[s];
        }
    }
    t->p[P_SLEN] = 64;
    t->p[P_SDIV] = 2;                                /* 1/16: the riddims' grid */
    t->p[P_SGATE] = 100;
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
    for (i = 0; i < 3u; i++) {                       /* T1 mel, T2 skank, T3 bass: their grid */
        track_t *t = &trk[i];
        t->p[P_SLEN] = 64;
        t->p[P_SDIV] = 2;
    }
    trk[0].p[P_SGATE] = 127;                         /* the melody rings (its TIEs hold it) */
    trk[1].p[P_SGATE] = 40;                          /* the skank stabs, short (P_VOICE POLY below) */
    trk[1].p[P_VOICE] = V_POLY;                      /* the stabs are real chords */
    trk[2].p[P_SGATE] = 127;                         /* the bass rings over its TIEs */
    trk[2].p[P_VOICE] = V_MONO;                      /* one note, as a bass */
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
