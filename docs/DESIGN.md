# Design: Tinsel for the M-VAVE FM-1

Status: approved, 23 November 2026 (builder-checked; the answers on testing, the public
release and the voice clips folded in).
Name: **Tinsel**. A Christmas firmware of music toys and games, all in one, for every age.
Credits: Joel Merrick, standing on Dinghy (zvenson), Felucca (Leo Kuroshita, Hügelton Instruments) and SLOOP
(isod89). Not a Merlin Engines project: it has its own playful identity.

## 1. What it is, and why

The FM-1 is cheap and small, and toddlers and non-musical grown-ups take to it at once. Tinsel is the thing
they switch on over Christmas and smile at: a snowy Grotto where every key rings a bell, a
carol book that plays Jingle Bells however you bash the keys, and a handful of games where a lit key tells you
what to press. Underneath, there is enough depth for a grown-up and a musician to enjoy it too.

It runs in parallel to Melisma (a serious music firmware by the same author) and is meant to be fun to make and fun to use. It
is public and GPL-3.0, so the FM-1 community can have it from the Firmware Hub.

## 2. Who it is for, in order

| Who | What they need |
|---|---|
| **A toddler** (the first user) | No reading. Instant cause and effect: press a key, see and hear something at once, in the place that key points to. No fail states. Nothing they can break. The keys come first; steering a cursor with a knob is the next delight |
| **A non-musical grown-up** | Sounds good whatever you press. Play a real carol by following the lights. Games that are actually games |
| **Musicians, and FM-1 people from the hub** | Real bell-ringing methods, harder levels, two-player games, nice sounds, a reason to keep it installed in December |

Design for the toddler first and the musician last. Where they conflict, the child wins on the default
settings and the musician gets a setting or a harder level.

## 3. Goals, non-goals, anti-goals

**Goals**
- Switch on, and within two seconds a toddler is making something delightful happen.
- One convention everywhere: **a brightly lit key means "press me"; a glowing key means "next"**.
- **HOME always goes back to the Grotto.** Nobody can get lost.
- Every toy and game has a toddler level where nothing can go wrong.
- An Advent calendar that gives a small surprise every day from 1 to 24 December, and grows with each release.
- On the maintainer's unit by **Sunday 29 November 2026** (Advent Sunday), with a release about every week until Christmas.

**Non-goals**
- Not a serious instrument (that is Melisma). No sequencer depth, no MIDI clock sync, no patch editing.
- No SMK-37 build for now (the platform supports it; nobody has asked).
- No key velocity, no second processor core, no Bluetooth (see section 13).
- No network or two-unit play (the TRS jack is input only; USB between two FM-1s needs a computer).

