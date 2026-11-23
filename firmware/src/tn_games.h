/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The rules of Jingle It!, Cracker Pull, Snowball Rally and Sleigh Ride (DESIGN section 8). Pure: the tests play
 * them; their screens (tn_play.h) draw them and make them sing. Levels as Present Catch's (tn_catch.h): Little Elf
 * never fails and never speeds up; Elf is a gentle game; Santa is a proper one. */
#ifndef TN_GAMES_H
#define TN_GAMES_H

/* ---- Jingle It!: a call, and you do it --------------------------------------------------------------------- */
enum { JC_RING, JC_TWIST, JC_JINGLE, JC_SHH, JC_COUNT };
enum { JA_KEY, JA_TWIST, JA_BUTTON };
enum { JE_NONE, JE_CALL, JE_GOOD, JE_WRONG, JE_MISS, JE_OVER };
typedef struct {
    uint8_t level, done, call, key, lives, active;
    uint16_t score;
    uint32_t seed, t_call, window, t_next;
} jingle_t;

static void jingle_start(jingle_t *j, uint32_t level, uint32_t seed, uint32_t now)
{
    memset(j, 0, sizeof *j);
    j->level = (uint8_t)(level > LV_SANTA ? LV_SANTA : level);
    j->seed = seed ? seed : 1u;
    j->lives = 3;
    j->window = j->level == LV_ELF ? 3500u : 2500u;
    j->t_next = now + 900u;
}

/* time passes: a new call when it is due, a miss when the window closes (an SHH call is passed by waiting) */
static uint32_t jingle_tick(jingle_t *j, uint32_t now)
{
    if (j->done) return JE_NONE;
    if (!j->active) {
        if ((int32_t)(now - j->t_next) < 0) return JE_NONE;
        j->call = (uint8_t)(tn_rand(&j->seed) % (j->level == LV_LITTLE ? 3u : 4u));
        if (j->call == JC_SHH && j->score < 3u) j->call = JC_TWIST;   /* (not straight away) */
        j->key = (uint8_t)(tn_rand(&j->seed) % 27u);
        j->active = 1;
        j->t_call = now;
        return JE_CALL;
    }
    if (j->level == LV_LITTLE || now - j->t_call < j->window) return JE_NONE;
    j->active = 0;
    j->t_next = now + 700u;
    if (j->call == JC_SHH) { j->score++; return JE_GOOD; }
    if (--j->lives == 0u) { j->done = 1; return JE_OVER; }
    return JE_MISS;
}

static int jingle_right(const jingle_t *j, uint32_t act, uint32_t key)
{
    switch (j->call) {
    case JC_RING:
        if (act != JA_KEY) return 0;
        return j->level == LV_LITTLE || (j->level == LV_ELF ? catch_lane_of(key) == catch_lane_of(j->key) : key == j->key);
    case JC_TWIST: return act == JA_TWIST;
    case JC_JINGLE: return act == JA_BUTTON;
    default: return 0;
    }
}

/* something done: JE_GOOD, JE_WRONG (Little Elf and Elf: nothing lost), JE_MISS or JE_OVER (Santa; and any act in SHH) */
static uint32_t jingle_act(jingle_t *j, uint32_t act, uint32_t key, uint32_t now)
{
    if (j->done || !j->active) return JE_NONE;
    if (jingle_right(j, act, key)) {
        j->active = 0;
        j->score++;
        j->t_next = now + (j->level == LV_LITTLE ? 900u : 500u);
        if (j->level == LV_ELF && j->window > 1800u) j->window -= 100u;
        if (j->level == LV_SANTA && j->window > 1000u) j->window -= 80u;
        return JE_GOOD;
    }
    if (j->level == LV_LITTLE || (j->level == LV_ELF && j->call != JC_SHH)) return JE_WRONG;
    j->active = 0;
    j->t_next = now + 700u;
    if (--j->lives == 0u) { j->done = 1; return JE_OVER; }
    return JE_MISS;
}

/* ---- Cracker Pull: two pull, in time, and it goes BANG ------------------------------------------------------ */
#define CR_BEAT 600u
#define CR_BANG 60
typedef struct {
    uint8_t level, done, winner, two;           /* winner 0 left, 1 right, 2 a draw */
    int16_t pull[2];
    uint32_t t0, elf_next, seed;
} cracker_t;

