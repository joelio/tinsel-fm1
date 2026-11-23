/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* A carol, played three ways (DESIGN section 7.2). Pure: no sound, no screen; the Carol Book (and the games, and the
 * Grotto left alone) turn its events into bells.
 *
 *   CM_MAGIC   any key plays the next note: bash, and the carol comes out; stop, and it waits
 *   CM_FOLLOW  the next note's key must be pressed; a wrong key does not move it on
 *   CM_LISTEN  it plays itself, in time
 * Rests pass by themselves. At the end `done` is set and nothing more happens (it never wraps silently). */
#ifndef TN_CAROL_H
#define TN_CAROL_H
#include "tn_music.h"

enum { CM_MAGIC, CM_FOLLOW, CM_LISTEN, CM_COUNT };
typedef struct {
    uint8_t carol, mode, done;
    uint16_t i;                                 /* the next note */
    uint32_t pos;                               /* where it starts, in sixteenths from the beginning */
    uint16_t ci;                                /* the next chord */
    uint32_t cpos;                              /* where it starts */
    uint32_t t_next;                            /* CM_LISTEN: when the next note is due (ms) */
    uint32_t beat_next;                         /* the next beat, in sixteenths (sleigh bells for the jolly ones) */
} carol_play_t;
typedef struct {
    uint8_t mel, mel_len;                       /* a melody note (MIDI; 0 none) and its length in sixteenths */
    uint8_t chord, root, kind;                  /* a chord to strike (root 0..11, kind 0 major 1 minor 2 seventh) */
    uint8_t beat;                               /* a beat passed: 1, or 2 on the first beat of a bar */
    uint16_t len_ms;                            /* the melody note's length at the written tempo */
} carol_ev_t;

static uint32_t carol_unit_ms(const tn_carol_t *c)   /* a sixteenth, in ms */
{
    return c->six8 ? 10000u / c->tempo : 15000u / c->tempo;
}

static void carol_skip_rests(carol_play_t *p)
{
    const tn_carol_t *c = &TN_CAROLS[p->carol];
    while (p->i < c->n && !c->pitch[p->i]) { p->pos += c->len[p->i]; p->i++; }
    if (p->i >= c->n) p->done = 1;
}

static void carol_start(carol_play_t *p, uint32_t which, uint32_t mode, uint32_t now)
{
    memset(p, 0, sizeof *p);
    p->carol = (uint8_t)(which % TN_NCAROLS);
    p->mode = (uint8_t)(mode % CM_COUNT);
    p->t_next = now + 400u;
    p->beat_next = 0;
    carol_skip_rests(p);
}

/* the key (0..26) of the next note, or of the one after (ahead 1); -1 when there is none */
static int carol_next_key(const carol_play_t *p, int ahead)
{
    const tn_carol_t *c = &TN_CAROLS[p->carol];
    uint32_t i = p->i;
    if (p->done) return -1;
    while (ahead > 0 && i < c->n) { i++; while (i < c->n && !c->pitch[i]) i++; ahead--; }
    return i < c->n ? (int)c->pitch[i] - 53 : -1;
}

/* the note now: into ev, and on to the next; the chords and beats it passes */
static void carol_advance(carol_play_t *p, carol_ev_t *ev)
{
    const tn_carol_t *c = &TN_CAROLS[p->carol];
    uint32_t end, bar = c->bar;
    memset(ev, 0, sizeof *ev);
    if (p->done) return;
    ev->mel = c->pitch[p->i];
    ev->mel_len = c->len[p->i];
    ev->len_ms = (uint16_t)(c->len[p->i] * carol_unit_ms(c));
    while (p->ci < c->nchord && p->cpos <= p->pos) {   /* the chord under this note (the latest one reached) */
        ev->chord = 1;
        ev->root = c->chord[2u * p->ci] & 15u;
        ev->kind = c->chord[2u * p->ci] >> 4;
        p->cpos += c->chord[2u * p->ci + 1u];
        p->ci++;
    }
    end = p->pos + c->len[p->i];
    while (p->beat_next < end) {                 /* the beats inside it (the bars counted from after the pickup) */
        uint32_t from_bar = p->beat_next + (c->pickup ? bar - c->pickup : 0u);
        if (p->beat_next >= p->pos) ev->beat = (uint8_t)(from_bar % bar == 0u ? 2u : ev->beat ? ev->beat : 1u);
        p->beat_next += c->six8 ? 6u : 4u;
    }
    p->pos = end;
    p->i++;
    carol_skip_rests(p);
}

/* a key pressed: 1 if the carol moved on (and ev holds what to play) */
static int carol_key(carol_play_t *p, uint32_t key, carol_ev_t *ev)
{
    memset(ev, 0, sizeof *ev);
    if (p->done || p->mode == CM_LISTEN) return 0;
    if (p->mode == CM_FOLLOW && (int)key != carol_next_key(p, 0)) return 0;
    carol_advance(p, ev);
    return 1;
}

/* CM_LISTEN: 1 when a note is due now */
static int carol_tick(carol_play_t *p, uint32_t now, carol_ev_t *ev)
{
    const tn_carol_t *c = &TN_CAROLS[p->carol];
    uint32_t len;
    memset(ev, 0, sizeof *ev);
    if (p->done || p->mode != CM_LISTEN || (int32_t)(now - p->t_next) < 0) return 0;
    len = c->len[p->i] * carol_unit_ms(c);
    {                                            /* (a rest after this note: its time too) */
        uint32_t j = p->i + 1u;
        while (j < c->n && !c->pitch[j]) { len += c->len[j] * carol_unit_ms(c); j++; }
    }
    carol_advance(p, ev);
    p->t_next += len;
    return 1;
}

/* a chord's notes, as MIDI near the given floor: root, third, fifth (and the seventh) */
static uint32_t carol_chord_notes(uint32_t root, uint32_t kind, uint32_t floor, uint8_t *out)
{
    uint32_t r = floor + ((root + 12u - floor % 12u) % 12u), n = 0;
    out[n++] = (uint8_t)r;
    out[n++] = (uint8_t)(r + (kind == 1u ? 3u : 4u));
    out[n++] = (uint8_t)(r + 7u);
    if (kind == 2u) out[n++] = (uint8_t)(r + 10u);
    return n;
}
#endif
