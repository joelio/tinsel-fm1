# Research notes, 22 November 2026

Condensed from four parallel research runs (music and traditions, games, sound, hardware) for Tinsel. Tags are the
researchers' own: **[V]** checked against a source, **[M]** from memory and probably right, **[?]** unsure.
Nothing here has been tried on a real unit unless it says so.

## 1. Music: what we may ship

**The rules.** UK copyright lasts for the author's life plus 70 years. In the US, works published in 1930 or earlier
are public domain in 2026 [V: web.law.duke.edu/cspd/publicdomainday/2026]. In the UK a modern edition of an old work
can carry its own copyright (*Sawkins v Hyperion* [2005] EWCA Civ 565) [V]. So we transcribe melodies ourselves,
checked against pre-1930 prints, and ship only notes and titles.

| Melody | Source | Trap |
|---|---|---|
| Jingle Bells | Pierpont 1857 | none |
| We Wish You a Merry Christmas | West Country folk | Warrell's 1935 arrangement: use the bare tune |
| Silent Night | Gruber 1818 | none |
| O Holy Night | Adam 1847 | wide range |
| Shchedryk ("Carol of the Bells") | Leontovych 1916 [V: IMSLP] | Wilhousky's 1936 English lyric and arrangement are in copyright [V: CPDL]. Use "Shchedryk" |
| Nutcracker: March, Sugar Plum Fairy, Trepak, Waltz of the Flowers | Tchaikovsky 1892 | none |
| Deck the Halls | Welsh *Nos Galan* | none |
| God Rest Ye Merry, Gentlemen | traditional | none |
| Good King Wenceslas | *Tempus adest floridum*, Piae Cantiones 1582 | none |
| Hark the Herald | Mendelssohn 1840 / Cummings 1855 | Willcocks' descant is not |
| In the Bleak Midwinter (*Cranham*) | Holst 1906, died 1934 | Darke's setting is in copyright (he died 1976 [V]) |
| Coventry Carol, Ding Dong Merrily (Arbeau 1589), The Holly and the Ivy (Sharp 1911), O Tannenbaum, Joy to the World (*Antioch*, Mason 1839) | old | none |
| Away in a Manger | UK *Cradle Song* is Kirkpatrick 1895; US *Mueller* is Murray [V: Wikipedia] | none |
| Sussex Carol | Bramley & Stainer 1878 tune [V] | Vaughan Williams' 1919 version is UK-copyright until the end of 2028 |
| Twelve Days of Christmas | Austin 1909 [V] | Austin died 1952: UK public domain since 2023 |
| Wassail songs, O Come All Ye Faithful, The First Nowell, While Shepherds Watched, Once in Royal David's City, I Saw Three Ships, We Three Kings, Auld Lang Syne, Skaters' Waltz (1882) | old | O Little Town of Bethlehem: only the US tune *St Louis* (*Forest Green* is Vaughan Williams) |
| Corelli Christmas Concerto, Bach Christmas Oratorio, Handel's Hallelujah, Leopold Mozart's *Musical Sleigh Ride* | old | none |

**Avoid** (in copyright): Winter Wonderland, Santa Claus Is Comin' to Town, White Christmas, Have Yourself a Merry
Little Christmas, Let It Snow, The Christmas Song, Sleigh Ride, Rudolph, Frosty, Silver Bells, Little Drummer Boy,
Jingle Bell Rock, Rockin' Around the Christmas Tree, Little Donkey, Linus and Lucy, Rutter's carols, Merry Xmas
Everybody, I Wish It Could Be Christmas Everyday, Happy Xmas, Wonderful Christmastime, Stop the Cavalry, Walking in
the Air, Last Christmas, Do They Know It's Christmas, Fairytale of New York, Mistletoe and Wine, All I Want for
Christmas Is You, Feliz Navidad, Mary's Boy Child, Home Alone themes.

**Range.** Every carol above spans about 8 to 17 semitones [M]; the keys are F3..G5 (26 semitones), so all fit after
transposition. The build step checks each one.

**Sources of note data.**

| Source | Licence | Use? |
|---|---|---|
| Mutopia | per file: public domain, CC BY, CC BY-SA [V] | Public domain and CC BY yes; CC BY-SA 4.0 only (one-way compatible with GPLv3 [V]); never 3.0 or older |
| Open Hymnal Project (ABC) | "public domain", per file [V] | Yes, as a cross-check; verify each tune |
| The Session | ODbL plus a ban on processing with language models [V] | **No** |
| Nottingham Music Database | no licence [V] | No |
| CPDL | own licence or public domain per edition [V] | Public-domain editions only |
| IMSLP | scans | The place to check our transcriptions against old prints |

