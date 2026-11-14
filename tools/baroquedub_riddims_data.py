#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MT-40X v0.6: the rhythm sections of five classic riddims, as the BaroqueDub MIDI
# transcriptions play them (https://baroquedub.co.uk/archive/audio_riddims.html, Jerome
# Di Pietro's MIDI REGGAE RIDDIMS, Creative Commons Sampling 1.0 -- not for sale or
# profit; the originals were sourced from the Jamaican Riddim Directory, tempo.mid was
# donated by Jahpapu Kongo). tools/midi_riddims.py converts them into riddims.json;
# tools/verify_bank.py checks the result against this module field by field.
#
# The events are the MIDI files' notes quantized onto the sixteenth grid (the files are
# on it: the largest drift is 0.27 of a sixteenth, a human push, rounded). One tuple per
# note: (step, GM note, velocity, duration in hundredths of a sixteenth). "bars" is the
# transcribed loop's length; the converter tiles it over the bank's four bars. The drums
# are the files' own tracks (kik, snare, hats -- and tempo's KIT); the bass is the bass
# track. The skank: only tempo.mid carries a chord track (its "Chords", the Am / F
# stabs); the four others have none -- the stabs stay the bank's session part (the
# SLOOP Dub lines), the chord roots below are the harmony the transcription's bass
# implies (see midi_riddims.py). The Horns Melody of tempo.mid is the fourth instrument
# the machine has none of: dropped, like SLOOP Dub's toaster's voice was in v0.5.
#
# The transcriptions' tempos are the ones the filtered loops are labelled with
# (gunman_[filtered]_78.181.mp3 and so on), rounded to the whole BPM.

