# The carols

One file per carol, our own transcriptions (GPL-3.0-only, like the rest of Tinsel), of public-domain melodies only
(docs/DESIGN.md section 11). `tools/gen_music.py` turns them into `firmware/gen/tn_music.h` and refuses a file whose
bars do not add up, whose chords do not cover the tune, that has no source line, or that will not fit the keys.

```
title:   shown in the Carol Book
source:  where the melody comes from (composer or tradition, the old print to check against)
meter:   4/4, 3/4, 2/4 or 6/8
tempo:   crotchets a minute (6/8: dotted crotchets)
pickup:  sixteenths before the first full bar (0 if none)
picture: bells, pudding, holly, star, crown, angel, candle, bell
jolly:   yes: sleigh bells on the beat when it plays
tune:    PITCH:LEN ...  (C4:4 is a crotchet on middle C; lengths in sixteenths; b for flat, # for sharp; r:LEN a rest;
         | a bar line, checked)
chords:  NAME:LEN ...  (C, Am, G7, Bb ...; the same bars)
```

These are transcribed by ear, from memory of the traditional tunes. Each still has to be checked note by note
against an old print (IMSLP has scans): until then the source line says so.
