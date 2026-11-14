#!/usr/bin/env python3
# Verify: our mt40_riddims.h / riddims.json carry the v0.5 bank -- the SLOOP Dub 1.6 bank
# with the two v0.5 moves (tools/mt40_bass.py): the melody track removed (the MT-40's
# three parts) and the bass re-struck as the machine plucks it. The reference for the
# untouched fields is the vendored generator (sloop_riddims_data.py, the same code that
# generated upstream's dub_riddim.h): every drum lane and velocity, every skank velocity,
# every root and chord must match, riddim by riddim; the bass must be the SLOOP line with
# exactly the machine's re-strikes (a hand calibration afterwards is the JSON's right --
# verify_bank reports it as a drift, the .h check stays strict).
import json
import os
import sys

HERE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(HERE, "tools"))
import sloop_riddims_data as S
import mt40_bass as M

LANE_OF_GM = {36: 0, 38: 1, 40: 1, 39: 2, 42: 3, 46: 4, 45: 5, 47: 5, 37: 6, 56: 7}

doc = json.load(open(os.path.join(HERE, "riddims.json")))
jb = {r["name"]: r for r in doc["riddims"]}
assert doc.get("format") == "mt40x-riddims-2", "riddims.json: the v0.5 format expected"
assert len(doc["riddims"]) == 28, len(doc["riddims"])
bad = drift = 0
for defn in S.RIDDIM_DEFS:
    name, roots = defn[0], list(defn[1])
    chords = list(defn[5]) if len(defn) > 5 else roots
    sn, sv = S.build_riddim(defn)
    r = jb[name]
    fail = []
    if "mel" in r:
        fail.append("the melody is still there (v0.5 dropped it)")
    if r["roots"] != roots or r["chords"] != chords:
        fail.append(f"roots {r['roots']} != {roots} or chords {r['chords']} != {chords}")
    # drums
    want = [(st, LANE_OF_GM[sn[3][st]], sv[3][st]) for st in range(64) if sn[3][st] != S.REST]
    if sorted(map(tuple, r["drums"])) != sorted(want):
        fail.append("drums")
    # skank velocities
    want = [(st, sv[1][st]) for st in range(64) if sv[1][st]]
    if sorted(map(tuple, r["skank"])) != sorted(want):
        fail.append("skank")
    # bass: the SLOOP line with the machine's re-strikes (mt40_bass.articulate)
    want = []
    for st in range(64):
        n = sn[0][st]
        if n == S.CUT:
            want.append([st, S.CUT, 0])
        elif n != S.REST:
            want.append([st, n - roots[st // 16], sv[0][st]])
    if sorted(map(tuple, r["bass"])) != sorted(map(tuple, M.articulate(want))):
        print(f"{name}: bass differs from the v0.5 articulation "
              f"(a hand calibration? the JSON is the source either way)")
        drift += 1
    M.validate(r)                                     # the structural rules, always
    if fail:
        print(f"{name}: " + "; ".join(fail))
        bad += 1

# and the .h must carry the same arrays as the JSON (the firmware's copy)
h = open(os.path.join(HERE, "firmware", "src", "mt40_riddims.h")).read()
body = h.split("static const mt40_riddim_t MT40_RIDDIM_DATA[] = {", 1)[1]
import re
for m in re.finditer(r'\{"([^"]+)", (\d+), \{([^}]*)\}, \{([^}]*)\}, (\d+),\s*\{([^}]*)\},\s*\{([^}]*)\},\s*\{([^}]*)\},\s*\{([^}]*)\},\s*\{([^}]*)\}\},', body, re.S):
    g = m.groups()
    name = g[0]
    r = jb[name]
    arr = lambda s: [int(x) for x in s.split(",")]
    if int(g[1]) != r["bpm"] or arr(g[2]) != r["roots"] or arr(g[3]) != r["chords"] or int(g[4]) != r["skank_qual"]:
        print(f"{name}: header/JSON mismatch (bpm, roots, chords, qual)"); bad += 1
    lane, vel = arr(g[5]), arr(g[6])
    want_lane = [S.REST] * 64
    want_vel = [0] * 64
    for st, ln, v in r["drums"]:
        want_lane[st], want_vel[st] = ln, v
    if lane != want_lane or vel != want_vel:
        print(f"{name}: .h drums != JSON"); bad += 1
    deg, vel = arr(g[7]), arr(g[8])
    want_deg = [S.REST] * 64
    want_vel = [0] * 64
    for st, d, v in r["bass"]:
        want_deg[st], want_vel[st] = d, v
    if deg != want_deg or vel != want_vel:
        print(f"{name}: .h bass != JSON"); bad += 1
    skv = arr(g[9])
    want = [0] * 64
    for st, v in r["skank"]:
        want[st] = v
    if skv != want:
        print(f"{name}: .h skank != JSON"); bad += 1
if "mel_note" in h or "mel_vel" in h:
    print(".h: the melody arrays are still in the struct data"); bad += 1

print(f"28 riddims checked: the SLOOP Dub 1.6 bank (drums, skank, roots, chords), the bass "
      f"re-struck as the machine plucks, no melody; failures: {bad}, hand-calibrated bass: {drift}")
sys.exit(1 if bad else 0)
