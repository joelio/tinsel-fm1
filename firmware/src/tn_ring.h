/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Change ringing, the rules (DESIGN section 7.4; docs/RESEARCH-NOTES.md section 2). Pure: the tests ring it.
 *
 * Bells strike once a row; between rows a bell moves at most one place. A method is place notation: 'x' swaps every
 * pair, a number lists the places that stay, '.' separates changes, "a,b" is a and a backwards without its last
 * change, then the same for b (CompLib's form). A lead ends when the treble leads again; a plain course comes round.
 * A bob or a single changes the last change(s) of a lead. */
#ifndef TN_RING_H
#define TN_RING_H

#define RING_MAXB 8u
#define RING_MAXCH 40u
enum { RM_ROUNDS, RM_HUNT5, RM_HUNT6, RM_BOB_DOUBLES, RM_BOB_MINOR, RM_GRANDSIRE, RM_COUNT };
enum { CALL_NONE, CALL_BOB, CALL_SINGLE };
typedef struct {
    const char *name, *pn, *bob, *single;       /* the lead, and what a bob / single puts in place of its last changes */
    uint8_t bells, call_n;                      /* bells; how many of the lead's last changes a call replaces */
} ring_method_t;
static const ring_method_t RING_METHODS[RM_COUNT] = {
    {"ROUNDS", "", "", "", 6, 0},
    {"PLAIN HUNT ON 5", "5.1", "", "", 5, 0},
    {"PLAIN HUNT ON 6", "x16", "", "", 6, 0},
    {"PLAIN BOB DOUBLES", "5.1.5.1.5,125", "145", "123", 5, 1},
    {"PLAIN BOB MINOR", "x16x16x16,12", "14", "1234", 6, 1},
    {"GRANDSIRE DOUBLES", "3,1.5.1.5.1", "3.1", "3.123", 5, 2},
};

typedef struct {
    uint8_t method, bells, nch, pos, call_next;
    uint8_t row[RING_MAXB];                     /* bell numbers 1..n in striking order */
    uint16_t ch[RING_MAXCH];                    /* the lead's changes: places that stay, bit 0 = place 1 (0: x) */
    uint16_t rows;                              /* rows rung since rounds */
    uint8_t rounds_again;                       /* the last change brought it back to rounds */
} ring_t;

/* "x16x16.14" -> changes; returns how many */
static uint32_t ring_parse(const char *s, uint16_t *out, uint32_t max)
{
    uint32_t n = 0, cur = 0, have = 0;
    for (;; s++) {
        char c = *s;
        if (c == 'x' || c == 'X' || c == '-') {
            if (have && n < max) out[n++] = (uint16_t)cur;
            if (n < max) out[n++] = 0;
            cur = 0; have = 0;
        } else if (c >= '1' && c <= '9') { cur |= 1u << (c - '1'); have = 1; }
        else if (c == 'E' || c == '0') { cur |= 1u << (c == '0' ? 9 : 11); have = 1; }
        else {                                   /* '.', ',' or the end */
            if (have && n < max) out[n++] = (uint16_t)cur;
            cur = 0; have = 0;
            if (!c || c == ',') break;
        }
    }
    return n;
}

/* the lead from CompLib's "a,b" (or a plain list) */
static uint32_t ring_lead(const char *pn, uint16_t *out)
{
    const char *comma = pn;
    uint32_t n = 0, a, i;
    uint16_t tmp[RING_MAXCH];
    while (*comma && *comma != ',') comma++;
    if (!*comma) return ring_parse(pn, out, RING_MAXCH);
    a = ring_parse(pn, tmp, RING_MAXCH);         /* a, then a backwards without its last */
    for (i = 0; i < a && n < RING_MAXCH; i++) out[n++] = tmp[i];
    for (i = a - 1u; i-- > 0u && n < RING_MAXCH;) out[n++] = tmp[i];
    a = ring_parse(comma + 1, tmp, RING_MAXCH);  /* the same for b */
    for (i = 0; i < a && n < RING_MAXCH; i++) out[n++] = tmp[i];
    for (i = a - 1u; i-- > 0u && n < RING_MAXCH;) out[n++] = tmp[i];
    return n;
}

static void ring_start(ring_t *r, uint32_t method)
{
    uint32_t i;
    memset(r, 0, sizeof *r);
    r->method = (uint8_t)(method % RM_COUNT);
    r->bells = RING_METHODS[r->method].bells;
    for (i = 0; i < r->bells; i++) r->row[i] = (uint8_t)(i + 1u);
    r->nch = (uint8_t)ring_lead(RING_METHODS[r->method].pn, r->ch);
}

static void ring_apply(ring_t *r, uint32_t stays)   /* one change */
{
    uint32_t i = 0, n = r->bells;
    while (i < n) {
        if (stays >> i & 1u) i++;
        else if (i + 1u < n && !(stays >> (i + 1u) & 1u)) { uint8_t t = r->row[i]; r->row[i] = r->row[i + 1u]; r->row[i + 1u] = t; i += 2u; }
        else i++;
    }
}

static int ring_is_rounds(const ring_t *r)
{
    uint32_t i;
    for (i = 0; i < r->bells; i++) if (r->row[i] != i + 1u) return 0;
    return 1;
}

/* the next row (rounds stays rounds); a call asked for applies at the next lead end */
static void ring_next(ring_t *r)
{
    const ring_method_t *m = &RING_METHODS[r->method];
    uint32_t stays;
    r->rounds_again = 0;
    if (!r->nch) { r->rows++; return; }          /* rounds (or call changes, done by ring_swap) */
    if (r->call_next && m->call_n && r->pos >= r->nch - m->call_n) {   /* the call's changes instead of the lead's last */
        uint16_t c[4];
        uint32_t k = ring_parse(r->call_next == CALL_BOB ? m->bob : m->single, c, 4);
        uint32_t j = r->pos - (r->nch - m->call_n);
        stays = j < k ? c[j] : 0u;
    } else
        stays = r->ch[r->pos];
    ring_apply(r, stays);
    r->rows++;
    if (++r->pos >= r->nch) { r->pos = 0; r->call_next = CALL_NONE; }
    r->rounds_again = (uint8_t)ring_is_rounds(r);
}

static void ring_call(ring_t *r, uint32_t call) { if (RING_METHODS[r->method].call_n) r->call_next = (uint8_t)call; }

/* call changes (in rounds): the bells in places p and p+1 (0-based) swap */
static int ring_swap(ring_t *r, uint32_t p)
{
    uint8_t t;
    if (r->nch || p + 1u >= r->bells) return 0;
    t = r->row[p]; r->row[p] = r->row[p + 1u]; r->row[p + 1u] = t;
    return 1;
}

/* a named row on six: 1 rounds, 2 Queens, 3 Kings, 4 Tittums, 5 back rounds, 0 none */
static uint32_t ring_named(const ring_t *r)
{
    static const uint8_t ROWS[5][6] = {{1, 2, 3, 4, 5, 6}, {1, 3, 5, 2, 4, 6}, {5, 3, 1, 2, 4, 6}, {1, 4, 2, 5, 3, 6}, {6, 5, 4, 3, 2, 1}};
    uint32_t k, i;
    if (r->bells != 6u) return 0;
    for (k = 0; k < 5u; k++) {
        for (i = 0; i < 6u && r->row[i] == ROWS[k][i]; i++) {}
        if (i == 6u) return k + 1u;
    }
    return 0;
}
static const char *const RING_NAMES[6] = {"", "ROUNDS", "QUEENS", "KINGS", "TITTUMS", "BACK ROUNDS"};
#endif
