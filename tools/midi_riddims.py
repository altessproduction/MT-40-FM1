#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MT-40X v0.6: the five BaroqueDub riddims rebuilt from the MIDI transcriptions
# (tools/baroquedub_riddims_data.py -- where they come from and under which licence).
#
#   python3 tools/midi_riddims.py       merge the five into riddims.json (mt40x-riddims-2
#                                       -> mt40x-riddims-3), then run gen_mt40_riddims.py
#
# What the conversion does, riddim by riddim:
#
#   tempo     the BPM of the filtered loop (the recording's own tempo), the four bars
#             being the transcription's loop tiled over the bank's grid;
#   roots     the bass's own tonal centre (the harmony the line implies: GUNMAN's G,
#             HEAVENLESS's static Am vamp, SLENG TENG's C# pedal, TEMPO's Am, PEANY
#             PEANY's D dorian) -- the keyboard's re-rooting rides it;
#   bass      the transcription's notes as the machine plays them: the degrees over the
#             bar's root (negative ones, the sub-root walk of Heavenless's G E G G#, are
#             stored as negative integers and the .h encodes them, see mt40.h), a
#             MT40_CUT where the note ends before the next one (the quantized note-off,
#             skipped when it lands on the next onset or past the bar), then the
#             machine's re-strikes on top (tools/mt40_bass.py: every hold plucks again
#             on the next sixteenth) -- the transcription's own repeats pass through;
#   drums     the transcription's tracks merged onto the grid's one-velocity-per-step
#             form: the hits of a step keep their two structurally highest lanes (kick,
#             then snare, then the sidestick's rim) and share the louder velocity when
#             the two are within twenty of each other -- the kick and snare unisons of
#             the backbeats survive as the grid's double hit (drum_lane's two nibbles);
#             a lone hat under a much louder drum dies (it is masked in the original
#             too), a lone quieter drum keeps its own velocity;
#   skank     the stabs: tempo.mid carries a chord track and its stabs replace the
#             bank's; the four others have none -- the bank's session part stays (the
#             SLOOP Dub line), over the chord roots written in baroquedub_riddims_data.
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import baroquedub_riddims_data as BD      # noqa: E402
import mt40_bass as M                     # noqa: E402  (the machine's re-strikes)
import sloop_riddims_data as S            # noqa: E402  (the twenty-three's skeleton)

STEPS = 64
BAR = 16
CUT = M.CUT

LANE_OF_GM = {35: 0, 36: 0, 38: 1, 39: 2, 42: 3, 46: 4, 45: 5, 37: 6}
PRIORITY = [0, 1, 6, 2, 3, 4, 5, 7]       # kick, snare, rim, clap, hat, open, tom, bell
SHARE = 20                                # two hits share a step's velocity within this

JSON_PATH = os.path.join(ROOT, "riddims.json")


