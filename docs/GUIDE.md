# Tinsel: the grown-ups' guide

Tinsel is made so that a small child can switch it on and play without help, and so that nothing they do can lose
anything. This page is for the grown-up who sets it up. It describes Tinsel as it is now (v0.1); later releases add
more toys and games through December.

## The rules everywhere

| | |
| --- | --- |
| **HOME** | Back to the Grotto, from anywhere. In the Grotto: the Sleigh (the menu) |
| **A brightly lit key** | Press me |
| **A glowing key** | Next, or "this does something" |
| **MASTER** | The volume, under the ceiling set in the panel |
| **EDIT and SAVE, held together for 2 seconds** | The grown-up panel. Nothing else changes a setting or deletes anything |

## The Grotto (where it starts)

| | |
| --- | --- |
| Keys | Each rings a handbell and lights the bauble above it. The bells are tuned so any cluster sounds sweet. Lots of keys at once: sleigh bells, and the tree sparkles |
| Any knob | Moves the robin: SELECT, KNOB1 and PRESETS across; ALGORITHM and KNOB2 to KNOB4 up and down. What it lands on reacts: the star, the moon, the presents, the snowman, the baubles, a mouse by the trunk |
| PLAY | A fanfare, a snowstorm, every light |
| OCT- / OCT+ | The bells lower or higher, the snow lighter or heavier |
| FX, SEL, ENV, LFO, GLO, ARP, SEQ, REC | A reindeer, a shooting star, the moon winks, a snow puff, the lights chase, a sparkle, the church bell, the robin sings |

Left alone for a minute, the Grotto plays a carol to itself on the bells. After five minutes the screen dims (any
touch wakes it). Doors of the Advent calendar add things to the Grotto: a candy cane, a gingerbread man, a stocking.

## The Sleigh (the menu)

In the Grotto, the little sleigh in the top corner says **HOME**, and the HOME button breathes: press it for the
Sleigh. Turn any knob to slide through the pictures; each has its own little jingle. Then press **PLAY** (the gold
▶ under the picture, and PLAY's light pulses), or any key, or any other button but HOME, EDIT and SAVE: mashing
buttons gets in. A wrapped present is something still to come.

## The Carol Book

Turn a knob to choose a carol (a picture each). OCT- and OCT+ choose the way to play it; PLAY (the gold ▶), any key
or another button starts.

| Way | Picture | What happens |
| --- | --- | --- |
| Magic | a wand | Any key plays the next note, and the backing follows. Press faster and it goes faster; stop and it waits |
| Follow | a lit key | The next key lights brightly, the one after glows. Only the lit key moves the carol on; another key sounds quietly |
| Listen | an ear | It plays itself while the keys light along |

While a carol plays, a knob changes the melody's sound (celesta, music box, handbell, glockenspiel, chime); PLAY goes
back to the page. At the end: snow fireworks.

The carols: Jingle Bells, We Wish You a Merry Christmas, Deck the Halls, Silent Night, Good King Wenceslas, Joy to
the World, God Rest Ye Merry Gentlemen, Shchedryk (Carol of the Bells).

## Present Catch

PLAY (the gold ▶), any key or any button starts. Presents fall from Santa's sleigh towards seven chimneys. The keys under the lowest present
light up: there are seven groups of keys, one under each chimney. Each catch plays the next note of Jingle Bells.

| Level (set in the panel) | |
| --- | --- |
| Little Elf (the default) | One present at a time, slowly. Caught any time it is falling. One that reaches the bottom is caught by the snowman: nothing is ever missed, and it never ends |
| Elf | Two at a time; caught in the lower half; five misses end the round |
| Santa | Three at a time; caught at the chimney only; three misses end it; faster and faster |

The score shows as stars (one a catch, up to ten), with the number in the corner. The best score is kept.

## The Advent calendar

A house with 24 windows. **The first 24 keys are the doors**: key 1 is door 1, and so on. Today's door wobbles and
its key pulses; doors already open glow. Knobs move a gold frame too, and PLAY opens the framed door. A door that
may not open yet just boings. An open door can be opened again any time.

Behind the doors: carols (played on handbells), cracker jokes (with a bang), sounds, and decorations for the Grotto.

**How it knows the date.** The FM-1 forgets the date when it is switched off. Two ways:

1. **The date page.** Plug the FM-1 into a computer by USB, open [web/advent.html](../web/advent.html) in Chrome or
   Edge, and press the button. Tinsel twinkles when it has the date, and keeps it. Do this once a day, or whenever
   you like: doors up to today open, and the date never goes backwards.
2. **Without a computer**, Tinsel opens a door for each day it is switched on (the next door becomes available the
   first time it is switched on after the last one was opened). Switching it off and on again opens the next one
   early. That is allowed.

The panel can also set the date by hand, or open the whole calendar.

## The grown-up panel

Hold **EDIT and SAVE together for 2 seconds**. SELECT (or KNOB1) moves; ALGORITHM (or KNOB2) changes; **HOME**
leaves and keeps the changes. Things that cannot be undone ask first: turn ALGORITHM, then press **PLAY** (it
blinks) to confirm.

| | |
| --- | --- |
| Volume | The ceiling: Gentle (the default, half of full), Medium, Full. MASTER still turns it down from there |
| Present Catch | Its level |
| All games | Every game's level at once |
| Advent: open all | The whole calendar, now |
| Advent: close all | Every door shut again, and the day count reset |
| Date | Today's date by hand, a day at a time |
| High scores | Clear them |
| Screen test | How fast the screen link runs (see below). Never kept: leaving the page puts it back |
| About | The version |

### The screen test (for testing a unit)

The FM-1's screen is fed over a serial link. Dinghy runs it at 12 MHz; the panel can be faster. The screen test
page fills the screen with moving colour every frame and shows the speed, the time a frame takes and the frames a
second. Turn ALGORITHM for 12, 15, 20, 30 and 60 MHz. If a speed shows a clean picture (no garbage, no stripes, no
wrong colours) for a minute, it works on that unit. Report which speeds do: a later release will use the fastest
safe one.

## What is kept, and when

The settings, the calendar and the high scores are saved by themselves, when nothing is sounding (a save makes the
sound stop for a moment). They live in Tinsel's own part of the flash, so installing another firmware and coming
back to Tinsel keeps them, and Tinsel never touches another firmware's saved things.

## If something goes wrong

| | |
| --- | --- |
| It misbehaves | Switch off and on |
| It will not start | Hold OCT- while switching on: the updater (no sound). Install again from there |
| It will not start at all | Hold REC while switching on: download mode, for M-Flasher |
| You want another firmware back | Install it as usual; Tinsel can always be installed again later |
