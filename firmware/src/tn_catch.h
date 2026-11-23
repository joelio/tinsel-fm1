/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Present Catch, the rules (DESIGN section 8.1). Pure: the tests play it; tn_catchgame.h draws it and makes it sing.
 *
 * Presents fall towards seven chimneys. Each chimney has a group of keys (4 4 4 3 4 4 4, left to right: all 27);
 * the key under a falling present lights. A key of the right group, pressed while the present is in its window,
 * catches it.
 *   Little Elf  one at a time, slowly; caught any time; a present that reaches the bottom is caught by the snowman
 *               (it still counts); never ends, never speeds up
 *   Elf         two at a time; caught in the lower half; five misses end the round; a little faster with catches
 *   Santa       three at a time; caught near the chimney only; three misses end it; faster and faster */
#ifndef TN_CATCH_H
#define TN_CATCH_H

#define CATCH_LANES 7u
#define CATCH_FLOOR (160 << 8)                  /* how far a present falls, Q8 pixels */
static const uint8_t CATCH_LANE_FIRST[CATCH_LANES + 1] = {0, 4, 8, 12, 15, 19, 23, 27};
enum { LV_LITTLE, LV_ELF, LV_SANTA };

typedef struct { uint8_t on, lane, col; int32_t y; } catch_present_t;
typedef struct {
    uint8_t level, done, max_on;
    uint16_t score, misses, saved;              /* saved: caught by the snowman (Little Elf) */
    uint32_t seed, spawn_in;
    int32_t speed;                              /* Q8 pixels a second */
    catch_present_t p[4];
} catch_t;

static uint32_t catch_lane_of(uint32_t key)
{
    uint32_t l;
    for (l = 0; l < CATCH_LANES; l++) if (key < CATCH_LANE_FIRST[l + 1u]) return l;
    return CATCH_LANES - 1u;
}

static int32_t catch_window(const catch_t *g)   /* where catching starts, Q8 */
{
    return g->level == LV_LITTLE ? 0 : g->level == LV_ELF ? CATCH_FLOOR * 2 / 5 : CATCH_FLOOR * 7 / 10;
}

static void catch_start(catch_t *g, uint32_t level, uint32_t seed)
{
    memset(g, 0, sizeof *g);
    g->level = (uint8_t)(level > LV_SANTA ? LV_SANTA : level);
    g->seed = seed ? seed : 1u;
    g->max_on = (uint8_t)(g->level + 1u);
    g->speed = (g->level == LV_LITTLE ? 30 : g->level == LV_ELF ? 45 : 60) << 8;
    g->spawn_in = 600;
}

static void catch_spawn(catch_t *g)
{
    uint32_t i, on = 0, lane;
    for (i = 0; i < 4u; i++) on += g->p[i].on;
    if (on >= g->max_on) return;
    lane = tn_rand(&g->seed) % CATCH_LANES;
    for (i = 0; i < 4u; i++)                     /* (not on top of another in the same lane) */
        if (g->p[i].on && g->p[i].lane == lane && g->p[i].y < CATCH_FLOOR / 3) return;
    for (i = 0; i < 4u; i++)
        if (!g->p[i].on) {
            g->p[i].on = 1; g->p[i].lane = (uint8_t)lane; g->p[i].y = 0; g->p[i].col = (uint8_t)(tn_rand(&g->seed) % 5u);
            return;
        }
}

/* time passes; returns how many presents reached the bottom (missed, or saved by the snowman) */
static uint32_t catch_step(catch_t *g, uint32_t dt)
{
    uint32_t i, landed = 0;
    if (g->done) return 0;
    if (dt > 200u) dt = 200u;
    for (i = 0; i < 4u; i++) {
        if (!g->p[i].on) continue;
        g->p[i].y += (int32_t)((uint32_t)g->speed * dt / 1000u);
        if (g->p[i].y >= CATCH_FLOOR) {
            g->p[i].on = 0;
            landed++;
            if (g->level == LV_LITTLE) { g->saved++; g->score++; }
            else {
                g->misses++;
                if (g->misses >= (g->level == LV_ELF ? 5u : 3u)) g->done = 1;
            }
        }
    }
    if (g->spawn_in > dt) g->spawn_in -= dt;
    else { catch_spawn(g); g->spawn_in = g->level == LV_LITTLE ? 900u : g->level == LV_ELF ? 1300u : 900u; }
    return landed;
}

/* a key: 1 caught (the lowest present of its lane, if in the window), 0 not */
static int catch_key(catch_t *g, uint32_t key)
{
    uint32_t lane = catch_lane_of(key), i, best = 4;
    if (g->done) return 0;
    for (i = 0; i < 4u; i++)
        if (g->p[i].on && g->p[i].lane == lane && g->p[i].y >= catch_window(g) && (best == 4u || g->p[i].y > g->p[best].y)) best = i;
    if (best == 4u) return 0;
    g->p[best].on = 0;
    g->score++;
    if (g->level == LV_ELF && g->speed < (110 << 8)) g->speed += 2 << 8;
    if (g->level == LV_SANTA && g->speed < (160 << 8)) g->speed += 4 << 8;
    if (!g->spawn_in || g->spawn_in > 500u) g->spawn_in = 350;
    return 1;
}

/* the lane of the present nearest the bottom (its keys light), or -1 */
static int catch_lit_lane(const catch_t *g)
{
    uint32_t i;
    int best = -1;
    int32_t y = -1;
    for (i = 0; i < 4u; i++) if (g->p[i].on && g->p[i].y > y) { y = g->p[i].y; best = (int)g->p[i].lane; }
    return best;
}
#endif
