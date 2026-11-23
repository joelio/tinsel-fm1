#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Tinsel's heaviest moment in fm1-emu: every key at once, PLAY's fanfare, a sparkle, the lights chasing, the robin
# flying. A test build (TN_BENCH) leaves the audio interrupt's times in RAM; this prints them.
#   tools/bench.sh [MHZ]        default 240 (the emulator's; the unit runs 360 or 320)
set -e
cd "$(dirname "$0")/.."
PY="${PYTHON:-python3}"; [ -x .venv/bin/python ] && PY="$PWD/.venv/bin/python"
EMU="$(ls -d .tools/fm1-emu/*/fm1-live 2>/dev/null | head -1)"
[ -n "$EMU" ] || { echo "bench: no fm1-emu in .tools/"; exit 0; }
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$(cd .jieli/toolchain && pwd -P)}" AC79_SDK="${AC79_SDK:-$(cd .sdk && pwd -P)}" PYTHON="$PY"
DINGHY_DEFS=-DTN_BENCH PATH="$(dirname "$PY"):$PATH" ./build.sh >build/bench-build.log 2>&1 || { tail -5 build/bench-build.log; exit 1; }
mkdir -p build/emu && cp build/dinghy.fwsc build/emu/bench.fwsc
K=""; t=3500
for n in F3 F#3 G3 G#3 A3 A#3 B3 C4 C#4 D4 D#4 E4 F4 F#4 G4 G#4 A4 A#4 B4 C5 C#5 D5 D#5 E5 F5 F#5 G5; do K="$K$t:$n@100,"; done
K="${K}3900:PLAY,4000:PLAY^,4100:ARP,4200:ARP^,4300:e2:+10,4500:e3:-8,5000:GLO,5100:GLO^,5400:F3^,5400:C4^,5400:G5^"
cd build/emu
FM1_DUMPRAM=bench-ram.bin FM1_BENCH_KEYS="$K" FM1_BENCH_WAV=bench.raw "../../$EMU" --fwsc bench.fwsc --flash-chip uc25hq80 --mhz "${1:-240}" --bench 9000 >bench.log 2>&1 || { tail -5 bench.log; exit 1; }
"$PY" - "${1:-240}" <<'PYEOF'
import struct, sys
d = open('bench-ram.bin', 'rb').read()
i = d.find(b'LOAD')
if i < 0 or d[i + 16:i + 20] != b'MARK': sys.exit("bench: the times were not found in the memory")
worst, total, n = struct.unpack('<III', d[i + 4:i + 16])
block = 256 * 1000000 // 44100 * 24
mean, worst = 100 * total / max(n, 1) / block, 100 * worst / block
print(f"heaviest: the audio interrupt takes {mean:.0f}% of a block, {worst:.0f}% at worst ({sys.argv[1]} MHz; {n} blocks)")
sys.exit(0 if n > 1000 and worst < 60 else 1)
PYEOF
