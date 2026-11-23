# Licensing

Tinsel is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). You may use, study, change and share it. If you give someone a firmware built from it, give them its
complete source under the same licence too.

## Who made what

| | |
| --- | --- |
| Tinsel: `firmware/src/tinsel.c`, `firmware/src/tn_*.h`, `music/`, `tools/gen_music.py`, `tools/bench.sh`, `tools/check.sh`, `tests/tn_test.c`, `tests/scripts/tinsel.txt`, `web/advent.html`, the per-LED levels in `firmware/hal/fm1_input.h` (`FM1_LED_LEVELS`), Tinsel's store window in `firmware/src/store.h`, the docs | Copyright (C) 2026 Joel Merrick |
| The platform, the host mode, the tests and the tools Tinsel builds on: [Dinghy](https://github.com/zvenson/dinghy) 0.5 | zvenson, and the people below |
| The platform: `firmware/hal/`, `firmware/loader/`, `crt0.S`, `app.ld`, `firmware/src/` (`usb.c`, `ota.c`, `recovery.c`, `lcd.c`, `gfx.c`, `libc.c`, `bootguard.h`, `uac_fir.h`), `tools/` | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca)), with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89) |
| The SMK-37 board layer (`firmware/hal/smk_input.h`, the SMK-37 parts of the HAL, loader and tools) | Copyright (C) 2026 M. Geisler (genkigenki/makkuro), from [genkigenki/sloop-fm1](https://github.com/genkigenki/sloop-fm1) branch `smk37` |
| The TRS MIDI IN `firmware/src/midi_uart.c` | Felucca's, as sloopDX carries it, with the ring read by content from Felucca [Salt] 0.9-salt14 by ChanceTheMaker (found by keremimo) |
| The update loader's screen: `firmware/loader/ldr_screen.c`, `tests/ldr_screen_test.c` | Copyright (C) 2026 zvenson (from OberVave); the idea from [Skiff](https://github.com/illuminus5/skiff) by illuminus5, GPL-3.0 |
| From OberVave: the storage (blobs, slots), `LCD_IDLE`, the SMK-37 wiring of `platform.c`, `store.h`, `tests/smk_flash_test.c`, `tests/run_tests.sh`, the release workflow | Copyright (C) 2026 zvenson |
| The macOS host-build fix in `gfx.c`, the shape of `tools/check.sh` and `tools/bench.sh` (the emulator checks) | from Melisma (Joel Merrick), GPL-3.0 |

## Ideas and numbers from elsewhere

Nothing below is copied code; these are the sources of an idea, a method or a constant, credited with thanks.

| What | Where from |
| --- | --- |
| The sleigh bells: Perry Cook's PhISEM model, with the "Sleigh Bells" constants (32 jingles; resonators at 2500, 5300, 6500, 8300, 9800 Hz; the decays) | [The Synthesis ToolKit](https://github.com/thestk/stk) (`Shakers.cpp`), Perry R. Cook and Gary P. Scavone, under STK's own permissive licence |
| The bell, celesta, music-box and glockenspiel partials | published acoustics (church-bell partials as bell founders tune them; free bar and cantilever modes); docs/RESEARCH-NOTES.md section 5 cites them |
| The feedback-delay-network reverb | the standard design (Jot; Schroeder), with lengths chosen for Tinsel |
| Per-LED brightness by frames | felucca-bsp's `fm1_led_anim.h` (Hügelton Instruments, MPL-2.0) did it first; Tinsel's is written for Dinghy's tick |
| The sine | Dinghy's refined parabola; the pitch table is Dinghy's |
| Teaching by lit keys | Casio's Step Up Lesson and Yamaha's Education Suite (the idea of a mode that waits for the right key) |
| What other FM-1 firmwares already do (so Tinsel does something else) | FM-1 Arcade, RetroDash, FM-1 Midi-Mania, Kris Jangle, GNOMNOM, Boardwalk, Mighty Arcade: docs/RESEARCH-NOTES.md section 4 |

## The music

The carols in `music/` are melodies in the public domain (composed before 1930 by people who died more than 70 years
ago, or traditional), transcribed for Tinsel; the transcriptions are GPL-3.0 like the rest. No lyrics are
included. `music/README.md` and docs/DESIGN.md section 11 say which songs are kept out and why.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Terminus font 8x16 (ter-u16n), rendered into `firmware/gen/felucca_font.h` | SIL OFL 1.1 | `assets/fonts/Terminus-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments; "M-VAVE", "FM-1", "SMK-37" and "MKE-P37"
belong to their owners. Tinsel is not made or endorsed by them.
