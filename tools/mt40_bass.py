#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MT-40X v0.5: the bank's two calibration moves, written into riddims.json once.
#
#   python3 tools/mt40_bass.py            riddims.json  mt40x-riddims-1 -> mt40x-riddims-2
#   python3 tools/mt40_bass.py --force    re-apply the articulation (hand bass edits are lost)
#
# 1. The melody track leaves the bank. The MT-40 plays bass and drums and the session
#    adds the skank; the melody was SLOOP Dub's toaster's voice, the one part the machine
#    never had. T1 of the sequencer stays free for the user's own line.
#
# 2. The bass gets the machine's articulation: the MT-40's bass never holds -- it plucks
#    and re-strikes. The reference is the machine's own Rock pattern (the bank's mtstut
#    groove, the verified Noel Davey transcription: every sixteenth struck, a lighter
#    strike each time, the tails cut). The other grooves as converted from SLOOP Dub
#    strike once and hold (a rest is a TIE): the repeats are missing, the bass reads
#    sustained where the machine bounces. These rules bring every groove to the same
#    bounce without touching a degree, a root, a drum or a skank:
#
#      - a struck note that would hold is struck AGAIN on the next sixteenth, lighter
#        (the pluck, PLUCK_DROP under the strike's velocity);
#      - a strong strike that holds three sixteenths or more re-strikes once more on
#        the following eighth (RESTRIKE_DROP under the strike's velocity);
#      - nothing else moves: the skeleton's steps, degrees and velocities stay the
#        SLOOP Dub ones, the rests between the re-strikes stay TIEs, the grooves that
#        already re-strike or cut (mtstut, mtmarch) pass through untouched.
#
# After this script run tools/gen_mt40_riddims.py: mt40_riddims.h and the grids follow
# the JSON. verify_bank.py checks the result against the same rules.

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)

JSON_PATH = os.path.join(ROOT, "riddims.json")

STEPS = 64
BAR = 16
CUT = 253

PLUCK_DROP = 22        # the pluck's velocity under the strike's
RESTRIKE_DROP = 34     # the eighth re-strike's velocity under the strike's
PLUCK_FLOOR = 72       # the re-strikes never go under (the machine's ghosts stay heard)
RESTRIKE_FLOOR = 64


def articulate_bar(events):
    """one bar's bass events (sorted [step, deg, vel], deg CUT = the hold ends) with the
    machine's re-strikes added. The skeleton passes through; every added strike keeps the
    note it repeats."""
    out = {s: (d, v) for s, d, v in events}
    for s, (d, v) in sorted(out.items()):
        if d == CUT:
            continue
        nxt = next((e for e in sorted(out) if e > s), BAR)   # the hold ends there (or the bar does)
        gap = nxt - s
        if gap >= 2 and s + 1 < BAR and s + 1 not in out:
            out[s + 1] = (d, max(PLUCK_FLOOR, v - PLUCK_DROP))
        if gap >= 3 and s + 2 < BAR and s + 2 not in out:
            out[s + 2] = (d, max(RESTRIKE_FLOOR, v - RESTRIKE_DROP))
    return [[s, out[s][0], out[s][1]] for s in sorted(out)]


def articulate(bass):
    """the 64 steps' bass events, bar by bar (each bar carries its own root, the degrees
    are bar-relative)"""
    out = []
    for b in range(STEPS // BAR):
        bar = [e for e in bass if b * BAR <= e[0] < (b + 1) * BAR]
        out.extend(articulate_bar(bar))
    return out


def validate(r):
    """the same rules the generator applies: no step twice, a CUT only over a held note"""
    n = r["name"]
    seen = set()
    held = False
    for st, deg, vel in sorted(r["bass"]):
        assert 0 <= st < STEPS, f"{n}: a bass step {st}"
        if deg == CUT:
            assert held, f"{n}: a cut with nothing held (step {st})"
            held = False
        else:
            assert 0 <= deg <= 16, f"{n}: bass step {st}: degree {deg}"
            assert st not in seen, f"{n}: step {st} twice (bass)"
            seen.add(st)
            held = True
    assert "mel" not in r, f"{n}: the melody is still there"


def main():
    force = "--force" in sys.argv
    with open(JSON_PATH) as f:
        doc = json.load(f)
    if doc.get("format") != "mt40x-riddims-1" and not force:
        print(f"riddims.json is {doc.get('format')} (mt40x-riddims-2 already carries the v0.5 "
              f"bank); --force re-applies the articulation and loses hand bass edits")
        return
    assert doc.get("format") == "mt40x-riddims-1" or force, "wrong format"
    for r in doc["riddims"]:
        r.pop("mel", None)                    # the melody leaves the bank (the machine has none)
        r["bass"] = articulate(r["bass"])     # the machine's re-strikes
        validate(r)
    doc["format"] = "mt40x-riddims-2"
    doc["source"] = (doc["source"] + "; v0.5: the melody track removed (the MT-40's three parts: "
                     "drums, bass, skank) and the bass re-struck as the machine plucks "
                     "(tools/mt40_bass.py, the Rock pattern's articulation)")
    with open(JSON_PATH, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    hits = sum(len(r["bass"]) for r in doc["riddims"])
    print(f"riddims.json: 28 riddims, no melody, {hits} bass strikes "
          f"(the machine's pluck on every hold) -- format mt40x-riddims-2")
    print("next: python3 tools/gen_mt40_riddims.py")


if __name__ == "__main__":
    main()