static void cracker_start(cracker_t *c, uint32_t level, int two_players, uint32_t now)
{
    memset(c, 0, sizeof *c);
    c->level = (uint8_t)(level > LV_SANTA ? LV_SANTA : level);
    c->two = (uint8_t)(two_players != 0);
    c->t0 = now;
    c->elf_next = now + 800u;
    c->seed = now | 1u;
}
static int cracker_on_beat(const cracker_t *c, uint32_t now)
{
    uint32_t ph = (now - c->t0) % CR_BEAT;
    return ph < 110u || ph > CR_BEAT - 110u;
}
static void cracker_check(cracker_t *c)
{
    if (c->pull[0] + c->pull[1] < CR_BANG) return;
    c->done = 1;
    c->winner = (uint8_t)(c->pull[0] > c->pull[1] ? 0u : c->pull[1] > c->pull[0] ? 1u : 2u);
}
/* a tap on one side: how hard it pulled */
static uint32_t cracker_tap(cracker_t *c, uint32_t side, uint32_t now)
{
    uint32_t p;
    if (c->done) return 0;
    p = c->level == LV_LITTLE ? 2u : cracker_on_beat(c, now) ? 4u : 1u;
    c->pull[side & 1u] = (int16_t)(c->pull[side & 1u] + (int32_t)p);
    cracker_check(c);
    return p;
}
/* with one player the elf pulls the right-hand end; returns 1 when it tapped */
static int cracker_tick(cracker_t *c, uint32_t now)
{
    uint32_t gap;
    if (c->done || c->two || (int32_t)(now - c->elf_next) < 0) return 0;
    gap = c->level == LV_LITTLE ? 420u : c->level == LV_ELF ? CR_BEAT : CR_BEAT / 2u;
    c->elf_next = now + gap;
    if (c->level == LV_LITTLE) { c->pull[1] = (int16_t)(c->pull[1] + 2); cracker_check(c); return 1; }
    cracker_tap(c, 1, c->t0 + ((now - c->t0 + CR_BEAT / 2u) / CR_BEAT) * CR_BEAT);   /* the elf is on the beat */
    if (tn_rand(&c->seed) % 4u == 0u) c->elf_next += CR_BEAT / 3u;                /* (and sometimes dawdles) */
    return 1;
}

/* ---- Snowball Rally: a rooftop rally for two, a knob each --------------------------------------------------- */
#define RL_TOP 36
#define RL_BOT 232
#define RL_X0 12
#define RL_X1 228
enum { RE_NONE, RE_HIT_L, RE_HIT_R, RE_WALL, RE_MISS_L, RE_MISS_R };
typedef struct {
    int32_t bx, by, vx, vy;                     /* the snowball, Q8 pixels and Q8 pixels a second */
    int32_t py[2], half;                        /* the paddles' middles, their half height */
    uint8_t level, serve_to;
    uint16_t hits, snowmen[2];
    uint32_t seed, wait;                        /* ms before the next serve */
    int32_t speed;
} rally_t;

static void rally_serve(rally_t *r)
{
    int32_t dy = (int32_t)(tn_rand(&r->seed) % 120u) - 60;
    r->bx = 120 << 8; r->by = 134 << 8;
    r->vx = r->serve_to ? r->speed : -r->speed;
    r->vy = r->speed * dy / 120;
    r->wait = 0;
}
static void rally_start(rally_t *r, uint32_t level, uint32_t seed)
{
    memset(r, 0, sizeof *r);
    r->level = (uint8_t)(level > LV_SANTA ? LV_SANTA : level);
    r->seed = seed ? seed : 1u;
    r->half = r->level == LV_LITTLE ? 34 : r->level == LV_ELF ? 24 : 18;
    r->speed = (r->level == LV_LITTLE ? 80 : r->level == LV_ELF ? 120 : 140) << 8;
    r->py[0] = r->py[1] = 134;
    rally_serve(r);
    r->wait = 800;
}
static void rally_move(rally_t *r, uint32_t side, int32_t detents)
{
    int32_t *p = &r->py[side & 1u];
    *p += detents * 8;
    if (*p < RL_TOP + r->half) *p = RL_TOP + r->half;
    if (*p > RL_BOT - r->half) *p = RL_BOT - r->half;
}
/* the elf's paddle follows the ball, not quite perfectly */
static void rally_elf(rally_t *r, uint32_t side, uint32_t dt)
{
    int32_t want = r->by >> 8, step = (int32_t)dt * (r->level == LV_LITTLE ? 60 : r->level == LV_ELF ? 110 : 160) / 1000;
    if ((r->vx > 0) != (side == 1u)) want = 134;    /* (going away: back to the middle) */
    if (step < 1) step = 1;
    if (want > r->py[side] + 2) r->py[side] += step;
    else if (want < r->py[side] - 2) r->py[side] -= step;
    rally_move(r, side, 0);
}
static uint32_t rally_step(rally_t *r, uint32_t dt)
{
    int32_t x, y;
    if (dt > 100u) dt = 100u;
    if (r->wait) { r->wait = r->wait > dt ? r->wait - dt : 0u; if (!r->wait) rally_serve(r); return RE_NONE; }
    r->bx += r->vx * (int32_t)dt / 1000;
    r->by += r->vy * (int32_t)dt / 1000;
    y = r->by >> 8;
    if (y < RL_TOP + 3) { r->by = (RL_TOP + 3) << 8; r->vy = -r->vy; return RE_WALL; }
    if (y > RL_BOT - 3) { r->by = (RL_BOT - 3) << 8; r->vy = -r->vy; return RE_WALL; }
    x = r->bx >> 8;
    if ((x <= RL_X0 && r->vx < 0) || (x >= RL_X1 && r->vx > 0)) {
        uint32_t side = x >= RL_X1;
        int32_t off = y - r->py[side];
        if (off >= -r->half - 3 && off <= r->half + 3) {   /* a hit: back it goes, the angle from where it struck */
            r->vx = -r->vx;
            r->vy = r->speed * off / (r->half + 3) * 3 / 4;
            r->bx = (side ? RL_X1 - 1 : RL_X0 + 1) << 8;
            r->hits++;
            if (r->level == LV_SANTA && r->speed < (300 << 8)) {
                r->speed += 8 << 8;
                r->vx += r->vx > 0 ? 8 << 8 : -(8 << 8);
            }
            return side ? RE_HIT_R : RE_HIT_L;
        }
        r->snowmen[side]++;                      /* a miss: a snowman on that side (nobody loses, it is a snowman) */
        r->serve_to = (uint8_t)side;
        r->wait = 1000;
        r->vx = r->vy = 0;
        return side ? RE_MISS_R : RE_MISS_L;
    }
    return RE_NONE;
}