Decision: write each melody by hand in a small ABC-like text file, GPL-3.0, naming the print it was checked against.

## 2. Change ringing

Checked by a simulator during the research [V, by computation]. Bells strike once per row; between rows a bell moves
at most one place. Rounds: treble (highest) to tenor. Rows come in pairs, handstroke and backstroke, with a one-beat
gap before each handstroke. At peal speed a row of 7 or 8 bells is about 2 seconds. Named rows on 8: Queens 13572468,
Tittums 15263748, back rounds 87654321.

Place notation: `x` swaps every pair; a number lists the places that stay; `.` separates changes; `a,b` is `a`, then
`a` reversed without its last change, then `b`. Applying a change:

```
i = 0;
while (i < n) {
  if (stays(i+1)) i++;
  else if (i+1 < n && !stays(i+2)) { swap(i, i+1); i += 2; }
  else i++;
}
```

| Method | Place notation | Lead head | Plain course | Calls |
|---|---|---|---|---|
| Plain Hunt on 5 | `5.1` repeated | 12345 | 10 rows | none |
| Plain Hunt on 6 | `x16` repeated | 123456 | 12 rows | none |
| Plain Bob Doubles | `5.1.5.1.5,125` [V: complib.org/method/10550] | 13524 | 40 rows | Bob: last `125` becomes `145`; single `123` [M]. P,P,P,B three times = the 120-row extent [V] |
| Plain Bob Minor | `x16x16x16,12` [V] | 135264 | 60 rows | Bob `14`, single `1234` [M] |
| Grandsire Doubles | `3,1.5.1.5.1` [V] | 12534 | 30 rows | Bob: last two changes become `3.1`; single `3.123` [M]. P,B,P,B,P,S twice = all 120 rows [V] |

Christmas customs: Dewsbury's "Devil's Knell", one stroke per year since Christ's birth, ending at midnight on
Christmas Eve [V]. Handbell ringers hold two bells each [V].

## 3. Traditions with a game in them

Crackers (Tom Smith 1847, the snap from 1860 [V]): a two-player pull, a bang, a hat, a joke. Advent calendars. Panto
call and response. Wassailing (bang pots to wake the trees). Carol singers at the door. The King's Christmas Message
at 3 pm. Stir-up Sunday (22 November 2026) and the flaming pudding. Stockings, mince pies. The Twelve Days as a
cumulative memory game. St Nicholas and Krampus (major and minor). Music boxes. Sleigh bells.

**Avoid** (brands and characters): The Snowman, Coca-Cola truck, John Lewis and M&S adverts, Rudolph and Frosty as
characters, the Grinch, Elf, Home Alone, Love Actually, Nightmare Before Christmas, Peanuts, Wallace & Gromit, the
Gruffalo, Paddington, Doctor Who, Quality Street.

Prior delight elsewhere: the UM66 musical-card chip [V: righto.com 2021]; the PICO-8 and Playdate Advent releases (one
small thing a day) [V]; Game Boy Advent calendars and carol rhythm games [V]; 8-Bit Jesus (carols in the styles of
NES games) [V].

## 4. Games already on the FM-1

From the fm1-editor.com index (about 440 firmwares, 40 or so tagged games) and the repos, licences read from each
repo's LICENSE (GitHub's licence field is empty for all of them) [V]. 

