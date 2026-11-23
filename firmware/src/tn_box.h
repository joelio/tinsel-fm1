/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* A music box's cylinder (DESIGN section 7.3): 64 steps (quavers), a pin per key per step. Pure: the tests use it.
 * The carol boxes are made from the carols' first 64 quavers; they and the kept boxes are play-only. */
#ifndef TN_BOX_H
#define TN_BOX_H

#define BOX_STEPS 64u
#define BOX_MAGIC 0x584F4254u                   /* "TBOX" */
typedef struct { uint32_t magic, n; uint32_t pin[BOX_STEPS]; } box_t;   /* n: pins, for the panel */

static void box_clear(box_t *b) { memset(b, 0, sizeof *b); b->magic = BOX_MAGIC; }
static int box_valid(const box_t *b)
{
    uint32_t i, n = 0;
    if (b->magic != BOX_MAGIC) return 0;
    for (i = 0; i < BOX_STEPS; i++) { if (b->pin[i] >> 27) return 0; n += tn_popcount(b->pin[i]); }
    return n == b->n;
}
static void box_pin(box_t *b, uint32_t step, uint32_t key)
{
    step %= BOX_STEPS;
    if (key >= 27u || (b->pin[step] >> key & 1u)) return;
    b->pin[step] |= 1u << key;
    b->n++;
}
static void box_from_carol(box_t *b, const tn_carol_t *c)   /* each note on the quaver it starts in */
{
    uint32_t i, pos = 0;
    box_clear(b);
    for (i = 0; i < c->n && pos / 2u < BOX_STEPS; i++) {
        if (c->pitch[i]) box_pin(b, pos / 2u, c->pitch[i] - 53u);
        pos += c->len[i];
    }
}
#endif
