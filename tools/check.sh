#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Tinsel's checks, on this computer (no hosted CI on push).
#   tools/check.sh           the carols, the unit tests, a host run through every screen (a few seconds)
#   tools/check.sh --full    then the device build, the platform's tests, the heaviest sound in fm1-emu, a boot there
set -e
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"
PY="${PYTHON:-python3}"; [ -x .venv/bin/python ] && PY="$PWD/.venv/bin/python"
"$PY" -I tools/gen_music.py --check
"$PY" -I tools/gen_music.py >/dev/null
git diff --quiet -- firmware/gen/tn_music.h 2>/dev/null || echo "music    firmware/gen/tn_music.h regenerated: commit it with the carols"
cc -O1 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable -Ifirmware/hal -Ifirmware/src -Ifirmware/gen -o "$OUT/tn_test" tests/tn_test.c
"$OUT/tn_test" | tail -3
rm -f "$OUT"/dinghy-*.png "$OUT"/dinghy-*.ppm
PATH="$(dirname "$PY"):$PATH" sh tests/run_host.sh tests/scripts/tinsel.txt >"$OUT/host.log" 2>&1 || { tail -20 "$OUT/host.log"; exit 1; }
echo "host     $(grep -c 'screen ->' "$OUT/host.log") screens, $(grep 'sound' "$OUT/host.log" | sed 's/.*(\(.*\))/\1/')"
PATH="$(dirname "$PY"):$PATH" sh tests/run_host.sh tests/scripts/games.txt >"$OUT/games.log" 2>&1 || { tail -20 "$OUT/games.log"; exit 1; }
echo "games    $(grep -c 'screen ->' "$OUT/games.log") screens through every toy and game"
[ "$1" = "--full" ] || exit 0

export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$(cd .jieli/toolchain && pwd -P)}" AC79_SDK="${AC79_SDK:-$(cd .sdk && pwd -P)}" PYTHON="$PY"
PYTHON="$PY" tools/bench.sh
PATH="$(dirname "$PY"):$PATH" ./build.sh | grep -E 'ok |package|error'
PATH="$(dirname "$PY"):$PATH" sh tests/run_tests.sh >"$OUT/tests.log" 2>&1 || { grep -E 'FAIL|differs|error' "$OUT/tests.log" | head; exit 1; }
echo "platform $(tail -1 "$OUT/tests.log")"
EMU="$(ls -d .tools/fm1-emu/*/fm1-live 2>/dev/null | head -1)"
if [ -n "$EMU" ]; then                         # the real image, on the newer flash chip, at an FM-1's own pace
    mkdir -p build/emu/work && cd build/emu
    FM1_STRICT=1 FM1_BENCH_KEYS='2500:C4@100,2600:C4^,3000:e2:+6,3500:PLAY,3600:PLAY^,4000:HOME,4100:HOME^,4500:e0:+1,5000:PLAY,5100:PLAY^,6000:E4@100,6100:E4^' \
        FM1_BENCH_WAV=check.raw "../../$EMU" --fwsc ../dinghy.fwsc --flash-chip uc25hq80 --bench 8000 >check.log 2>&1 || { tail -5 check.log; exit 1; }
    "$PY" - <<'PYEOF'
import array, sys
a = array.array('h'); a.frombytes(open('check.raw', 'rb').read()); L = a[0::2]
rep = sum(1 for b in range(1, len(L) // 256) if L[b * 256:(b + 1) * 256] == L[(b - 1) * 256:b * 256] and any(L[b * 256:(b + 1) * 256]))
peak = max(abs(x) for x in a) if a else 0
print(f"emulator peak {peak}, {rep} audio blocks repeated (a late render)")
sys.exit(0 if peak > 500 and rep == 0 else 1)
PYEOF
else
    echo "emulator skipped: no fm1-emu in .tools/"
fi