| Firmware | What | Licence, platform | For us |
|---|---|---|---|
| FM-1 Arcade, `gaskb/fm1-snake` | 10 games (Snake, Tetris, Arkanoid, OutRun, Synth Hero, Frogger, Galaga, Pac-Man, Dino, Flappy) | GPL-3.0, **Dinghy** | Copy its Q8 motion, paddle, erase-and-redraw sprites, high-score record. Its RNG mixes `fm1_ms` into every call: not replayable |
| RetroDash / VaveHero, `nasonliu/retrodash` | rhythm game | GPL-3.0, **Dinghy** | 40-60 fps measured, median 49; audio peak 2.1 ms; froze when frames were timed off the wrapping 24 MHz counter; LED hint: next note bright, the one after dim and pulsing |
| FM-1 Midi-Mania | falling notes from MIDI files | GPL-3.0, Felucca | Judge windows PERFECT 30 / GREAT 60 / GOOD 110 ms, ±100 ms offset, tap calibration, a tested `judge.c` |
| Kris Jangle, `doohinkus/kris-jangle` | **carol call-and-response** (3 notes, then 3 more per level, four carols) | MPL-2.0, fm1-base | Exists: do not make "Simon with carols" |
| GNOMNOM | mushrooms fall in 7 lanes, key LEDs cue, each catch plays the next note | MPL-2.0, fm1-base | Exists: Present Catch must be clearly its own |
| Boardwalk | 18 music minigames, including Bells (change ringing, handbell pairs) | MPL-2.0, fm1-base | Ideas |
| Mighty Arcade | 18 "the game is the sequencer" games; a tested d-pad layout; EDIT shows controls | MPL-2.0, fm1-base | Ideas; EDIT-for-help copied as a convention |
| FM-1 Doom (zvenson, FM-1_980) | Doom at 240 × 150 | 8-bit frame expanded through `pal565[256]` per line while the previous line is sent |
| CHRISTMAS MACHINE | 36 carols on an organ | not on GitHub | Exists |

Not seen anywhere: two-player Pong on two knobs, a Bit.Trip-style paddle game on the beat, circular games on an
encoder, a Bop It-style game, a toddler toy. Teaching modes to learn from: Casio's Step Up (Listen, Watch with the
backing waiting, Remember) and Yamaha's Y.E.S. (Timing, Waiting, Minus One) [V: manuals].

Playdate's crank is strongest for spinning, steering and scrubbing time; gimmicky when it is an occasional extra [V].
Our encoders have detents: better for steps and aiming than fast flicks [M]; smooth paddles towards a target.

## 5. Sound recipes

**Sleigh bells: STK PhISEM** [V: thestk/stk Shakers.cpp]: 32 bells; system decay 0.9994; sound decay 0.97;
resonators 2500, 5300, 6500, 8300, 9800 Hz, radius 0.99, gains 1, 1, 1, 0.5, 0.3, frequency jitter 3 %; output
`x[n] − x[n−2]`; gain `log(N)/N`; a shake adds `amp × 0.1` to the energy (max 1). Collision test per sample:
`rand1024 < N` then `snd += energy`. One shake lasts about 0.26 s. Integer: decays 32748 and 31785 (Q15). About 60-80
cycles a sample.

**Bells** (partials relative to the prime, with suggested amplitude and T60):

| Bell | Partials |
|---|---|
| Church [V: Hibberts, Wikipedia "Strike tone"] | hum 0.5 (0.5, 8 s), prime 1 (0.7, 5 s), tierce 1.183 (0.8, 4 s), quint 1.506 (0.3, 2 s), nominal 2 (1.0, 3 s), twelfth 3.011 (0.4, 1 s); a 0.5-2 Hz beat on the hum |
| Chowning FM bell [V] | c:m 1:1.4, index 10 decaying with the amplitude |
| Tubular | bar modes 1, 2.76, 5.40, 8.93 [V: BYU]; heard pitch about half of mode 4 |
| Glockenspiel | 1 (2 s), 2.76 (0.3, 0.25 s), 5.40 (0.15, 0.08 s), a 2 ms click |
| Celesta | FM 1:1, index 0.8 to 0 in 60 ms, T60 1-2 s, a 4:1 ping at 0.1 for 30 ms |
| Music box | tine modes 1 (1.5-3 s), 6.27 (0.2, 80 ms), 17.55 (0.05, 20 ms); 1-2 ms click; paired detune 1-3 Hz; ±3 ms timing |
| Handbell | 1 (4 s), 3.0 (0.4, 2 s), 4.2 (0.1, 0.3 s) [?] |

Additive partials cost about 8 cycles each; 16 bells of 6 partials about 10 % of the processor.

**Choir** formants (Hz / dB / bandwidth) [V: hsc3 vowel data]: soprano "a" 800/0/80, 1150/−6/90, 2900/−32/120; alto
"a" 800/0/80, 1150/−4/90, 2800/−20/120; "u" soprano 325/0/50, 700/−16/60, 2700/−35/170. Saw into three state-variable
band-passes (`f = 2 sin(π fc / fs)`, Q15); vibrato 5-6 Hz ±15-30 cents faded in over 300 ms; 3-4 voices ±5-12 cents.

**Organ:** drawbar ratios 0.5, 1.5, 1, 2, 3, 4, 5, 6, 8; church 8' 4' 2' plus a quiet mixture (6, 8, 12, 16); 25 ms
of chiff. **Brass:** FM 1:1, index following the level (peak 5), attack 60-100 ms, a −30 cent scoop.