def tile(loop_events, loop_steps):
    """the transcription's loop tiled over the bank's sixty-four steps"""
    out = []
    for rep in range(STEPS // loop_steps):
        out.extend((rep * loop_steps + st, *rest) for st, *rest in loop_events)
    return out


def build_bass(src, roots4):
    """the notes as the machine plays them: strikes, the note-offs' cuts, the plucks"""
    loop = src["bars"] * BAR
    notes = []
    for st, pitch, vel, dur in tile(src["bass"], loop):
        b = st // BAR
        notes.append((st, pitch - roots4[b], vel, dur / 100.0))
    events = []
    for b in range(STEPS // BAR):
        bar = sorted(n for n in notes if b * BAR <= n[0] < (b + 1) * BAR)
        for i, (st, deg, vel, dur) in enumerate(bar):
            events.append([st, deg, vel])
            cut = st + math.ceil(dur)           # the note-off's step, the tail's end
            nxt = bar[i + 1][0] if i + 1 < len(bar) else (b + 1) * BAR
            if st + 1 <= cut < (b + 1) * BAR and cut < nxt:
                events.append([cut, CUT, 0])    # the note ends before the next one sounds
    return M.articulate(events)                # the machine's re-strikes on top


def build_drums(src):
    """the tracks onto the grid: two lanes at the most, one velocity for the step"""
    loop = src["bars"] * BAR
    hits = {}
    for st, gm, vel in tile(src["drums"], loop):
        hits.setdefault(st, {})
        lane = LANE_OF_GM[gm]
        hits[st][lane] = max(hits[st].get(lane, 0), vel)
    out = []
    for st in range(STEPS):
        if st not in hits:
            continue
        lanes = sorted(hits[st], key=lambda l: (PRIORITY.index(l), l))
        l1, v1 = lanes[0], hits[st][lanes[0]]
        if len(lanes) >= 2 and abs(hits[st][lanes[1]] - v1) <= SHARE:
            l2, v2 = lanes[1], hits[st][lanes[1]]
            out.append([st, l1, max(v1, v2), l2])    # the double hit, the louder velocity
        else:
            out.append([st, l1, v1])                 # the masked quieter hit dies
    return out


def build_skank(src):
    """tempo's chord stabs; None: the bank's session part stays"""
    if "skank" not in src:
        return None
    loop = src["bars"] * BAR
    return [[st, vel] for st, vel in tile(src["skank"], loop)]


def convert(name):
    src = BD.RIDDIMS[name]
    roots4 = (src["roots"] * 4)[:4]
    chords4 = (src["chords"] * 4)[:4]
    r = {
        "name": name,
        "bpm": src["bpm"],
        "roots": roots4,
        "chords": chords4,
        "skank_qual": src["skank_qual"],
        "drums": build_drums(src),
        "bass": build_bass(src, roots4),
    }
    skank = build_skank(src)
    if skank is not None:
        r["skank"] = skank
    return r


def main():
    with open(JSON_PATH) as f:
        doc = json.load(f)
    assert doc.get("format") == "mt40x-riddims-2", "riddims.json: the v0.5 bank expected"
    BD.check()
    names = {r["name"] for r in doc["riddims"]}
    missing = set(BD.RIDDIMS) - names
    assert not missing, f"riddims.json has no {sorted(missing)}"
    for r in doc["riddims"]:
        if r["name"] not in BD.RIDDIMS:
            # v0.6 also re-articulates the twenty-three: the v0.5 build only ever
            # plucked the first bar (mt40_bass.articulate fed absolute steps to a
            # bar-relative check), the whole bank gets the machine's re-strikes now.
            # The skeleton is the SLOOP Dub one, the same the v0.5 conversion used.
            defn = next(d for d in S.RIDDIM_DEFS if d[0] == r["name"])
            roots = list(defn[1])
            sn, sv = S.build_riddim(defn)
            skeleton = []
            for st in range(STEPS):
                n = sn[0][st]
                if n == S.CUT:
                    skeleton.append([st, S.CUT, 0])
                elif n != S.REST:
                    skeleton.append([st, n - roots[st // 16], sv[0][st]])
            r["bass"] = M.articulate(skeleton)
            M.validate(r)
            continue
        fresh = convert(r["name"])
        if "skank" not in fresh:               # no chord track: the session part stays
            fresh["skank"] = r["skank"]
        r.update(fresh)
        M.validate(r)
    doc["format"] = "mt40x-riddims-3"
    doc["source"] = (
        "SLOOP Dub 1.6 (https://github.com/altessproduction/sloop-dub-fm1, tag v1.6, "
        "tools/gen_riddims.py, GPL-3.0-only) converted by tools/gen_mt40_riddims.py; "
        "v0.5: the melody track removed and the bass re-struck as the machine plucks "
        "(tools/mt40_bass.py); v0.6: GUNMAN, HEAVENLESS, SLENG TENG, TEMPO and PEANY "
        "PEANY rebuilt from the BaroqueDub MIDI transcriptions "
        "(https://baroquedub.co.uk/archive/audio_riddims.html, CC-Sampling-1.0, "
        "tools/baroquedub_riddims_data.py -> tools/midi_riddims.py): the recording's own "
        "bass lines and drum takes, the skank staying the bank's session part; the whole "
        "bank re-articulated (the v0.5 build only ever plucked the first bar)"
    )
    with open(JSON_PATH, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    redone = [n for n in BD.RIDDIMS]
    strikes = sum(len(r["bass"]) for r in doc["riddims"] if r["name"] in BD.RIDDIMS)
    doubles = sum(1 for r in doc["riddims"] if r["name"] in BD.RIDDIMS
                  for ev in r["drums"] if len(ev) > 3)
    print(f"riddims.json: {', '.join(redone)} rebuilt from the BaroqueDub MIDIs "
          f"({strikes} bass strikes, {doubles} double drum hits) -- format mt40x-riddims-3")
    print("next: python3 tools/gen_mt40_riddims.py")


if __name__ == "__main__":
    main()
