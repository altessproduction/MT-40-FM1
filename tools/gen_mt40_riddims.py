#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MT-40X: converts the riddim bank into the firmware's data.
#
#   python3 tools/gen_mt40_riddims.py --bootstrap   riddims.json from SLOOP Dub 1.6 (tools/sloop_riddims_data.py),
#                                                   the v0.5 shape (no melody, the bass re-struck),
#                                                   then mt40_riddims.h + riddims_grids.md
#   python3 tools/gen_mt40_riddims.py               mt40_riddims.h + riddims_grids.md from the riddims.json
#                                                   as it stands (the calibration round-trip: edit the JSON,
#                                                   run this again)
#
# riddims.json is the one editable source: the calibration workflow of CALIBRATION.md is
# "compare the grids of riddims_grids.md with the real MT-40, correct riddims.json, regenerate".
# The .h is committed: the firmware builds need no Python.

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import sloop_riddims_data as S   # noqa: E402  (the vendored SLOOP Dub 1.6 bank, GPL-3.0-only)
import mt40_bass as M            # noqa: E402  (the v0.5 articulation: no melody, the bass re-struck)

STEPS = 64
REST = 255          # MT40_REST in mt40.h
CUT = 253           # MT40_CUT in mt40.h: the held note ends there

# Felucca's DRUM engine lanes (eng_drum.c DRUM_LANE_NOTE = {36, 38, 39, 42, 46, 45, 37, 56}):
# 0 BD kick, 1 SD snare, 2 CP clap, 3 CH hat, 4 OH open, 5 TM tom, 6 RS rim, 7 CB cowbell.
# SLOOP Dub's riddims hit GM 36 kick, 37 rim, 38 snare, 39 clap, 40 second snare, 42 hat,
# 46 open hat; the second snare (40) falls to the snare lane, as a lighter one (its velocity
# is kept, and the DRUM engine scales by velocity).
LANE_OF_GM = {36: 0, 38: 1, 40: 1, 39: 2, 42: 3, 46: 4, 45: 5, 47: 5, 37: 6, 56: 7}
LANE_GLYPH = "KSPHOTRC"   # a grid's glyph per lane: K kick, S snare, P clap, H hat, O open, T tom, R rim, C cowbell

# The BPMs of SLOOP Dub 1.6 (their gen_riddims.py's comment, "BPMs for the docs").
SLOOP_BPM = {
    "ONE DROP": 72, "STEPPERS": 76, "ROCKERS": 80, "DUBWALK": 70, "MEDITATE": 66,
    "DUB CUT": 74, "ZION": 68, "SKA": 88, "HEAVENLESS": 80, "TONIGHT": 84,
    "GET IN GROOVE": 88, "HOT MILK": 92, "ANSWER": 82, "PROMISED LAND": 78,
    "GUNMAN": 78, "HERE I COME": 95, "SLENG TENG": 82, "TEMPO": 88, "PEANY PEANY": 92,
    "DANCEHALL": 95, "DEMBOW": 100, "JUNGLE": 172, "DUBSTEP": 140,
    "MT40 POPS": 96, "MT40 SLOWRK": 66, "MT40 SWING": 84, "MT40 WALTZ": 90, "MT40 SAMBA": 100,
}

SOURCE = ("SLOOP Dub 1.6 (https://github.com/altessproduction/sloop-dub-fm1, tag v1.6, "
          "tools/gen_riddims.py, GPL-3.0-only) converted by tools/gen_mt40_riddims.py")
JSON_PATH = os.path.join(ROOT, "riddims.json")
H_PATH = os.path.join(ROOT, "firmware", "src", "mt40_riddims.h")
MD_PATH = os.path.join(ROOT, "riddims_grids.md")


def sloop_to_json():
    """the vendored bank -> riddims.json (the editable, calibratable form), the v0.5 shape:
    the machine's three parts (the melody was SLOOP Dub's toaster's voice, the machine has
    none) and the bass as the machine plucks it (mt40_bass.py's re-strikes)"""
    S.check_bank()                       # the upstream bank's own consistency checks
    riddims = []
    for defn in S.RIDDIM_DEFS:
        name, roots = defn[0], list(defn[1])
        chords = list(defn[5]) if len(defn) > 5 else roots
        notes, vels = S.build_riddim(defn)
        drums, bass, skank = [], [], []
        for st in range(STEPS):
            bar = st // 16
            n = notes[3][st]
            if n != S.REST:
                drums.append([st, LANE_OF_GM[n], vels[3][st]])
            n = notes[0][st]
            if n == S.CUT:
                bass.append([st, CUT, 0])
            elif n != S.REST:
                deg = n - roots[bar]
                assert -12 <= deg <= 16, f"{name}: bass step {st}: degree {deg}"
                bass.append([st, deg, vels[0][st]])
            if vels[1][st]:
                skank.append([st, vels[1][st]])
        riddims.append({
            "name": name,
            "bpm": SLOOP_BPM[name],
            "roots": roots,               # the four bars' bass roots as authored (MIDI)
            "chords": chords,             # the skank stabs' chord roots per bar (MIDI, pitch class used)
            "skank_qual": 1,              # the stabs' chord as authored: 0 MAJ, 1 MIN, 2 DOM7
                                          # (SLOOP Dub's dub_skank_conf default: A MIN)
            "drums": drums,               # [step, lane, velocity]
            "bass": M.articulate(bass),   # [step, semitones over the bar's root (253 = the held note
                                          # ends), velocity], the machine's re-strikes added
            "skank": skank,               # [step, velocity]; the chord's root is the bar's
        })
    return {
        "format": "mt40x-riddims-2",
        "source": SOURCE,
        "lanes": {"BD": 0, "SD": 1, "CP": 2, "CH": 3, "OH": 4, "TM": 5, "RS": 6, "CB": 7},
        "qualities": ["MAJ", "MIN", "DOM7"],
        "riddims": riddims,
    }