**Reverb:** Freeverb [V: CCRMA]: combs 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617; all-passes 556, 441, 341, 225;
+23 samples on the right; about 50 KB stereo, about 100 cycles a sample a channel. A 4-line FDN (lengths 1931, 2467,
2971, 3533, Hadamard mix, `g = 10^(−3 d / (fs T60))`): about 22 KB, about 60 cycles. Cathedral: pre-delay 40-60 ms,
T60 4-6 s, damping at 4-5 kHz, input high-passed at 200 Hz. Round, do not truncate, inside feedback loops.

**Sound effects:** fire (hiss, crackles 1-10 a second, lapping; mix 0.1/0.3/0.6 [V: Farnell]); snow crunch (grains of
1-3 ms noise band-passed 1.5-4 kHz in a 250-400 ms step); wind (band-pass wandering 300-1200 Hz); cracker snap (1 ms
click, 60 ms noise, 200 ms crackle); sleigh whoosh (sweep 400 to 3000 to 400 Hz over 600 ms, bells pitched down 3 %);
hooves (click into resonators at 1.2 and 2.5 kHz); coin ding (B5 80 ms then E6 400 ms); sparkle (pentatonic pings
2-5 kHz, 10-30 a second); magic glissando (whole-tone run, 25-35 ms a note).

**Chip mode** [V: Pan Docs]: pulse duties 12.5, 25, 50 %; LFSR noise 15-bit or 7-bit (7-bit is tonal); arpeggios
changing every 3 blocks; it suits the speaker because its energy is all in harmonics.

**The speaker:** synthesise a bass note's 2nd, 3rd and 4th harmonics (about 1.0, 0.7, 0.5) instead of its
fundamental when on the speaker; the ear hears the missing fundamental [V: virtual-bass literature]. High-pass the
speaker path at about 150 Hz.

Note: the sound researcher costed cycles for an ARM Cortex-M; our chip is JieLi's pi32v2. Treat the numbers as rough
and measure.

## 6. Hardware answers

- **No usable clock across power-off.** The AC79 has an RTC block (internal RC or a 32.768 kHz crystal on PR0/PR1)
  [V: AC79 SDK docs], but no FM-1 firmware uses it, no crystal is visible in photos, and the power switch seems to cut
  the whole chip [M]. USB gives no date. **The date must come over MIDI**: a Web MIDI page sending control changes
  (SysEx belongs to the updater) [V].
- **Screen:** ST7789V on SPI1, RGB565, `MADCTL 0x00`, no tearing line [V: lcd.c]. Clock = 60 MHz / (LCD_BAUD + 1);
  Dinghy uses 4 (12 MHz); fm1-base uses 0 ("60 MHz, about 44 MHz effective, measured") [V]. Full frame: 77 ms at
  12 MHz, 31 ms at 30 MHz, 21 ms at 44 MHz; RGB444 (`COLMOD 0x03`, used by FM-1 ZX on hardware) saves 25 %. Dinghy's
  API.md "3 ms a strip" is wrong (about 20 ms at 12 MHz). No palette mode. Vertical scrolling (VSCRDEF/VSCSAD) looks
  usable with an 80-line hidden band but is untested on the FM-1.
- **LEDs:** multiplexed with the key matrix, lit at most 1/11 of the time. Dinghy: off, a global backlight level, a
  fixed glow (about 1/24), on. felucca-bsp has per-LED levels 0-64 by sigma-delta (`fm1_led_anim.h`, MPL-2.0): about
  6-7 usable levels without flicker [V]. Single colour; REC red; PLAY has a second, green LED [V: fm1-base].
- **Velocity:** possible by porting felucca-bsp's input (about 880 lines) and fm1-base's sense code; a few days and
  calibration; conflicts with LED effects on held keys. Skip [V/M].
- **Second core:** misreads memory on some units; do not use [V].
- **Battery:** ADC channel 3, stock thresholds 531/561/591 for 0-3 bars [V]. **Headphones:** PB5 = 0 when plugged,
  bounces about 150 ms, the speaker mutes itself [V].
- **MASTER as a paddle:** an absolute pot, usable if the platform's volume handling is decoupled [V].
- **Identity:** the hub catalogue (294 firmwares, 22 November 2026): **FM-1_902** is the only clean free 3-digit
  identity; 962 and 984 are prefixes of longer ones. Melisma's 96N collides with nine. Web installers only resume
  `FM-1_9NN` [V].
