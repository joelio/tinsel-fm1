/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel's colours: night blue, snow white, tinsel gold, holly red and green (DESIGN section 12). Indices into
 * g_pal; pal_load(dim) sets them, dim for the sleepy Grotto. */
#ifndef TN_PAL_H
#define TN_PAL_H

enum {
    P_BLACK, P_NIGHT, P_NIGHT2, P_NIGHT3, P_SNOW, P_SNOW_SH, P_GOLD, P_GOLD_D, P_RED, P_RED_D, P_GREEN, P_GREEN_D,
    P_GREEN_DD, P_BROWN, P_BROWN_D, P_ORANGE, P_PINK, P_BLUE, P_PURPLE, P_WHITE, P_GREY, P_YELLOW, P_CYAN, P_DIM,
    P_MOON, P_SKIN, P_CARROT, P_WINDOW, P_WINDOW_D, P_ROOF, P_TEXT_AA, P_GOLD_AA,
    P_BAUBLE = 32,                              /* eight bauble colours, then the same lit */
    P_BAUBLE_LIT = 40,
    P_SKY = 48,                                 /* eight steps of sky, top to the horizon */
    P_COUNT = 56
};

static const uint8_t PAL_RGB[P_COUNT][3] = {
    {0, 0, 0}, {8, 14, 40}, {16, 26, 64}, {28, 40, 92}, {240, 248, 255}, {170, 190, 225}, {255, 204, 0}, {190, 128, 0},
    {222, 30, 44}, {136, 12, 24}, {40, 160, 70}, {16, 104, 48}, {8, 66, 34}, {120, 70, 30}, {74, 42, 18}, {255, 140, 30},
    {255, 120, 180}, {60, 130, 255}, {150, 80, 220}, {255, 255, 255}, {110, 120, 150}, {255, 236, 120}, {90, 220, 240},
    {70, 80, 120}, {250, 240, 196}, {255, 200, 160}, {255, 120, 20}, {255, 214, 110}, {90, 60, 40}, {150, 40, 40},
    {90, 100, 140}, {120, 90, 20},
    /* baubles: red, gold, blue, green, purple, pink, orange, silver */
    {170, 20, 30}, {170, 120, 0}, {30, 70, 170}, {20, 120, 50}, {100, 40, 150}, {180, 70, 120}, {190, 90, 10}, {130, 140, 160},
    {255, 70, 80}, {255, 230, 60}, {110, 180, 255}, {90, 240, 120}, {210, 130, 255}, {255, 160, 220}, {255, 180, 60}, {240, 250, 255},
    /* the sky */
    {4, 8, 28}, {6, 12, 36}, {8, 16, 44}, {12, 22, 54}, {16, 28, 64}, {22, 36, 76}, {30, 46, 90}, {40, 58, 104},
};

static void pal_load(uint32_t dim)              /* dim: 0 bright, 1..3 sleepier */
{
    uint32_t i;
    for (i = 0; i < P_COUNT; i++) {
        uint32_t r = PAL_RGB[i][0], g = PAL_RGB[i][1], b = PAL_RGB[i][2];
        if (dim) { r = r * 2u / (dim + 3u); g = g * 2u / (dim + 3u); b = b * 2u / (dim + 3u); }
        g_pal_set(i, r, g, b);
    }
}
#endif