def validate(doc):
    names = set()
    for r in doc["riddims"]:
        n = r["name"]
        assert n not in names, f"{n}: twice"
        names.add(n)
        assert 40 <= r["bpm"] <= 240, f"{n}: bpm"
        assert len(r["roots"]) == 4 and len(r["chords"]) == 4, f"{n}: roots / chords"
        for x in r["roots"] + r["chords"]:
            assert 28 <= x <= 50, f"{n}: a root out of the bass' range"
        assert r["skank_qual"] in (0, 1, 2), f"{n}: skank_qual"
        seen = set()
        for st, lane, vel in r["drums"]:
            assert 0 <= st < STEPS and lane in range(8) and 0 < vel <= 127, f"{n}: a drum hit {st},{lane}"
            assert st not in seen, f"{n}: step {st} hit twice (drums)"
            seen.add(st)
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
        for st, vel in r["skank"]:
            assert 0 <= st < STEPS and 0 < vel <= 127, f"{n}: a skank hit"
        assert "mel" not in r, f"{n}: the melody left the bank (v0.5, the MT-40's three parts)"


def expand_bass(r):
    """the 64 steps of the bass: (deg, vel, kind) per step, kind 0 rest, 1 note, 2 cut"""
    out = [(0, 0, 0)] * STEPS
    for st, deg, vel in r["bass"]:
        out[st] = (deg, vel, 2 if deg == CUT else 1)
    return out


# the MAJ / DOM7 variants of a groove, derived (CALIBRATION.md: they are the ones to check
# against the real MT-40): MAJ raises the minor third to the major; DOM7 is MAJ with the
# bar's LAST fifth sung flat, the turnaround the dominant lives on.
def groove_variant(bar_degs, qual):
    if qual == 1:                                   # MIN: as written
        return list(bar_degs)
    out = [4 if d == 3 else d for d in bar_degs]    # MAJ: 3 -> 4
    if qual == 2:                                   # DOM7: the last 7 of the bar -> 10
        for i in range(len(out) - 1, -1, -1):
            if out[i] == 7:
                out[i] = 10
                break
    return out


def emit_h(doc):
    rows = []
    rows.append("/* SPDX-License-Identifier: GPL-3.0-only")
    rows.append(" * Generated by tools/gen_mt40_riddims.py from riddims.json -- edit the JSON and run")
    rows.append(" * the generator again. The bank: the twenty-eight riddims of SLOOP Dub 1.6")
    rows.append(" * (altessproduction, https://github.com/altessproduction/sloop-dub-fm1, GPL-3.0-only),")
    rows.append(" * reggae, dub, roots, ska, the Studio One and early-digital classics, dancehall,")
    rows.append(" * UK bass music and the Casio MT-40's six auto-rhythms -- the machine's three parts")
    rows.append(" * (drums, bass, skank; v0.5 dropped the melody, T1 of the sequencer stays free). Four")
    rows.append(" * bars of sixteenths; the bass rides the bar's root (the MT-40 keyboard re-roots it)")
    rows.append(" * and never holds: every note re-strikes as the machine plucks (tools/mt40_bass.py,")
    rows.append(" * the Rock pattern's articulation), a rest HOLDS the re-strike (written as a TIE by")
    rows.append(" * mt40.c) and MT40_CUT (253) ends the held note. */")
    rows.append("#ifndef FELUCCA_MT40_RIDDIMS_H")
    rows.append("#define FELUCCA_MT40_RIDDIMS_H")
    rows.append('#include "mt40.h"')
    rows.append("")
    rows.append("static const mt40_riddim_t MT40_RIDDIM_DATA[] = {")
    for r in doc["riddims"]:
        drums = [REST] * STEPS
        dvel = [0] * STEPS
        for st, lane, vel in r["drums"]:
            drums[st] = lane
            dvel[st] = vel
        bass = expand_bass(r)
        bdeg = [b[0] if b[2] else REST for b in bass]
        bvel = [b[1] for b in bass]
        skv = [0] * STEPS
        for st, vel in r["skank"]:
            skv[st] = vel

        def arr(v):
            return "{" + ", ".join(str(x) for x in v) + "}"
        rows.append(f'    {{"{r["name"]}", {r["bpm"]}, {arr(r["roots"])}, {arr(r["chords"])}, {r["skank_qual"]},')
        rows.append(f"        {arr(drums)},")
        rows.append(f"        {arr(dvel)},")
        rows.append(f"        {arr(bdeg)},")
        rows.append(f"        {arr(bvel)},")
        rows.append(f"        {arr(skv)}}},")
    rows.append("};")
    rows.append("")
    rows.append("/* pointers, not a struct array by value: an array of const structs is no constant")
    rows.append(" * expression for a static initializer on this compiler (the MT-40X build learned it) */")
    rows.append("const mt40_riddim_t *const MT40_RIDDIMS[] = {")
    for i, r in enumerate(doc["riddims"]):
        rows.append(f"    &MT40_RIDDIM_DATA[{i}u],")
    rows.append("};")
    rows.append("")
    rows.append(f"const uint32_t MT40_RIDDIM_N = {len(doc['riddims'])}u;")
    rows.append("")
    rows.append("#endif")
    return "\n".join(rows) + "\n"