RIDDIMS = {
    "GUNMAN": {
        "bpm": 78,
        "bars": 2,
        "roots": [31, 31],                   # G1: the bass's own centre (G F F# walk)
        "chords": [31, 31],                  # the stabs: Gm
        "skank_qual": 1,
        "bass": [
            (0, 31, 118, 146), (3, 31, 108, 90), (4, 29, 120, 208), (6, 31, 118, 196),
            (8, 29, 120, 124), (12, 29, 108, 226), (14, 30, 108, 221),
            (16, 31, 105, 133), (19, 31, 89, 100), (20, 29, 108, 208), (22, 31, 105, 189),
            (24, 29, 127, 143), (28, 29, 108, 217), (30, 30, 116, 208),
        ],
        "drums": [
            (0, 35, 101), (3, 35, 95), (4, 35, 101), (6, 35, 101), (8, 35, 101),
            (12, 35, 101), (14, 35, 101), (16, 35, 101), (19, 35, 95), (20, 35, 101),
            (22, 35, 101), (24, 35, 101), (28, 35, 101), (30, 35, 101),
            (4, 38, 100), (12, 38, 100), (20, 38, 100), (28, 38, 100),
        ] + [(s, 42, 74 if s % 2 == 0 else 29) for s in range(32)],
    },
    "HEAVENLESS": {
        "bpm": 77,
        "bars": 2,
        "roots": [33, 33],                   # A1: the static Am vamp (A G E G# only)
        "chords": [33, 33],                  # the stabs: Am
        "skank_qual": 1,
        "bass": [
            (0, 33, 107, 71), (1, 33, 107, 71), (2, 33, 107, 93), (3, 33, 107, 93),
            (4, 31, 114, 83), (5, 28, 127, 82), (6, 31, 114, 96), (7, 32, 102, 33),
            (8, 33, 107, 71), (9, 33, 107, 71), (10, 33, 107, 93), (11, 33, 107, 93),
            (12, 31, 107, 71), (13, 28, 127, 82), (14, 31, 107, 71), (15, 31, 117, 49),
            (16, 33, 107, 71), (17, 33, 107, 71), (18, 33, 107, 93), (19, 33, 107, 93),
            (20, 31, 114, 83), (21, 28, 127, 82), (22, 31, 114, 96), (23, 32, 102, 33),
            (24, 33, 107, 71), (25, 33, 107, 71), (26, 33, 107, 93), (27, 33, 107, 93),
            (28, 31, 107, 71), (29, 28, 127, 82), (30, 31, 107, 71), (31, 31, 117, 49),
        ],
        "drums": [
            (0, 35, 101), (3, 35, 101), (4, 35, 101), (7, 35, 101), (8, 35, 101),
            (11, 35, 101), (12, 35, 101), (15, 35, 101), (16, 35, 101), (19, 35, 101),
            (20, 35, 101), (23, 35, 101), (24, 35, 101), (27, 35, 101), (28, 35, 101),
            (31, 35, 101),
            (4, 38, 107), (12, 38, 107), (20, 38, 107), (28, 38, 107),
        ] + [(s, 42, 74 if s % 2 == 0 else (49 if s == 31 else 29)) for s in range(32)],
    },
    "SLENG TENG": {
        "bpm": 82,
        "bars": 1,
        "roots": [37],                       # C#2: the pedal the whole riddim rides
        "chords": [37, 35, 33, 35],          # the stabs stay the bank's C#m B A B reading
        "skank_qual": 1,
        "bass": [                            # the famous line: C# D D D | C# D D D |
            (0, 37, 118, 83), (1, 38, 127, 86), (2, 38, 121, 91), (3, 38, 124, 83),
            (4, 37, 121, 79), (5, 38, 120, 93), (6, 38, 121, 72), (7, 38, 126, 93),
            (8, 37, 119, 71), (9, 38, 121, 87), (10, 38, 126, 82), (11, 38, 126, 109),
            (12, 43, 112, 56), (14, 42, 118, 123),   # | G . F# .
        ],
        "drums": [
            (0, 35, 90), (4, 35, 90), (6, 35, 90), (8, 35, 90), (12, 35, 90),
            (4, 38, 100), (12, 38, 100),
        ] + [(s, 42, 74 if s % 2 == 0 else 29) for s in range(16)],
    },
    "TEMPO": {
        "bpm": 70,
        "bars": 1,
        "roots": [33],                       # A1: Am (beats 1-2) to F (beats 3-4) every bar
        "chords": [33],                      # the stabs: Am, the Chords track's own tonic
        "skank_qual": 1,
        "skank": [(2, 99), (6, 99), (10, 99), (14, 99)],   # the Chords track's stabs
        "bass": [                            # A A . . . . . E F E F . F E F
            (0, 33, 99, 94), (2, 33, 99, 94), (7, 28, 99, 94), (8, 29, 99, 94),
            (9, 28, 99, 94), (10, 29, 99, 94), (12, 29, 99, 94), (14, 28, 99, 94),
            (15, 29, 99, 94),
        ],
        "drums": [                           # the KIT: a steppers, kick every beat
            (0, 36, 99), (4, 36, 99), (8, 36, 99), (12, 36, 99),
            (4, 38, 99), (12, 38, 99),
        ] + [(s, 42, 99) for s in range(0, 16, 2)],
    },
    "PEANY PEANY": {
        "bpm": 81,
        "bars": 1,
        "roots": [38],                       # D2: the bass's D F E B (D dorian) centre
        "chords": [38],                      # the stabs: Dm
        "skank_qual": 1,
        "bass": [
            (0, 38, 118, 108), (1, 41, 117, 93), (2, 40, 127, 143), (5, 40, 125, 71),
            (6, 41, 122, 71), (7, 41, 120, 53), (9, 35, 103, 96), (10, 40, 121, 171),
            (13, 35, 102, 75), (14, 38, 114, 119), (15, 41, 120, 96),
        ],
        "drums": [                           # the steppers: the kick on every beat
            (0, 35, 101), (2, 35, 101), (4, 35, 101), (6, 35, 101), (8, 35, 101),
            (10, 35, 101), (12, 35, 101),
            (5, 36, 68), (13, 36, 68),       # the kick pickups
            (4, 38, 47), (12, 38, 47),       # the ghost snares
            (6, 37, 100), (14, 37, 100),     # the sidestick accents
        ] + [(s, 42, 74 if s % 2 == 0 else 29) for s in range(16)],
    },
}


def check():
    """the module's own consistency: steps on the grid, velocities, loop lengths"""
    for name, src in RIDDIMS.items():
        n = src["bars"] * 16
        assert n in (16, 32), f"{name}: bars"
        assert len(src["roots"]) == src["bars"], name
        assert len(src["chords"]) in (src["bars"], 4), name   # SLENG TENG keeps its 4-bar stabs
        for st, note, vel, dur in src["bass"]:
            assert 0 <= st < n and 28 <= note <= 50 and 0 < vel <= 127 and 0 < dur, \
                f"{name}: bass {st},{note}"
        for st, gm, vel in src["drums"]:
            assert 0 <= st < n and gm in (35, 36, 37, 38, 39, 42) and 0 < vel <= 127, \
                f"{name}: drum {st},{gm}"
        if "skank" in src:
            for st, vel in src["skank"]:
                assert 0 <= st < n and 0 < vel <= 127, f"{name}: skank {st}"


if __name__ == "__main__":
    check()
    print(f"baroquedub_riddims_data: {len(RIDDIMS)} riddims, ok")
