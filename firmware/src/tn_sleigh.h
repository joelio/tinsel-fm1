/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Sleigh: the menu (DESIGN section 6). A row of big pictures; any knob slides it, each has its own little
 * jingle so a child learns them by ear; any key or PLAY opens it. Things still to come are wrapped presents. */
#ifndef TN_SLEIGH_H
#define TN_SLEIGH_H

typedef struct { uint8_t pic, scene; const char *name; uint8_t tune[3]; } sl_item_t;
static const sl_item_t SL_ITEMS[] = {
    {PIC_TREE, SC_GROTTO, "THE GROTTO", {72, 76, 79}},
    {PIC_BOOK, SC_CAROL, "CAROL BOOK", {79, 77, 76}},
    {PIC_PRESENT, SC_CATCH, "PRESENT CATCH", {67, 72, 79}},
    {PIC_HOUSE, SC_ADVENT, "ADVENT", {84, 79, 84}},
    {PIC_WRAPPED, 255, "MUSIC BOX", {0, 0, 0}},
    {PIC_WRAPPED, 255, "BELL TOWER", {0, 0, 0}},
    {PIC_WRAPPED, 255, "JINGLE IT!", {0, 0, 0}},
    {PIC_WRAPPED, 255, "SNOW GLOBE", {0, 0, 0}},
    {PIC_WRAPPED, 255, "SNOWBALL RALLY", {0, 0, 0}},
    {PIC_WRAPPED, 255, "CRACKER PULL", {0, 0, 0}},
    {PIC_WRAPPED, 255, "SLEIGH RIDE", {0, 0, 0}},
};
#define SL_N (sizeof SL_ITEMS / sizeof SL_ITEMS[0])
static struct { uint8_t at; int32_t dir; uint32_t t_move, t_nope; } sl;

static void sl_jingle(uint32_t i)
{
    const sl_item_t *it = &SL_ITEMS[i % SL_N];
    uint32_t k;
    if (it->scene == 255u) { snd_ping(TI_TOY, 72, 60, 0); return; }
    for (k = 0; k < 3u; k++) snd_ping_at(k * 110u, TI_CELESTA, it->tune[k], 90, (int32_t)k * 30 - 30);
}

static void sl_enter(void) { sl.t_move = 0; sl_jingle(sl.at); }

static void sl_input(const tn_input_t *in)
{
    int32_t turn = tn_enc_sum(in);
    if (turn) {
        sl.at = (uint8_t)((sl.at + (turn > 0 ? 1u : (uint32_t)SL_N - 1u)) % SL_N);
        sl.dir = turn > 0 ? 1 : -1;
        sl.t_move = in->now;
        snd_shake(40);
        sl_jingle(sl.at);
    }
    if (tn_go(in, 0)) {                         /* any key, any button (but HOME, EDIT, SAVE): in */
        const sl_item_t *it = &SL_ITEMS[sl.at];
        if (it->scene == 255u) { sl.t_nope = in->now; snd_fx(SFX_BOING); }
        else scene_go(it->scene);
    }
}

static void sl_draw(uint32_t now)
{
    int32_t slide = sl.t_move && now - sl.t_move < 180u ? sl.dir * (int32_t)(180u - (now - sl.t_move)) * 2 / 3 : 0, y, k;
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    for (k = 0; k < 30; k++) g_pset((k * 97 + 11) % 240, (k * 61 + 5) % 150, (now / 300u + (uint32_t)k) % 7u ? P_GREY : P_WHITE);
    g_rect(0, 196, G_W, 44, P_SNOW);
    for (k = -2; k <= 2; k++) {                 /* the row: the chosen one big, its neighbours small */
        uint32_t i = (sl.at + SL_N + (uint32_t)(k + 2 * (int32_t)SL_N)) % SL_N;
        int32_t cx = 120 + k * 92 + slide, s = k ? 22 : 50;
        int32_t wob = (!k && sl.t_nope && now - sl.t_nope < 500u) ? (int32_t)((now / 50u) % 5u) - 2 : 0;
        if (cx < -40 || cx > 280) continue;
        if (!k) g_disc(cx, 104, 66, P_NIGHT2);
        pic_draw(SL_ITEMS[i].pic, cx + wob, k ? 110 : 98, s, now);
    }
    if (SL_ITEMS[sl.at].scene != 255u) ui_play_sign(120, 179, now, 0);   /* press PLAY (or anything) to go in */
    g_text_c(120, 205, &FONT_S, SL_ITEMS[sl.at].name, P_NIGHT, 0xFFu);
    for (k = 0; k < (int32_t)SL_N; k++) g_disc(120 - (int32_t)SL_N * 5 + k * 10 + 5, 228, k == sl.at ? 3 : 2, k == sl.at ? P_RED : P_SNOW_SH);
}

static void sl_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < TN_NKEYS; k++) {            /* the lights run in towards the middle: press anything */
        uint32_t d = k < 13u ? 13u - k : k - 13u;
        lv[TN_KEY_LED(k)] = (uint8_t)((d + now / 90u) % 7u == 0u ? 40u : 4u);
    }
    for (k = 0; k < 14u; k++) if (TN_GO_BUTTONS >> k & 1u) lv[k] = 6;
    lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64);
    lv[TB_HOME] = 6;
}
#endif