def grid_line(hits, width, glyph):
    """the hits of a track as one line of cells, | every beat, + every bar"""
    line = ""
    for st in range(width):
        line += glyph(hits[st]) if glyph else ("x" if hits[st] else ".")
        line += "|" if st % 4 == 3 else ""
        if st % 16 == 15:
            line = line[:-1] + "+"
    return line


def emit_md(doc):
    rows = []
    rows.append("# MT-40X — riddim grids (calibration sheet)")
    rows.append("")
    rows.append("Generated by `tools/gen_mt40_riddims.py` from `riddims.json` — the printable form of the bank")
    rows.append("(source: SLOOP Dub 1.6, altessproduction, GPL-3.0-only). The calibration workflow:")
    rows.append("compare with the real MT-40, correct `riddims.json`, regenerate (`python3 tools/gen_mt40_riddims.py`).")
    rows.append("")
    rows.append("One cell = a sixteenth, four bars of 16 (the `+` closes a bar). The drums: one hit a step")
    rows.append("at the most, the glyph is the lane (**K**ick, **S**nare, cla**P**, **H**at, **O**pen hat, **T**om, **R**im, **C**owbell,")
    rows.append("`.` = nothing). The bass: the interval over the bar's root (`1` the root, `b3`, `4`, `5`, `b7`, `6`,")
    rows.append("`8` the octave... `.` a rest that HOLDS, `x` the step where the held note ends (MT40_CUT));")
    rows.append("the bank v0.5 re-strikes every hold as the machine plucks (tools/mt40_bass.py): the same")
    rows.append("interval written on the next cells is the note struck again, the glyph carries no velocity.")
    rows.append("")
    NOTE_N = ["1", "b2", "2", "b3", "3", "4", "b5", "5", "b6", "6", "b7", "7", "8", "b9", "9", "#9", "10"]
    for r in doc["riddims"]:
        rows.append(f"## {r['name']}")
        rows.append("")
        roots = ", ".join(["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"][x % 12] +
                          str(x // 12 - 1) for x in r["roots"])
        chords = ", ".join(["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"][x % 12] +
                           str(x // 12 - 1) for x in r["chords"])
        rows.append(f"BPM {r['bpm']} - roots (as written): {roots} - skank chords: {chords} - "
                    f"chord: {['MAJ', 'MIN', 'DOM7'][r['skank_qual']]}")
        rows.append("")
        drums = [None] * STEPS
        for st, lane, _vel in r["drums"]:
            drums[st] = lane
        rows.append("```")
        rows.append("DRUM " + grid_line(drums, STEPS, lambda l: LANE_GLYPH[l] if l is not None else "."))
        bass = expand_bass(r)
        bglyph = lambda b: "." if b[2] == 0 else ("x" if b[2] == 2 else NOTE_N[b[0]])
        rows.append("BASS " + grid_line(bass, STEPS, bglyph))
        skv = [0] * STEPS
        for st, vel in r["skank"]:
            skv[st] = vel
        rows.append("SKNK " + grid_line(skv, STEPS, None))
        rows.append("```")
        rows.append("")
    return "\n".join(rows) + "\n"


def main():
    bootstrap = "--bootstrap" in sys.argv
    if bootstrap or not os.path.exists(JSON_PATH):
        doc = sloop_to_json()
        with open(JSON_PATH, "w") as f:
            json.dump(doc, f, indent=1)
            f.write("\n")
        print(f"riddims.json written: {len(doc['riddims'])} riddims (from SLOOP Dub 1.6)")
    else:
        with open(JSON_PATH) as f:
            doc = json.load(f)
        assert doc.get("format") == "mt40x-riddims-2", "riddims.json: wrong format (mt40x-riddims-2, the v0.5 bank)"
    validate(doc)
    with open(H_PATH, "w") as f:
        f.write(emit_h(doc))
    with open(MD_PATH, "w") as f:
        f.write(emit_md(doc))
    print(f"{H_PATH} and {MD_PATH} written: {len(doc['riddims'])} riddims")
    if bootstrap:
        print("next: check riddims.json (the calibration form), the .h and the grids follow it")


if __name__ == "__main__":
    main()
