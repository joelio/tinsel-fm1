/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* What every screen gets: the panel as it is this pass, and the rules that hold everywhere (DESIGN section 6).
 * Pure: the tests include it. */
#ifndef TN_UI_H
#define TN_UI_H
#include <stdint.h>

/* the buttons by id (docs/API.md) */
enum { TB_OCT_DOWN, TB_OCT_UP, TB_FX, TB_SEL, TB_ENV, TB_LFO, TB_EDIT, TB_GLO, TB_HOME, TB_SAVE, TB_ARP, TB_SEQ, TB_PLAY, TB_REC };
enum { TE_SELECT, TE_ALGORITHM, TE_KNOB1, TE_KNOB2, TE_KNOB3, TE_KNOB4, TE_PRESETS };
#define TN_NKEYS 27u
#define TN_KEY_LED(k) (14u + (k))               /* (as platform.c's KEY_LED on the FM-1) */
#define TN_NLED 41u

typedef struct {
    uint32_t keys, key_down, key_up;            /* bit k = key k (0 = F3 .. 26 = G5) */
    uint32_t btn, btn_down, btn_up;             /* bit = TB_... */
    int32_t enc[7];                             /* detents this pass, + = clockwise */
    int32_t enc_x, enc_y;                       /* the knobs as a cursor: left-right, up-down (any knob of each group) */
    uint32_t now;                               /* ms */
} tn_input_t;

static uint32_t tn_popcount(uint32_t x) { uint32_t n = 0; while (x) { x &= x - 1u; n++; } return n; }
static int tn_lowest(uint32_t x) { int i; for (i = 0; i < 32; i++) if (x >> i & 1u) return i; return -1; }

/* the knobs as a cursor: SELECT, KNOB1, PRESETS move it left-right; ALGORITHM, KNOB2..4 up-down (a small hand grabs
 * whichever is nearest, and every one of them does something) */
static void tn_cursor(tn_input_t *in)
{
    in->enc_x = in->enc[TE_SELECT] + in->enc[TE_KNOB1] + in->enc[TE_PRESETS];
    in->enc_y = in->enc[TE_ALGORITHM] + in->enc[TE_KNOB2] + in->enc[TE_KNOB3] + in->enc[TE_KNOB4];
}
static int32_t tn_enc_sum(const tn_input_t *in) { int32_t s = 0, i; for (i = 0; i < 7; i++) s += in->enc[i]; return s; }

/* the grown-up gesture: EDIT and SAVE both held, without a break, for 2 s. Fires once per hold. */
#define TN_GESTURE_MS 2000u
typedef struct { uint32_t since; uint8_t held, fired; } gesture_t;
static int gesture_step(gesture_t *g, uint32_t btn, uint32_t now)
{
    uint32_t both = (btn >> TB_EDIT & 1u) && (btn >> TB_SAVE & 1u);
    if (!both) { g->held = 0; g->fired = 0; return 0; }
    if (!g->held) { g->held = 1; g->since = now; }
    if (!g->fired && now - g->since >= TN_GESTURE_MS) { g->fired = 1; return 1; }
    return 0;
}

/* deterministic little random numbers (xorshift32), seeded where they are used */
static uint32_t tn_rand(uint32_t *s) { uint32_t x = *s ? *s : 0x9E3779B9u; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x; }

/* the pictures (drawn in tn_pics.h) */
enum {
    PIC_BELLS, PIC_PUDDING, PIC_HOLLY, PIC_STAR, PIC_CROWN, PIC_ANGEL, PIC_CANDLE, PIC_BELL,   /* the carols' (CP_...) */
    PIC_TREE, PIC_BOOK, PIC_PRESENT, PIC_HOUSE, PIC_WRAPPED, PIC_WAND, PIC_EAR, PIC_FOLLOW, PIC_COUNT
};

/* a number as text (no printf on the chip) */
static char *tn_utoa(char *b, uint32_t v)
{
    char t[11];
    uint32_t n = 0, i;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 10u);
    for (i = 0; i < n; i++) b[i] = t[n - 1u - i];
    b[n] = 0;
    return b + n;
}

/* black keys among the 27, F3 = 0 */
static int tn_black(uint32_t k) { return (0x54Au >> ((k + 5u) % 12u)) & 1u; }
#endif
