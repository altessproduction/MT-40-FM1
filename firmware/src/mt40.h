/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 MT-40X contributors (Felucca by Leo Kuroshita / Hugelton Instruments,
 * riddim bank by altessproduction's SLOOP Dub 1.6, both GPL-3.0-only) */
/* MT-40X: the Casio MT-40 / 41's auto-accompaniment on the FM-1. A riddim (its drum loop,
 * its bass, its skank — the machine's three parts; v0.5 dropped the melody, T1 of the
 * sequencer stays the user's) loads into the tracks; the keys, while MT-40X is armed, are
 * the MT-40's bass keyboard: a key re-roots the whole accompaniment live, the key a
 * semitone above plays it minor, two above seventh (CALIBRATION.md's rule). The bank is
 * mt40_riddims.h (tools/gen_mt40_riddims.py): the twenty-eight riddims of SLOOP Dub 1.6,
 * the bass re-struck as the machine plucks it (tools/mt40_bass.py). The drums never
 * transpose; the bass and the skank's chords follow the keyboard; nothing pressed, the
 * riddim plays as written (its own four bar roots). */
#ifndef FELUCCA_MT40_H
#define FELUCCA_MT40_H

#include <stdint.h>

#define MT40_STEPS 64u
#define MT40_REST 255u                    /* a step with nothing */
#define MT40_CUT 253u                     /* the held note ends here (the plucked tails) */

enum { MT40_Q_MAJ = 0, MT40_Q_MIN = 1, MT40_Q_DOM7 = 2 };

typedef struct {
    const char *name;                     /* "ONE DROP", the MENU row's value */
    uint8_t bpm;                          /* the riddim's tempo (the load puts it in) */
    uint8_t roots[4];                     /* the four bars' bass roots as written (MIDI 28..50) */
    uint8_t chords[4];                    /* the skank stabs' chord root per bar (pitch class used) */
    uint8_t skank_qual;                   /* the stabs' chord as written: MT40_Q_* */
    uint8_t drum_lane[MT40_STEPS];        /* the hit's lane 0..7 (DRUM_LANE_NOTE); 240 + lane
                                           * a single hit, the low nibble lane and the high
                                           * nibble the step's second lane a double hit (the
                                           * backbeats' kick and snare unisons), MT40_REST none */
    uint8_t drum_vel[MT40_STEPS];
    uint8_t bass_deg[MT40_STEPS];         /* semitones over the bar's root; 240..254 the
                                           * semitones below it (239 - byte, the sub-root
                                           * walks); MT40_CUT ends the hold */
    uint8_t bass_vel[MT40_STEPS];
    uint8_t skank_vel[MT40_STEPS];        /* the stab's velocity, 0 = none; its root is the bar's */
} mt40_riddim_t;

extern const mt40_riddim_t *const MT40_RIDDIMS[];
extern const uint32_t MT40_RIDDIM_N;

extern uint8_t mt40_on;                   /* MENU > AUDIO > MT-40X: the keys are the bass keyboard */
extern int32_t mt40_rid;                  /* the loaded riddim, -1 none */
extern int32_t mt40_root;                 /* the keyboard's chord root (MIDI), -1 nothing pressed:
                                           * the riddim plays as written */
extern uint8_t mt40_qual;                 /* the keyboard's chord quality: MT40_Q_* */

void mt40_set(uint32_t on);               /* the MENU row's OFF / ON */
void mt40_load(uint32_t rid);             /* a riddim into the four tracks, its tempo in */
uint32_t mt40_key_press(uint32_t k);      /* a key down while armed: the bass keyboard's chord.
                                           * 1 = the key was taken (it sounds nothing) */

#endif