/* ---- Sleigh Ride: steer, and ring the bell posts as they pass ---------------------------------------------- */
#define RD_FAR 1000
#define RD_NPOST 5u
enum { RD_NONE, RD_RUNG, RD_MISSED, RD_OVER };
typedef struct {
    int32_t x;                                  /* the sleigh, -100..100 */
    int32_t speed;                              /* z a second */
    struct { uint8_t on, lane, rung; int32_t z; } post[RD_NPOST];
    uint8_t level, done, lives;
    uint16_t score;
    uint32_t seed, spawn;
} ride_t;

static int32_t ride_post_x(uint32_t lane) { return -90 + (int32_t)lane * 30; }
static void ride_start(ride_t *d, uint32_t level, uint32_t seed)
{
    memset(d, 0, sizeof *d);
    d->level = (uint8_t)(level > LV_SANTA ? LV_SANTA : level);
    d->seed = seed ? seed : 1u;
    d->lives = 3;
    d->speed = d->level == LV_LITTLE ? 260 : d->level == LV_ELF ? 360 : 440;
    d->spawn = 600;
}
static void ride_steer(ride_t *d, int32_t detents)
{
    d->x += detents * 12;
    if (d->x < -100) d->x = -100;
    if (d->x > 100) d->x = 100;
}
static int ride_near(const ride_t *d, uint32_t i)   /* close enough to ring (in z, and in x for Elf and Santa) */
{
    int32_t dx = d->x - ride_post_x(d->post[i].lane);
    if (d->post[i].z > 220 || d->post[i].z < -20) return 0;
    if (dx < 0) dx = -dx;
    return d->level == LV_LITTLE || dx <= (d->level == LV_ELF ? 40 : 26);
}
static uint32_t ride_step(ride_t *d, uint32_t dt)
{
    uint32_t i, ev = RD_NONE;
    if (d->done) return RD_NONE;
    if (dt > 100u) dt = 100u;
    for (i = 0; i < RD_NPOST; i++) {
        if (!d->post[i].on) continue;
        d->post[i].z -= d->speed * (int32_t)dt / 1000;
        if (d->post[i].z < -40) {
            d->post[i].on = 0;
            if (!d->post[i].rung && d->level != LV_LITTLE) {
                ev = RD_MISSED;
                if (--d->lives == 0u) { d->done = 1; return RD_OVER; }
            }
        }
    }
    if (d->spawn > dt) d->spawn -= dt;
    else {
        for (i = 0; i < RD_NPOST; i++)
            if (!d->post[i].on) {
                d->post[i].on = 1; d->post[i].rung = 0; d->post[i].z = RD_FAR;
                d->post[i].lane = (uint8_t)(tn_rand(&d->seed) % CATCH_LANES);
                break;
            }
        d->spawn = d->level == LV_LITTLE ? 1800u : d->level == LV_ELF ? 1300u : 1000u;
    }
    return ev;
}
/* a key: rings the nearest post it may ring (its lane's keys, or any key for Little Elf); returns the post + 1 */
static int ride_key(ride_t *d, uint32_t key)
{
    uint32_t i, best = RD_NPOST;
    for (i = 0; i < RD_NPOST; i++)
        if (d->post[i].on && !d->post[i].rung && ride_near(d, i) && (d->level == LV_LITTLE || catch_lane_of(key) == d->post[i].lane))
            if (best == RD_NPOST || d->post[i].z < d->post[best].z) best = i;
    if (best == RD_NPOST) return 0;
    d->post[best].rung = 1;
    d->score++;
    if (d->level == LV_SANTA && d->speed < 800) d->speed += 15;
    return (int)best + 1;
}
static int ride_lit_lane(const ride_t *d)       /* the lane of the nearest post not yet rung */
{
    uint32_t i;
    int best = -1;
    int32_t z = 1 << 30;
    for (i = 0; i < RD_NPOST; i++)
        if (d->post[i].on && !d->post[i].rung && d->post[i].z > -20 && d->post[i].z < z) { z = d->post[i].z; best = (int)d->post[i].lane; }
    return best;
}
#endif
