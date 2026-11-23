/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel's pictures, drawn in code: big, chunky, readable at arm's length by someone who cannot read yet.
 * pic_draw(which, cx, cy, s, now): s is the size (about the picture's half width, in pixels). */
#ifndef TN_PICS_H
#define TN_PICS_H

/* (the PIC_ names: tn_ui.h) */

/* the little sprites, drawn in text */
static const char *const ROBIN_ROWS[] = {
    "....kkkk....",
    "...kkkkkk...",
    "..kkkkekkk..",
    "..kkkkkkkkoo",
    ".kkrrrrkkk..",
    "kkrrrrrrkk..",
    "kkrrrrrrkk..",
    ".kkrrrrkk...",
    "..kkkkkk....",
    "...d..d.....",
};
static const uint8_t ROBIN_COLS[] = {P_BROWN, P_RED, P_BLACK, P_ORANGE, P_BROWN_D};
static const g_sprite_t ROBIN = {ROBIN_ROWS, 12, 10, "krebd", ROBIN_COLS};
static const char *const DEER_ROWS[] = {
    "..a.a...........",
    "...a............",
    "..bbb...........",
    ".bbnb...........",
    "..bb............",
    "...bbbbbbbbbb...",
    "...bbbbbbbbbbb..",
    "....bbbbbbbbb.t.",
    "....b.b...b.b...",
    "...b...b.b...b..",
};
static const uint8_t DEER_COLS[] = {P_BROWN_D, P_BROWN, P_RED, P_SNOW};
static const g_sprite_t DEER = {DEER_ROWS, 16, 10, "abnt", DEER_COLS};


static void pic_bell(int32_t cx, int32_t cy, int32_t s, uint32_t c, uint32_t dark)
{
    g_disc(cx, cy - s / 3, s / 2, c);
    g_rect(cx - s / 2, cy - s / 3, s + 1, s / 2, c);
    g_tri_up(cx, cy - s / 3, cx - s * 3 / 4, cx + s * 3 / 4, cy + s / 3, c);
    g_rect(cx - s * 3 / 4, cy + s / 4, s * 3 / 2 + 1, s / 6 + 1, c);
    g_rect(cx - s * 3 / 4, cy + s / 4 + s / 6, s * 3 / 2 + 1, 2, dark);
    g_disc(cx, cy + s / 2, s / 6 + 1, dark);
    g_disc(cx, cy - s / 3 - s / 2, s / 8 + 1, dark);
}

static void pic_star(int32_t cx, int32_t cy, int32_t s, uint32_t c)
{
    g_tri_up(cx, cy - s, cx - s * 3 / 5, cx + s * 3 / 5, cy + s / 4, c);
    g_rect(cx - s, cy - s / 4, 2 * s + 1, s / 4 + 1, c);
    g_tri_up(cx, cy - s / 2, cx - s, cx + s, cy, c);
    g_tri_up(cx - s * 3 / 5, cy, cx - s * 3 / 4, cx - s / 8, cy + s, c);
    g_tri_up(cx + s * 3 / 5, cy, cx + s / 8, cx + s * 3 / 4, cy + s, c);
    g_tri_up(cx, cy - s / 4, cx - s * 2 / 3, cx + s * 2 / 3, cy + s / 2, c);
}

static void pic_holly_leaf(int32_t cx, int32_t cy, int32_t s, int flip)
{
    int32_t d = flip ? -1 : 1, i;
    for (i = 0; i < 3; i++) g_disc(cx + d * (s / 4 + i * s / 4), cy - i * s / 8, s / 5, P_GREEN_D);
    for (i = 0; i < 3; i++) g_disc(cx + d * (s / 4 + i * s / 4), cy - i * s / 8, s / 7, P_GREEN);
}

/* "press PLAY": a pulsing gold button with a play arrow, and the word under it (every screen that waits to start) */
static void ui_play_sign(int32_t cx, int32_t cy, uint32_t now, int word)
{
    int32_t r = 13 + (int32_t)(tn_pulse(now, 0, 4)), y;
    g_disc(cx, cy, r + 2, P_GOLD_D);
    g_disc(cx, cy, r, P_GOLD);
    for (y = -7; y <= 7; y++) g_hline(cx - 4, cx - 4 + (7 - (y < 0 ? -y : y)) * 10 / 7, cy + y, P_RED_D);
    if (word) g_text_c(cx, cy + r + 4, &FONT_S, "PLAY", P_GOLD, P_GOLD_AA);
}

/* "HOME for more": a little sleigh and the word, in a corner of the Grotto */
static void ui_home_hint(int32_t x, int32_t y, uint32_t now)
{
    int32_t bob = (int32_t)((now / 400u) & 1u);
    g_rect(x, y + 8 + bob, 22, 7, P_RED);
    g_rect(x + 16, y + 2 + bob, 6, 8, P_RED);
    g_rect(x - 2, y + 15 + bob, 28, 2, P_GOLD);
    g_rect(x + 4, y + 3 + bob, 8, 5, P_GREEN);   /* presents in it */
    g_rect(x + 7, y + 1 + bob, 2, 7, P_GOLD);
    g_text(x - 4, y + 19, &FONT_S, "HOME", P_GOLD, P_GOLD_AA);
}

static void pic_draw(uint32_t which, int32_t cx, int32_t cy, int32_t s, uint32_t now)
{
    int32_t wob = (int32_t)((now / 120u) % 4u) - 1;
    switch (which) {
    case PIC_BELLS:
        pic_bell(cx - s / 3, cy + wob, s * 2 / 3, P_GOLD, P_GOLD_D);
        pic_bell(cx + s / 3, cy - wob, s * 2 / 3, P_GOLD, P_GOLD_D);
        g_disc(cx, cy - s * 2 / 3, s / 5, P_RED);
        g_tri_up(cx - s / 4, cy - s * 2 / 3, cx - s / 2, cx - s / 8, cy - s / 3, P_RED);
        g_tri_up(cx + s / 4, cy - s * 2 / 3, cx + s / 8, cx + s / 2, cy - s / 3, P_RED);
        break;
    case PIC_PUDDING:
        g_disc(cx, cy + s / 6, s * 3 / 4, P_BROWN);
        g_rect(cx - s, cy + s / 3, 2 * s + 1, s, P_NIGHT);   /* (a flat bottom) */
        g_rect(cx - s * 3 / 4 - 4, cy + s / 3, s * 3 / 2 + 9, 5, P_SNOW_SH);   /* the plate */
        g_disc(cx, cy - s / 4, s / 2, P_SNOW);   /* the custard */
        g_disc(cx - s / 3, cy, s / 6, P_SNOW);
        g_disc(cx + s / 3, cy - s / 20, s / 6, P_SNOW);
        pic_holly_leaf(cx, cy - s * 2 / 3, s / 2, 0);
        pic_holly_leaf(cx, cy - s * 2 / 3, s / 2, 1);
        g_disc(cx, cy - s * 2 / 3, s / 8 + 1, P_RED);
        break;
    case PIC_HOLLY:
        pic_holly_leaf(cx - s / 6, cy + s / 6, s * 3 / 2, 1);
        pic_holly_leaf(cx + s / 6, cy + s / 6, s * 3 / 2, 0);
        g_disc(cx - s / 6, cy, s / 5, P_RED);
        g_disc(cx + s / 6, cy, s / 5, P_RED);
        g_disc(cx, cy - s / 5, s / 5, P_RED);
        g_pset(cx - s / 6 - 2, cy - 2, P_WHITE);
        break;
    case PIC_STAR: pic_star(cx, cy, s, (now / 300u) & 1u ? P_GOLD : P_YELLOW); break;
    case PIC_CROWN:
        g_rect(cx - s * 3 / 4, cy - s / 6, s * 3 / 2 + 1, s * 2 / 3, P_GOLD);
        g_tri_up(cx - s * 3 / 4 + s / 4, cy - s * 3 / 4, cx - s * 3 / 4, cx - s / 4, cy - s / 6, P_GOLD);
        g_tri_up(cx, cy - s, cx - s / 4, cx + s / 4, cy - s / 6, P_GOLD);
        g_tri_up(cx + s * 3 / 4 - s / 4, cy - s * 3 / 4, cx + s / 4, cx + s * 3 / 4, cy - s / 6, P_GOLD);
        g_disc(cx, cy + s / 6, s / 7, P_RED);
        g_disc(cx - s / 2, cy + s / 6, s / 9, P_BLUE);
        g_disc(cx + s / 2, cy + s / 6, s / 9, P_GREEN);
        g_rect(cx - s * 3 / 4, cy + s / 2 - 2, s * 3 / 2 + 1, 3, P_GOLD_D);
        break;
    case PIC_ANGEL:
        g_tri_up(cx, cy - s / 3, cx - s / 2, cx + s / 2, cy + s, P_SNOW);   /* the gown */
        g_disc(cx - s / 2, cy - s / 6, s / 3, P_SNOW_SH);   /* wings */
        g_disc(cx + s / 2, cy - s / 6, s / 3, P_SNOW_SH);
        g_tri_up(cx, cy - s / 4, cx - s / 3, cx + s / 3, cy + s * 3 / 4, P_SNOW);
        g_disc(cx, cy - s / 2, s / 4, P_SKIN);
        g_ring(cx, cy - s * 5 / 6, s / 4, P_GOLD);
        g_hline(cx - s / 4, cx + s / 4, cy - s * 5 / 6, P_GOLD);
        break;
    case PIC_CANDLE:
        g_rect(cx - s / 4, cy - s / 3, s / 2 + 1, s + s / 3, P_RED);
        g_rect(cx - s / 4, cy - s / 3, s / 8 + 1, s + s / 3, P_RED_D);
        g_disc(cx, cy - s / 2 - (int32_t)((now / 90u) % 3u), s / 6 + 1, P_ORANGE);
        g_disc(cx, cy - s / 2, s / 10 + 1, P_YELLOW);
        pic_holly_leaf(cx, cy + s, s * 2 / 3, 0);
        pic_holly_leaf(cx, cy + s, s * 2 / 3, 1);
        break;
    case PIC_BELL: pic_bell(cx + wob, cy, s, P_GOLD, P_GOLD_D); break;
    case PIC_TREE:
        g_tri_up(cx, cy - s, cx - s / 2, cx + s / 2, cy - s / 5, P_GREEN);
        g_tri_up(cx, cy - s * 2 / 3, cx - s * 3 / 4, cx + s * 3 / 4, cy + s / 3, P_GREEN);
        g_tri_up(cx, cy - s / 5, cx - s, cx + s, cy + s * 3 / 4, P_GREEN);
        g_rect(cx - s / 8, cy + s * 3 / 4, s / 4 + 1, s / 4, P_BROWN);
        g_disc(cx - s / 3, cy, s / 9 + 1, P_BAUBLE_LIT + 0);
        g_disc(cx + s / 4, cy + s / 3, s / 9 + 1, P_BAUBLE_LIT + 2);
        g_disc(cx + s / 8, cy - s / 2, s / 9 + 1, P_BAUBLE_LIT + 1);
        g_disc(cx - s / 2, cy + s / 2, s / 9 + 1, P_BAUBLE_LIT + 4);
        pic_star(cx, cy - s, s / 5, P_GOLD);
        break;
    case PIC_BOOK:
        g_rect(cx - s, cy - s * 2 / 3, s, s * 4 / 3, P_RED);
        g_rect(cx + 1, cy - s * 2 / 3, s, s * 4 / 3, P_RED);
        g_rect(cx - s + 4, cy - s * 2 / 3 + 4, s - 6, s * 4 / 3 - 8, P_SNOW);
        g_rect(cx + 3, cy - s * 2 / 3 + 4, s - 6, s * 4 / 3 - 8, P_SNOW);
        {
            int32_t i;
            for (i = 0; i < 4; i++) {            /* notes on the pages */
                g_disc(cx - s * 3 / 4 + i * s / 5, cy + s / 4 - (i & 1) * s / 4, s / 12 + 1, P_BLACK);
                g_rect(cx - s * 3 / 4 + i * s / 5 + s / 12, cy - s / 3 - (i & 1) * s / 4, 1, s / 2, P_BLACK);
                g_disc(cx + s / 4 + i * s / 6, cy + s / 6 - (i % 3) * s / 8, s / 12 + 1, P_BLACK);
            }
        }
        g_rect(cx - 1, cy - s * 2 / 3, 3, s * 4 / 3, P_RED_D);
        break;
    case PIC_PRESENT:
    case PIC_WRAPPED: {
        int32_t lift = which == PIC_WRAPPED ? wob : 0;
        uint32_t c = which == PIC_WRAPPED ? P_PURPLE : P_RED;
        g_rect(cx - s * 3 / 4, cy - s / 2 + lift, s * 3 / 2 + 1, s + s / 4, c);
        g_rect(cx - s * 3 / 4 - 3, cy - s / 2 - s / 5 + lift, s * 3 / 2 + 7, s / 4, c);
        g_rect(cx - s / 8, cy - s / 2 - s / 5 + lift, s / 4 + 1, s + s / 2, P_GOLD);
        g_disc(cx - s / 4, cy - s * 3 / 4 + lift, s / 5, P_GOLD);
        g_disc(cx + s / 4, cy - s * 3 / 4 + lift, s / 5, P_GOLD);
        if (which == PIC_WRAPPED) g_text_c(cx, cy + s * 3 / 4 + 6, &FONT_S, "SOON", P_GREY, P_DIM);
        break;
    }
    case PIC_HOUSE: {
        int32_t i;
        g_rect(cx - s * 3 / 4, cy - s / 4, s * 3 / 2 + 1, s + s / 4, P_RED_D);
        g_tri_up(cx, cy - s, cx - s, cx + s, cy - s / 4, P_ROOF);
        g_rect(cx - s, cy - s / 4 - 3, 2 * s + 1, 4, P_SNOW);
        for (i = 0; i < 4; i++) g_rect(cx - s / 2 + (i % 2) * s * 2 / 3, cy + (i / 2) * s / 2 - s / 8, s / 4, s / 4, (now / 400u + (uint32_t)i) % 3u ? P_WINDOW : P_YELLOW);
        g_text_c(cx, cy - s / 2 - 4, &FONT_S, "24", P_GOLD, P_GOLD_AA);
        break;
    }
    case PIC_WAND:
        g_line(cx - s / 2, cy + s / 2, cx + s / 4, cy - s / 4, P_BROWN);
        g_line(cx - s / 2 + 1, cy + s / 2, cx + s / 4 + 1, cy - s / 4, P_BROWN);
        pic_star(cx + s / 3, cy - s / 3, s / 3, P_GOLD);
        g_pset(cx + s * 3 / 4, cy - s * 3 / 4, P_WHITE);
        g_pset(cx + s / 8, cy - s * 3 / 4, P_WHITE);
        break;
    case PIC_FOLLOW: {                           /* keys with one lit */
        int32_t i;
        for (i = 0; i < 5; i++) g_rect(cx - s + i * s * 2 / 5, cy - s / 3, s * 2 / 5 - 2, s, i == 2 ? P_GOLD : P_SNOW);
        pic_star(cx, cy - s * 2 / 3, s / 4, P_GOLD);
        break;
    }
    case PIC_MUSICBOX:
        g_rect(cx - s, cy - s / 3, 2 * s + 1, s, P_BROWN);
        g_rect(cx - s, cy - s / 3, 2 * s + 1, s / 6 + 1, P_BROWN_D);
        g_tri_up(cx, cy - s, cx - s, cx + s, cy - s / 3, P_BROWN_D);   /* the lid, open */
        g_rect(cx - s * 3 / 4, cy - s / 8, s * 3 / 2, s / 4, P_GOLD);   /* the cylinder */
        g_line(cx + s, cy, cx + s + s / 3, cy - s / 4 + (int32_t)((now / 150u) % 3u) * 2, P_GREY);   /* the crank */
        g_disc(cx + s + s / 3, cy - s / 4 + (int32_t)((now / 150u) % 3u) * 2, s / 10 + 1, P_RED);
        g_text_c(cx + s / 2, cy - s - 6 - (int32_t)((now / 200u) % 4u), &FONT_S, "*", P_YELLOW, 0xFFu);
        break;
    case PIC_GLOBE:
        g_disc(cx, cy - s / 6, s * 3 / 4, P_SNOW_SH);
        g_disc(cx, cy - s / 6, s * 3 / 4 - 2, P_NIGHT3);
        g_tri_up(cx, cy - s * 2 / 3, cx - s / 3, cx + s / 3, cy + s / 6, P_GREEN);
        {
            int32_t i;
            for (i = 0; i < 10; i++) g_pset(cx - s / 2 + (i * 37) % s, cy - s / 2 + (i * 23 + (int32_t)(now / 60u)) % (s * 3 / 4), P_SNOW);
        }
        g_rect(cx - s * 2 / 3, cy + s / 2, s * 4 / 3, s / 3, P_RED_D);
        break;
    case PIC_SNOWBALL:
        g_rect(cx - s, cy + s / 2, 2 * s + 1, s / 4, P_ROOF);
        g_disc(cx + (int32_t)((now / 20u) % (uint32_t)s) - s / 2, cy - s / 4, s / 4, P_SNOW);
        g_rect(cx - s, cy - s / 2, s / 6 + 1, s * 2 / 3, P_RED);
        g_rect(cx + s - s / 6, cy - s / 2, s / 6 + 1, s * 2 / 3, P_BLUE);
        break;
    case PIC_CRACKER:
        g_rect(cx - s * 2 / 3, cy - s / 5, s * 4 / 3, s * 2 / 5, P_RED);
        g_tri_up(cx - s, cy - s / 3, cx - s, cx - s * 2 / 3, cy + s / 3, P_RED_D);
        g_tri_up(cx + s, cy - s / 3, cx + s * 2 / 3, cx + s, cy + s / 3, P_RED_D);
        g_rect(cx - s * 2 / 3, cy - s / 5, s / 10 + 1, s * 2 / 5, P_GOLD);
        g_rect(cx + s * 2 / 3 - s / 10, cy - s / 5, s / 10 + 1, s * 2 / 5, P_GOLD);
        pic_star(cx, cy, s / 6 + 1, P_GOLD);
        break;
    case PIC_SLEIGH:
        g_rect(cx - s * 2 / 3, cy, s * 4 / 3, s / 3, P_RED);
        g_rect(cx + s / 2, cy - s / 3, s / 5, s / 2, P_RED);
        g_rect(cx - s * 3 / 4 - 2, cy + s / 3 + 2, s * 3 / 2 + 4, 3, P_GOLD);
        g_rect(cx - s / 2, cy - s / 4, s / 3, s / 4, P_GREEN);
        g_rect(cx - s / 8, cy - s / 6, s / 4, s / 6, P_BLUE);
        g_sprite(&DEER, cx - s - 26, cy - s / 2, 2, 0);
        break;
    case PIC_EAR:
        g_disc(cx, cy, s / 2, P_SKIN);
        g_disc(cx + s / 8, cy, s / 4, P_ORANGE);
        g_disc(cx + s / 6, cy, s / 8, P_SKIN);
        g_ring(cx + s / 2, cy, s * 2 / 3, P_GOLD);
        g_ring(cx + s / 2, cy, s * 2 / 3 + 1, P_GOLD);
        g_rect(cx - s, cy - s, s / 2 + s / 4, 2 * s, P_NIGHT);   /* (only the waves' right half) */
        g_disc(cx, cy, s / 2, P_SKIN);
        g_disc(cx + s / 8, cy, s / 4, P_ORANGE);
        g_disc(cx + s / 6, cy, s / 8, P_SKIN);
        break;
    }
}
#endif
