#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Joel Merrick
"""Tinsel's carols: music/*.txt -> firmware/gen/tn_music.h (music/README.md says how a file looks).

  tools/gen_music.py           write the header (checked in, so a build needs no Python)
  tools/gen_music.py --check   only check the files

It refuses a carol whose bars do not add up, whose chords do not cover the tune bar for bar, with no source line,
or that does not fit the 27 keys (F3..G5, MIDI 53..79) after moving it by whole octaves.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "firmware/gen/tn_music.h"
KEY_LO, KEY_HI = 53, 79
METERS = {"4/4": 16, "3/4": 12, "2/4": 8, "6/8": 12}
PICTURES = ["bells", "pudding", "holly", "star", "crown", "angel", "candle", "bell"]
PC = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
KINDS = {"": 0, "m": 1, "7": 2}
# the order in the Carol Book: the ones a small child may know first
ORDER = ["jingle-bells", "we-wish-you", "deck-the-halls", "silent-night", "good-king-wenceslas", "joy-to-the-world",
         "god-rest-ye", "shchedryk"]


def fail(path, msg):
    sys.exit(f"{path.name}: {msg}")


def pitch(path, tok):
    m = re.fullmatch(r"([A-G])([#b]?)(\d)", tok)
    if not m:
        fail(path, f"not a pitch: {tok}")
    return 12 * (int(m[3]) + 1) + PC[m[1]] + {"": 0, "#": 1, "b": -1}[m[2]]


def bars(path, text, parse):
    out, cur, all_ = [], [], []
    for tok in text.split():
        if tok == "|":
            out.append(cur)
            cur = []
            continue
        m = re.fullmatch(r"([^:]+):(\d+)", tok)
        if not m or int(m[2]) == 0:
            fail(path, f"not NAME:LEN: {tok}")
        item = (parse(path, m[1]), int(m[2]))
        cur.append(item)
        all_.append(item)
    if cur:
        out.append(cur)
    return out, all_


def chord(path, name):
    m = re.fullmatch(r"([A-G])([#b]?)(m|7|)", name)
    if not m:
        fail(path, f"not a chord: {name}")
    return ((PC[m[1]] + {"": 0, "#": 1, "b": -1}[m[2]]) % 12, KINDS[m[3]])


def load(path):
    fields, key = {}, None
    for line in path.read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        m = re.match(r"(\w+):\s*(.*)", line)
        if m and m[1] in ("title", "source", "meter", "tempo", "pickup", "picture", "jolly", "tune", "chords"):
            key = m[1]
            fields[key] = m[2]
        elif key in ("tune", "chords", "source"):
            fields[key] += " " + line.strip()
        else:
            fail(path, f"what is this line: {line}")
    for k in ("title", "source", "meter", "tempo", "pickup", "picture", "tune", "chords"):
        if not fields.get(k, "").strip():
            fail(path, f"no {k}:")
    if fields["meter"] not in METERS:
        fail(path, f"meter {fields['meter']}: one of {', '.join(METERS)}")
    if fields["picture"] not in PICTURES:
        fail(path, f"picture {fields['picture']}: one of {', '.join(PICTURES)}")
    bar, pick = METERS[fields["meter"]], int(fields["pickup"])
    tb, notes = bars(path, fields["tune"], lambda p, t: None if t == "r" else pitch(p, t))
    cb, chords = bars(path, fields["chords"], chord)
    if len(tb) != len(cb):
        fail(path, f"{len(tb)} bars of tune, {len(cb)} of chords")
    for i, (t, c) in enumerate(zip(tb, cb)):
        want = pick if (pick and i == 0) else (bar - pick if (pick and i == len(tb) - 1) else bar)
        lt, lc = sum(d for _, d in t), sum(d for _, d in c)
        if lt != want:
            fail(path, f"bar {i + 1}: the tune has {lt} sixteenths, the bar {want}")
        if lc != want:
            fail(path, f"bar {i + 1}: the chords have {lc} sixteenths, the bar {want}")
    ps = [p for p, _ in notes if p is not None]
    shift = 0
    while max(ps) + shift > KEY_HI:
        shift -= 12
    while min(ps) + shift < KEY_LO:
        shift += 12
    if max(ps) + shift > KEY_HI or min(ps) + shift < KEY_LO:
        fail(path, f"spans {min(ps)}..{max(ps)}: wider than the keys (MIDI {KEY_LO}..{KEY_HI})")
    return {"name": path.stem, "title": fields["title"], "source": fields["source"].strip(), "bar": bar,
            "tempo": int(fields["tempo"]), "six8": fields["meter"] == "6/8", "pickup": pick,
            "picture": PICTURES.index(fields["picture"]), "jolly": fields.get("jolly", "no").strip() == "yes",
            "notes": [(0 if p is None else p + shift, d) for p, d in notes], "chords": chords}


def main():
    files = sorted((ROOT / "music").glob("*.txt"))
    carols = [load(f) for f in files]
    carols.sort(key=lambda c: ORDER.index(c["name"]) if c["name"] in ORDER else 99)
    if "--check" in sys.argv:
        print(f"music: {len(carols)} carols check")
        return
    lines = ["/* SPDX-License-Identifier: GPL-3.0-only */",
             "/* Tinsel's carols. Made by tools/gen_music.py from the files in music/: change those, not this. */",
             "#ifndef TN_MUSIC_H", "#define TN_MUSIC_H",
             "typedef struct { const char *title; const uint8_t *pitch, *len, *chord; uint16_t n, nchord, tempo;",
             "                 uint8_t bar, pickup, picture, six8, jolly; } tn_carol_t;",
             "enum { CP_" + ", CP_".join(p.upper() for p in PICTURES) + " };"]
    for i, c in enumerate(carols):
        lines.append(f"/* {c['title']}: {c['source']} */")
        lines.append(f"static const uint8_t TNM_P{i}[] = {{{', '.join(str(p) for p, _ in c['notes'])}}};")
        lines.append(f"static const uint8_t TNM_L{i}[] = {{{', '.join(str(d) for _, d in c['notes'])}}};")
        lines.append(f"static const uint8_t TNM_C{i}[] = {{{', '.join(f'{r | k << 4}, {d}' for (r, k), d in c['chords'])}}};")
    lines.append("static const tn_carol_t TN_CAROLS[] = {")
    for i, c in enumerate(carols):
        title = c["title"].replace('"', '\\"')
        lines.append(f'    {{"{title}", TNM_P{i}, TNM_L{i}, TNM_C{i}, {len(c["notes"])}, {len(c["chords"])}, {c["tempo"]}, '
                     f'{c["bar"]}, {c["pickup"]}, {c["picture"]}, {int(c["six8"])}, {int(c["jolly"])}}},')
    lines += ["};", f"#define TN_NCAROLS {len(carols)}u", "#endif", ""]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines))
    print(f"{OUT.relative_to(ROOT)}: {len(carols)} carols, {sum(len(c['notes']) for c in carols)} notes")


if __name__ == "__main__":
    main()
