#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Joel Merrick
"""Spoken call-outs for a private Tinsel build (DESIGN section 9.4): private/voices/*.wav -> firmware/gen/tn_voice.h.

  tools/make_voices.py              make the clips that are missing with ElevenLabs text-to-speech, then the header
  tools/make_voices.py --no-fetch   only the header, from the WAVs already in private/voices/ (your own recordings:
                                    ring.wav, twist.wav, jingle.wav, shh.wav, welldone.wav, hohoho.wav, merry.wav,
                                    hooray.wav; any rate, mono or stereo)
  DINGHY_DEFS=-DTN_VOICE ./build.sh build a package with them (for your own units only)

The ElevenLabs key is read from the macOS keychain (service elevenlabs-api-key), never from a file. The clips and the
header are gitignored: generated voices may not be shared under every plan, so they never go into the repo.
"""
import array
import json
import os
import subprocess
import sys
import urllib.request
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIR = ROOT / "private/voices"
OUT = ROOT / "firmware/gen/tn_voice.h"
RATE = 11025
VOICE_ID = os.environ.get("TN_VOICE_ID", "JBFqnCBsd6RMkjVDRZzb")   # a premade ElevenLabs voice
PHRASES = [("ring", "Ring it!"), ("twist", "Twist it!"), ("jingle", "Jingle it!"), ("shh", "Shhh..."),
           ("welldone", "Well done!"), ("hohoho", "Ho ho ho!"), ("merry", "Merry Christmas!"), ("hooray", "Hooray!")]


def key():
    r = subprocess.run(["security", "find-generic-password", "-a", os.environ["USER"], "-s", "elevenlabs-api-key", "-w"],
                       capture_output=True, text=True)
    if r.returncode:
        sys.exit("make_voices: no ElevenLabs key in the keychain (service elevenlabs-api-key)")
    return r.stdout.strip()


def fetch(name, text, k):
    mp3 = DIR / f"{name}.mp3"
    req = urllib.request.Request(
        f"https://api.elevenlabs.io/v1/text-to-speech/{VOICE_ID}?output_format=mp3_22050_32",
        data=json.dumps({"text": text, "model_id": "eleven_multilingual_v2",
                         "voice_settings": {"stability": 0.4, "similarity_boost": 0.8, "style": 0.6}}).encode(),
        headers={"xi-api-key": k, "Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=60) as r:
        mp3.write_bytes(r.read())
    subprocess.run(["afconvert", "-f", "WAVE", "-d", "LEI16@22050", "-c", "1", str(mp3), str(DIR / f"{name}.wav")], check=True)
    print(f"  {name}: {text}")


def pcm8(path):
    """the clip as signed 8-bit at RATE, trimmed of silence, peak at 120"""
    tmp = DIR / f".{path.stem}.tmp.wav"
    subprocess.run(["afconvert", "-f", "WAVE", "-d", f"LEI16@{RATE}", "-c", "1", str(path), str(tmp)], check=True)
    with wave.open(str(tmp)) as w:
        a = array.array("h", w.readframes(w.getnframes()))
    tmp.unlink()
    peak = max(1, max(abs(x) for x in a))
    lo = next((i for i, x in enumerate(a) if abs(x) > peak // 30), 0)
    hi = len(a) - next((i for i, x in enumerate(reversed(a)) if abs(x) > peak // 30), 0)
    a = a[lo:hi][: RATE * 2]                    # (two seconds at most)
    return [max(-127, min(127, x * 120 // peak)) for x in a]


def main():
    DIR.mkdir(parents=True, exist_ok=True)
    if "--no-fetch" not in sys.argv:
        k = None
        for name, text in PHRASES:
            if not (DIR / f"{name}.wav").exists():
                k = k or key()
                fetch(name, text, k)
    clips = []
    for name, _ in PHRASES:
        f = DIR / f"{name}.wav"
        clips.append((name, pcm8(f) if f.exists() else []))
    lines = ["/* Tinsel's spoken call-outs, for a private build only. Made by tools/make_voices.py: not in the repo. */",
             "#ifndef TN_VOICE_H", "#define TN_VOICE_H", f"#define TN_VOICE_RATE {RATE}u"]
    for name, s in clips:
        lines.append(f"static const int8_t TNV_{name.upper()}[{max(1, len(s))}] = {{{', '.join(map(str, s)) or '0'}}};")
    lines.append("static const struct { const int8_t *pcm; uint32_t n; } TNV_CLIPS[] = {")
    lines += [f"    {{TNV_{name.upper()}, {len(s)}u}}," for name, s in clips]
    lines += ["};", "#endif", ""]
    OUT.write_text("\n".join(lines))
    print(f"{OUT.relative_to(ROOT)}: {sum(1 for _, s in clips if s)} clips, {sum(len(s) for _, s in clips)} B")


if __name__ == "__main__":
    main()