**Anti-goals** (it would be a failure even if it worked)
- A toddler can wipe a saved thing, change a setting by accident, or get stuck on a screen they cannot leave.
- Anything is loud enough to startle a small child by default.
- Text a child must read to know what to do.
- A wrong note, a buzzer or a "you lose" at the toddler level.
- Copyrighted songs, characters or brands (see section 11).
- Anything that risks bricking a unit (Melisma's `docs/FLASHING-SAFETY.md` rules apply unchanged).

## 4. Constraints from the hardware and platform

From the research (`docs/RESEARCH-NOTES.md` in the Tinsel repo; tags there):

- JieLi AC791N, integer only (no float, no 64-bit division; keep 64-bit multiplies out of per-sample loops and
  in `noinline` helpers because of a chip erratum at 360 MHz).
- Audio: 44.1 kHz, 256-frame blocks every 5.8 ms; stay under 2.9 ms per block in total, measured at the peak.
- 96 KB RAM, 336 KB pool for big buffers, 568 KB code and constant tables.
- Screen: ST7789V, 240 × 240, RGB565 over SPI, no tearing signal. Dinghy drives it at 12 MHz (a full frame takes
  about 77 ms); fm1-base runs the same panel at about 44 MHz effective (about 21 ms). No palette mode on the
  controller.
- 27 keys (F3..G5) each with a single-colour LED; 14 buttons with LEDs (REC red, PLAY has a second green LED);
  7 endless encoders with detents; the MASTER pot; a speaker that plays nothing below about 100 Hz (mutes when
  headphones are in, detectable on PB5).
- No clock that survives power-off. No microphone, motion sensor or vibration.
- Storage: 4 KB settings, eight 4 KB blobs, eight 16 KB slots. An erase stops the sound for about 50 ms a sector;
  save only when silent; never save in a loop.
- MIDI in from USB and TRS; the app sees channel messages and clock/transport only (SysEx belongs to the updater).

## 5. Approaches (how it gets built)

The maintainer asked to reuse existing work rather than redo it. Considered:

| | Approach | Trade-off |
|---|---|---|
| **A (chosen, Claude's call)** | A new public repo whose history starts from Dinghy's (`zvenson/dinghy` v0.5), then Melisma's platform fixes and tooling, then Tinsel's own app | Clean GPL lineage and credits; reuses everything we already trust (checks, host mode, emulator, boot-plate tool, installer flow, FM voice code); Tinsel's code starts small |
| B | Fork `gaskb/fm1-snake` (FM-1 Arcade, Dinghy-based, GPL-3.0) | Ten games for free, but they are Arcade's (Snake, Tetris, Pac-Man...), not Christmas, and their structure is not ours; we would delete most of it. Borrow its helpers instead |
| C | Build on fm1-base / felucca-bsp (MPL-2.0) | Velocity, Bluetooth and a faster screen out of the box, but a different platform from Melisma: none of our tooling, emulator flow or experience carries over |

Within A, Tinsel takes:

- **From Melisma (`joelio/melisma-fm1`)**: the macOS host-build fix in `gfx.c`; `tools/check.sh` and the
  pre-commit hook; the emulator recipe (`.tools/fm1-emu`, `--flash-chip uc25hq80`, `FM1_STRICT=1`);
  `tools/gen_splash.py` (boot plate from SVG); the FM voice and envelope code in `mel_synth.h` and its sine,
  pitch table and drum recipes as a starting point; the "save on demand, glow when unsaved" pattern.
- **From FM-1 Arcade (`gaskb/fm1-snake`, GPL-3.0)**: the fixed-point (Q8) motion and paddle code, the
  erase-and-redraw sprite pattern, the high-score record. Credit in LICENSING.md and file headers.
- **From RetroDash (`nasonliu/retrodash`, GPL-3.0, Dinghy-based)**: its hardware-validation lessons (frames
  scheduled off the millisecond clock, not the 24 MHz counter that wraps) and its LED hint scheme.
- **From FM-1 Midi-Mania (`shan-sherif/fm-1-midi-mania`, GPL-3.0)**: the timing judge (PERFECT 30 ms, GREAT
  60 ms, GOOD 110 ms) and its test, for the grown-up levels of the games.
- **From felucca-bsp (MPL-2.0, combinable with GPL-3.0: no incompatibility notice)**: the per-LED brightness
  mechanism (`fm1_led_anim.h`), giving 6 to 7 levels; and the headphone detect (`fm1_jack.h`).
- **Ideas only** (MPL-2.0 apps on a different platform): Kris Jangle, GNOMNOM, Boardwalk, Mighty Arcade. Tinsel
  must not be a reskin of any of them (section 11).

## 6. The shape of it

```
            switch on
               |
   boot plate: TINSEL in gold on a red ribbon, a sleigh-bell flourish (2 s, any key skips)
               |
        +--> THE GROTTO  <-------------------- HOME, from anywhere, always
        |      |  HOME again
        |      v
        |   THE SLEIGH (picture menu: one picture per toy, game and the calendar)
        |      |  turn any knob to choose, press any key or PLAY to go
        |      v
        +-- a toy, a game, the Advent calendar
```

- **The Grotto** is home and the toddler's whole world. It is where Tinsel starts every time.
- **The Sleigh** is the menu: a row of big pictures (a book for the Carol Book, a bell for the Bell Tower, a
  present for Present Catch...). Turning any knob slides it; the chosen picture is big and named in small text
  for grown-ups, with a short jingle that is different for each item so a child learns them by sound. Any key or
  PLAY opens it. Things not released yet show as a wrapped present that wobbles and says "soon".
- **HOME** goes back to the Grotto from anywhere, at once, saving nothing and losing nothing that matters (a game
  in progress is abandoned; that is fine).

### Global controls

| Control | Everywhere |
|---|---|
| HOME | Back to the Grotto. In the Grotto: the Sleigh |
| MASTER | Volume, under a cap (section 9) |
| A brightly lit key | Press me |
| A glowing key | Next, or "this one does something" |
| **Grown-up gesture**: hold EDIT and SAVE together for 2 seconds | The grown-up panel (section 10). The only way to reach anything destructive or any setting |

Everything else is per toy or game and is printed on a help card: hold EDIT for a moment and the screen shows
what each control does in this place (pictures first, a few words for grown-ups).

### Levels

Every game has three levels. **The level is chosen only in the grown-up panel**, per game (Claude's call, after
the builder check: knobs are the toddler's cursor, so a knob on a start screen would put them into a game with
misses). The game's start screen shows the level as one, two or three stars and does nothing when a knob turns;
any key or PLAY starts. A grown-up who wants Santa for one go uses the panel and sets it back.

| Level | Picture | Rules |
|---|---|---|
| **Little Elf** (default) | one star | No fail state, no timer pressure. Any key is never "wrong": it does something nice, and the right key does something better. Speed never rises |
| **Elf** | two stars | Real game, gentle: misses cost little, speed rises slowly |
| **Santa** | three stars | Proper challenge, timing judged, high scores kept |

## 7. The toys (no winning or losing)

### 7.1 The Grotto (home)

A night scene: a Christmas tree in the middle, snow falling, presents and a snowman at the bottom, a robin.

- **Keys:** each of the 27 keys rings a bell (pitched to a scale so any cluster sounds sweet: a major
  pentatonic by default) and lights the bauble on the tree that sits above it, left to right across the screen.
  Several keys at once ring a chord and light several baubles. Mashing everything makes a sleigh-bell flurry
  and the whole tree sparkles, never a mess.
- **Knobs:** KNOB1 moves the robin left and right, KNOB2 up and down (a cursor to steer). SELECT and ALGORITHM do the
  same, so whichever knob a small hand grabs works. Where the robin lands, something reacts: a present wobbles and pops
  a sparkle, the snowman's hat jumps, the star on top chimes, the chimney puffs. Each reaction has its own
  sound.
- **PLAY:** the big moment: a flurry of snow, a short fanfare, every light on the tree and every key LED
  twinkles at once.
- **OCT−/OCT+:** the bells go lower or higher, and the snow gets heavier or lighter.
- **The other buttons** each make a festive sound and a small visual (a reindeer runs past, a shooting star,
  the moon winks). None of them changes a setting.
- **Left alone** for a minute: the Grotto plays itself gently (a slow carol on the bells, the robin hopping),
  then dims the screen after 5 minutes to save the battery. Any touch wakes it.
- **Advent decorations:** each opened Advent door can add a thing to the Grotto (a new ornament, a new
  creature, a new reaction), so the scene fills up through December.

### 7.2 Carol Book

About 25 public-domain carols (section 11), each with a picture. Choose by turning a knob; PLAY starts.
Three ways to play, chosen on the carol's screen:

| Way | What happens | For |
|---|---|---|
| **Magic** (default) | Any key plays the next note of the melody, with the backing following. Bash, and the carol comes out; press faster and it goes faster; stop and it waits | A toddler |
| **Follow** | The next key is lit brightly, the one after glows; the backing waits until you press the right key (a wrong key sounds quietly, in tune, and does not advance) | A grown-up |
| **Listen** | The carol plays itself with the keys lighting along | Anyone; learning a tune |

The backing is simple: a bass note and a chord per beat on a soft sound, plus sleigh bells on the beat for the
jolly ones. A grown-up can choose the melody sound (music box, celesta, bells, choir, organ, chip) with a knob.
At the end: a little celebration on screen (fireworks of snow), never a score at the Little Elf level.

### 7.3 Music Box

A cylinder with pins on screen and a comb of 27 tines.

- **The boxes** (Claude's call, after the builder check, so mashing never buries a tune): the carol boxes come
  pinned and are **play-only**; keys on them just sound the tines. One **scribble box** takes pins from key presses
  as the cylinder turns; it is not saved and starts empty each visit. A grown-up **keeps** a scribble into one of
  four kept boxes from the grown-up panel; kept boxes are play-only too, and are cleared only from the panel.
- **Crank it:** turn KNOB1 (or any knob) to turn the cylinder: the faster you turn, the faster it plays;
  backwards plays backwards. Let go and it stops (a real music box), or PLAY sets it to wind itself and run down
  slowly like clockwork.
- The sound is the music-box recipe (section 9): plucked tines, a little ring, a gentle wobble.

### 7.4 Bell Tower

A church tower with six (or eight) bells drawn as bells with ropes.

- **Rounds:** PLAY starts the bells ringing rounds, high to low, with the handstroke gap of real ringing.
- **Call changes** (the toddler and grown-up way): press any two neighbouring lit keys and those two bells
  swap places in the order; the tower keeps ringing the new order. The famous orders (Queens, Tittums, back
  rounds) are recognised and celebrated.
- **Methods** (for ringers and the curious): turn a knob to pick Plain Hunt, Plain Bob Doubles, Plain Bob Minor or Grandsire
  Doubles; the tower rings it from its place notation, with the rows shown scrolling. **Ring a bell
  yourself:** choose one bell; its key is lit; press it when your bell's turn comes (timing judged at Elf and
  Santa, any time near enough at Little Elf). Bobs and singles called with OCT− and OCT+.
- **The sound:** the church-bell recipe (hum, prime, tierce, quint, nominal), in the cathedral reverb.

### 7.5 Snow Globe

A glass globe with a little village inside.

- **Turn any knob** to shake it: snow swirls with the speed of turning, and the shimmer of tiny bells follows
  the snow (a soft cloud of sparkly notes).
- **Stop** and the snow settles, and the sparkle settles into a chord.
- **Keys** change what is inside the globe (a church, a tree, a snowman, a sleigh...), each with its sound.

## 8. The games

All games follow the lit-key convention and the three levels.

### 8.1 Present Catch

Presents fall from Santa's sleigh across the top of the screen down towards seven chimneys along the bottom.
Each chimney belongs to a group of keys; the key under a falling present lights brightly as it nears.

- **Little Elf:** presents fall slowly, one at a time; any key in the right group catches it; a missed present
  just bounces softly into the snow and is caught by the snowman anyway. Each catch plays the next note of a
  carol, so a run of catches plays the song.
- **Elf:** several presents at once, the right key needed, misses lose a little.
- **Santa:** timing judged (catch it as it reaches the chimney), speed rises, high score kept.
- Different from GNOMNOM (lanes and eating to the beat) by its presentation and by aim-free, carol-driven
  catching; it does not copy its code or art.

### 8.2 Jingle It!

Our own call-and-do game in the spirit of the classic electronic shout-and-press toys: Tinsel calls an
action, you do it.

| Call | Do | Picture |
|---|---|---|
| "Ring it!" | Press the lit key | a bell |
| "Twist it!" | Turn any knob | a twisting candy cane |
| "Jingle it!" | Press PLAY (or any lit button) | sleigh bells |
| "Shh!" (Elf and up) | Do nothing until the next call | a sleeping reindeer |

- The calls are spoken (generated voice clips, section 9.4) and drawn; a different sound for each so that no
  reading is needed even without the voice.
- **Little Elf:** no time limit; every success cheers; it never ends unless you go HOME.
- **Elf / Santa:** a time window that shrinks, a beat that speeds up, three misses end the round; high score.
- Not named or styled after any existing toy brand.

### 8.3 Snowball Rally (two players)

A rooftop Pong for two, one knob each at opposite ends of the panel (the two encoders furthest apart; which ones
is checked on the unit, section 13). The ball is a snowball; every hit sounds a jingle note on the beat of a
background carol; a miss turns the snowball into a little snowman on that side.

- One player: against a gentle elf who misses sometimes.
- Little Elf: the paddles are wide and the snowball slow; nobody loses, the snowmen just pile up.
- Santa: Bit.Trip-style speed-ups on the beat.

### 8.4 Cracker Pull (two players)

A Christmas cracker across the screen; player one holds the low keys (F3..C4), player two the high keys
(C5..G5). Tap your keys in time with the beat to pull; on the beat pulls hardest, mashing slips. The cracker
stretches, and goes BANG (the cracker-snap sound, a flash on screen, every LED). The winner gets the paper hat
on screen; then a joke (Tinsel's own, written for this, in big text for grown-ups to read out) and a little
toy picture.

- Little Elf: no beat needed, just tapping; it always ends in a bang after a few seconds.

### 8.5 Sleigh Ride

The sleigh flies over a night village, seen from behind. KNOB1 (or any knob) steers left and right. Bell posts
come towards you, each with a lit key; ring it as you pass and it chimes the next note of the tune.

- Little Elf: steering cannot crash; bells ring if you press anything near them.
- Santa: steer between chimneys, ring on time, speed up.
- This is the most demanding on the screen; it is the last game built and may slip past Christmas.

### 8.6 Later, if there is time (not promised)

- **Carol Detective:** a carol plays with one wrong note; press where it went wrong.
- **Robin's Duet:** two players, call and answer on the low and high keys.
- **Reindeer Roll-call:** rhythm commands, each reindeer answering a four-beat pattern.

## 9. Sound

### 9.1 The instruments

| Sound | How (recipes in RESEARCH-NOTES) | Used for |
|---|---|---|
| Sleigh bells | STK PhISEM "Sleigh Bells": 32 jingles, five resonators 2.5 to 9.8 kHz, shaken bursts with micro-timing | Everywhere |
| Church bell | Additive: hum 0.5, prime 1, tierce 1.183, quint 1.506, nominal 2, twelfth 3.011, own decays | Bell Tower, Grotto |
| Handbell | Additive 1, 3, 4.2 | Grotto keys, Carol Book |
| Celesta | 2-op FM 1:1, short index decay, plus a quiet 4:1 ping | Sugar Plum, Carol Book |
| Music box | Tine partials 1, 6.27, 17.55, a click, paired detune | Music Box |
| Glockenspiel | 1, 2.76, 5.40, a click | Present Catch |
| Choir "aah" | Saw through three formant filters, vibrato, three detuned voices | Carol Book (grown-up) |
| Organ | Additive principals 8' 4' 2' plus a quiet mixture, a breath of chiff | Carol Book |
| Brass band | 2-op FM 1:1, index following the level, a scoop | Carol Book |
| Chip | Pulse waves (12.5, 25, 50 %), arpeggio chords, LFSR noise drums | A "retro" switch for everything |
| Soft backing | Melisma's FM voices (round bass, soft pad) | Carol Book, games |

### 9.2 Effects and sound effects

- A **cathedral reverb** (a 4-line feedback delay network like SLOOP's, about 22 KB and a few per cent of the
  processor, or Freeverb at about 50 KB) on everything, less on the games, more on the bells and the choir.
- Game and Grotto effects, each a small recipe: crackling fire, snow crunch, wind, cracker snap, sleigh whoosh
  (with a Doppler drop on the bells), reindeer hooves, a collect "ding", sparkle, magic glissando, paper rustle.

### 9.3 The little speaker and little ears

- **Speaker-aware bass:** on the speaker (headphones out, detected on PB5) bass notes drop their fundamental and
  play their second to fourth harmonics instead, so the ear still hears the low note; with headphones in, the real
  bass plays. Melodies and bells sit between 400 Hz and 4 kHz.
- **Volume cap:** the output is limited to a gentle maximum by default (MASTER moves within it). The grown-up
  panel lifts the cap. Sudden loud sounds (the cracker bang, the fanfare) are limited to the same ceiling.

### 9.4 Voice clips

- Spoken call-outs ("Ring it!", "Twist it!", "Jingle it!", "Shh!", "Well done!", "Ho ho ho!", "Merry
  Christmas!", numbers one to ten, names of the games) made with ElevenLabs text-to-speech by a script in
  `tools/` at build-asset time, never at build time. The key is read from the maintainer's macOS keychain
  and is never in the repo. The generated clips are converted to 11 kHz 4-bit ADPCM (about
  5.5 KB a second; the whole set should stay under 100 KB of flash) and committed as converted data with their
  source text and the voice used.
- **The clips cannot be shared** under the text-to-speech plan used (its output may not be passed on for anyone's
  use, as a GPL repo would allow). So generated clips are **never committed**: they live in the gitignored
  `private/` folder and go only into a **private build**. The **public build** uses short melodic cues instead of
  words (a rising bell figure for "Ring it!", a twisting glissando for "Twist it!", a jingle for "Jingle it!", a
  hush for "Shh!"), drawn as well as heard, so every game plays fully without voices. On a plan that allows
  sharing, clips could be committed later without changing the code.
- **Own recordings:** anyone can drop their own recordings into `private/` and build a private package with
  them. They are never committed.

## 10. The grown-up panel

Reached only by holding EDIT and SAVE together for 2 seconds (from anywhere). It is a plain list for grown-ups,
moved through with SELECT, changed with ALGORITHM, left with HOME.

- Volume cap (gentle, medium, full).
- Level per game (Little Elf, Elf, Santa), and one setting to change them all at once.
- Music Box: keep the scribble box into one of four kept boxes.
- Sound style (normal, retro chip).
- Advent: open all doors now; close them again; set the date by hand (if no computer is at hand).
- Clear high scores; clear the music boxes (each asks for a confirming press).
- About: version, credits (Joel Merrick; the lineage; the borrowed code; the sound recipes' sources).

Nothing a toddler can do outside this panel changes a setting or deletes anything.

## 11. Content rules: music, words and pictures

- **Melodies:** only the public-domain list in RESEARCH-NOTES (Jingle Bells, Silent Night, Deck the Halls, Good
  King Wenceslas, Ding Dong Merrily on High, The Holly and the Ivy, O Christmas Tree, We Wish You a Merry
  Christmas, God Rest Ye Merry Gentlemen, Hark the Herald, O Come All Ye Faithful, The First Nowell, Away in a
  Manger (Cradle Song), Joy to the World, In the Bleak Midwinter (Holst's Cranham), Coventry Carol, I Saw Three
  Ships, We Three Kings, Once in Royal David's City, While Shepherds Watched, Twelve Days of Christmas,
  Shchedryk (the Ukrainian original of "Carol of the Bells"), Dance of the Sugar Plum Fairy, the Nutcracker
  March, Here We Come A-wassailing, Auld Lang Syne).
- **Not:** anything in the "avoid" list (Sleigh Ride, Rudolph, Frosty, Let It Snow, Winter Wonderland, White
  Christmas, Jingle Bell Rock, Walking in the Air, Last Christmas, and the rest). Carol of the Bells only as
  Shchedryk (Wilhousky's 1936 English version is in copyright); Sussex Carol only in the 1878 tune; O Little
  Town of Bethlehem only to the US tune St Louis.
- **Transcriptions are our own:** each melody is written by hand in a small ABC-like text file in `music/`,
  GPL-3.0, with a comment naming the old printed source it was checked against (pre-1930, e.g. on IMSLP). Never
  copied from a modern carol book (a modern edition can carry its own copyright in the UK) and never from The
  Session (its licence forbids processing with language models). A build step turns the files into C tables,
  transposes each to fit F3..G5 and checks the range.
- **Words:** only titles on screen; no lyrics are needed. Cracker jokes are Tinsel's own.
- **Pictures:** our own art, drawn in code or from small PNGs made for Tinsel. No characters, brands or styles of
  existing films, books, adverts or games (The Snowman, the Grinch, Paddington, Coca-Cola, John Lewis...). A
  generic Santa, reindeer, robin and snowman are fine.
- **Overlap with other FM-1 firmwares:** a carol copy-me game (Kris Jangle) and a falling-notes lane game
  (GNOMNOM) exist. Tinsel has neither a Simon-with-carols game nor GNOMNOM's lanes-and-gnome; its Carol Book and
  Present Catch must stay clearly its own.

## 12. Screen, lights and the look

- **The look:** bright, chunky, high-contrast, friendly. Night blue, snow white, tinsel gold, holly red and
  green. Big shapes a toddler reads at arm's length. Text only for grown-ups, in the platform's two fonts.
- **Drawing:** a full-screen 8-bit indexed frame (57.6 KB in the pool) expanded to RGB565 strip by strip as it
  is sent (FM-1 Doom's technique), with only the changed strips sent. Twinkling and colour cycling are done by
  changing palette entries. Snow is particles redrawn over a background that can be recomputed.
- **Screen speed:** raise the SPI clock from Dinghy's 12 MHz towards fm1-base's measured setting
  (`LCD_BAUD 0`), in steps, tested in the emulator and then on a real unit; the target is 30 frames a second for
  the games. If the faster clock misbehaves, fall back to 12 MHz with smaller dirty areas.
- **Key and button lights:** port felucca-bsp's per-LED level mechanism so each LED has six or seven brightness
  levels. Used for: "press me" (bright), "next" (glow), fairy-light twinkles and chases across the keys, the
  tree lighting up with the keys in the Grotto.
- **Boot plate:** TINSEL in gold on a red ribbon with a little bow, snow falling, a sleigh-bell flourish; made
  with `tools/gen_splash.py` from an SVG. Any key skips it.

## 13. The Advent calendar

24 doors on a picture of a house front (a door per window). The door for today wobbles and is lit.

- **Each door** gives one of: a new carol in the Carol Book; a new Grotto decoration or creature; a new sound; a
  cracker joke and hat; on a few days, a whole new toy or game (for example, Bell Tower on 6 December, Cracker
  Pull on 13 December, Snowball Rally on 20 December; the exact plan follows what is built). Nothing that
  shipped in v0.1 is ever locked: the doors add, they never take away.
- **The date:** there is no clock that survives power-off. A small web page (`web/advent.html`, Chrome or Edge,
  Web MIDI) sends today's date as four control changes on MIDI channel 16 (year minus 2000, month, day, a
  checksum) when the FM-1 is plugged in. Tinsel keeps the latest date it has been told in flash; while on, it
  counts time from its own millisecond clock. Doors up to today's date open.
- **Without a computer:** the grown-up panel sets the date by hand; and if Tinsel has never been told a date, it
  opens one door for each day it is switched on (a new door at the first power-on after the previous door was
  opened; this can be cheated by switching off and on, and that is fine).
- **After the 24th:** all doors are open; on the 25th the Grotto has a Christmas Day surprise.
- **Updates fill doors:** v0.1 contains the doors it has content for; each release adds more. Opened doors are
  kept across installs (same store magic).

## 14. What is kept, and when

| What | Where | Saved when |
|---|---|---|
| Settings (cap, level, sound style), the known date, opened doors | settings | On leaving the grown-up panel; when a door opens; when a new date arrives |
| High scores | a blob | On the game-over screen, after its sound ends, only if a record changed |
| Kept music boxes | slots | Only when a grown-up keeps or clears one in the panel (silent there) |

Never in a loop, never during sound. The store has its own magic ("TNSL"), so it never reads Melisma's or any
other firmware's data.

**Tinsel never erases Melisma's data** (Claude's call, after the builder check). Every Dinghy fork, Melisma included,
keeps its store in the same sectors (settings 0xFC000, blobs 0xF4000.., slots 0x97000..0xB6FFF), so a magic only
stops a firmware *reading* another's data, not *overwriting* it; Tinsel saves on its own (a door, a date), so on a
unit that switches between the two it would wipe Melisma's piece. Tinsel therefore keeps its whole store in the part
of the FM-1's data area that Melisma does not use, **0xB7000..0xDFFFF** (inside `FL_DATA`, 0x97000..0xE0000; clear
of the update staging at 0xE0000), which is exactly one settings sector, eight blobs and eight 16 KB slots
(41 sectors). `store.h` gets the new addresses and a narrower `FL_RANGE_OK` that allows only that window and the
update staging, with compile-time asserts that it stays inside `FL_DATA` and clear of 0xE0000; a host test checks
that no Tinsel save reaches 0x97000..0xB6FFF, 0xF4000.. or 0xFC000. This is the first job of v0.1, before anything
saves. Other Felucca-family firmwares that use 0xB7000.. may still lose their data to Tinsel, as with any
firmware; that is normal on the FM-1.

## 15. Identity and repository

- Repo: `github.com/joelio/tinsel-fm1`, **public**, GPL-3.0-only, history starting from Dinghy's.
- Package identity: **FM-1_902** for every release (the only fully free 3-digit identity on the hub's list on
  22 November 2026; ask the hub to reserve it). `SCREEN_NAME` "TINSEL", `SCREEN_COLOUR` tinsel gold,
  `DATA_FAMILY` "TINSEL", `STORE_MAGIC` "TNSL".
- Test packages: `build/tinsel-N.fwsc`, installed by hand with M-Flasher.
- No GitHub Actions on push (as for Melisma); checks are local (`tools/check.sh`, `--full`), with the pre-commit
  hook. Releases: tag and upload the `.fwsc` by hand, or the release workflow on a tag only, since the repo is
  public (Actions on public repos cost no credits).
- README with a photo or a host screenshot, a short user guide for grown-ups (`docs/GUIDE.md`) kept in step with
  the controls, LICENSING.md with every borrowed part credited.

## 16. Testing

The same three layers as Melisma, before anything goes near the unit:

1. **Unit tests** on the host for the pure parts: the carol tables (range, length, the source comment present),
   the bell-ringing methods (rows of Plain Bob and Grandsire against known lead heads; extents complete), the
   timing judge, the Advent date rules, the ADPCM decoder, the music-box pins.
2. **Host mode** scripts for each toy and game (keys, knobs, buttons over time) producing screen PNGs and a WAV;
   golden screenshots for the main screens.
3. **The emulator** with the real `.fwsc` (`--flash-chip uc25hq80`, `FM1_STRICT=1`) to boot, run each mode and
   check the audio block time stays under 2.9 ms at the peak.

Then on a real unit: a person listens (Claude cannot hear), and a toddler is the acceptance test.

## 17. Releases and order of work

| Release | Target | Contents |
|---|---|---|
| **v0.1** | Sunday 29 November | Platform carried over (with LED levels and the faster screen if they pass testing); boot plate; the Grotto; the Sleigh menu; Carol Book (Magic, Follow, Listen) with at least 8 carols; Present Catch; the Advent calendar with doors 1 to 7; sleigh bells, handbell, celesta, music box sounds; reverb; the volume cap; the grown-up panel; the date page |
| v0.2 | about 6 December | Bell Tower; Music Box; Jingle It!; more carols; doors to 14 |
| v0.3 | about 13 December | Cracker Pull; Snow Globe; choir, organ, brass; chip mode; doors to 21 |
| v0.4 | about 20 December | Snowball Rally; Sleigh Ride if it is ready; the last doors; the Christmas Day surprise |

**v0.1 is public from the start** (decided 23 November): FM-1_902 reserved with the hub, the
README with a screenshot, `docs/GUIDE.md`, a complete LICENSING.md, a `v0.1` tag and release, and the hub listing
request. Two packages per release: `tinsel-X.Y.fwsc` (public, melodic cues) and a private build made locally with
the voice clips, never published.

**Test installs before the 29th** (decided 23 November): a mid-week build (about Wednesday 25 November) that proves
the faster screen clock and the LED levels on a real unit, then v0.1 on the 29th. Each of the two upgrades is a
build-time setting with the safe value as the fallback, so a failure on the unit means flipping it back, not
redesigning.

Order inside v0.1: platform and identity, Tinsel's own store window (section 14), then the Grotto (it proves keys, LEDs, screen and sound together),
then the Carol Book, then Present Catch, then the calendar. If time is short, the calendar ships with fewer
doors rather than the Grotto shipping worse.

## 18. Risks and unknowns

| Risk | Plan |
|---|---|
| The faster screen clock misbehaves on a real unit | Step up from 12 MHz; emulator first; keep 12 MHz as the fallback; draw less |
| The LED level port breaks the key scan's timing | Port in isolation, test the scan in host mode and the emulator; keep Dinghy's two levels as a fallback |
| Audio over budget with reverb plus many bells | Measure every mode in the emulator; reverb FDN rather than Freeverb; fewer partials per bell; envelopes once per 32 frames |
| Generated voice clips cannot be shared | Clips only in the local private build; melodic cues in the public one |
| Which encoders sit at opposite ends of the panel (two-player) | Check on the unit before Snowball Rally |
| Hardware scrolling (ST7789 VSCRDEF) untested on the FM-1 | Not relied on; an experiment for Sleigh Ride only |
| Too much for the 29th | v0.1 is the Grotto, Carol Book and Present Catch; everything else can move a week |
| 64-bit multiply erratum at 360 MHz | 64-bit multiplies only in `noinline` helpers, never in per-sample loops |

## 19. Left to the builder

- The exact picture designs, colours within the palette, animations and the jingles for each Sleigh item.
- Which carols go in v0.1 (at least 8, from the list in section 11, favouring the ones a toddler may know:
  Jingle Bells, We Wish You a Merry Christmas, Deck the Halls, Away in a Manger).
- Code layout inside `firmware/src/` (one file per toy or game is suggested), the internal engine for sound
  voices, the reverb choice (FDN or Freeverb), the ADPCM flavour.
- The cracker jokes (Tinsel's own, family-friendly, groan-worthy).
- The mapping of the 27 keys to Grotto baubles and Present Catch chimneys.

## 20. Separately, for Melisma (not part of Tinsel)

Found during this research and reported; to be handled in the Melisma repo, not here:

1. Melisma's package identity `FM-1_96N` collides with nine other firmwares on the hub (960 is SID-1; 961 and
   963 to 969 are taken). 962 is free as a string but is the start of ARTIHIVE's `FM-1_9620`. Pick and reserve
   another before Melisma goes to the hub.
2. Dinghy drives the screen at 12 MHz and its API.md says a 240 × 64 strip goes out "in some 3 ms"; at 12 MHz it is
   about 20 ms. Melisma would benefit from the same faster clock Tinsel tests.
