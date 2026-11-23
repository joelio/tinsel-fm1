/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Music Box (DESIGN section 7.3; the cylinder: tn_box.h).
 *
 *   any knob    turns the crank: the faster you turn, the faster it plays; backwards plays backwards
 *   PLAY        winds it up: it plays by itself and runs down slowly, like clockwork (PLAY again: stop)
 *   OCT- OCT+   another box: four carols, the scribble box, the kept boxes
 *   keys        on the scribble box, a pin where the comb is (and its note); on the others, just the note
 *   REC         empties the scribble box (only that one: it is not kept until a grown-up keeps it in the panel) */
#ifndef TN_MUSICBOX_H
#define TN_MUSICBOX_H

#define MB_CAROLS 4u
#define MB_KEPT 4u
static const uint8_t MB_CAROL_OF[MB_CAROLS] = {0, 1, 3, 2};   /* Jingle Bells, We Wish You, Silent Night, Deck the Halls */
static struct {
    box_t carol[MB_CAROLS], kept[MB_KEPT], scribble;
    uint8_t kept_ok[MB_KEPT], loaded, at;
    int32_t pos;                                /* where the comb is, Q8 steps */
    int32_t wind;                               /* clockwork: Q8 steps a second (0: stopped) */
    uint32_t t_tooth[27], t_last_crank;
    int32_t crank_angle;
} mb;

static uint32_t mb_count(void) { uint32_t n = MB_CAROLS + 1u, i; for (i = 0; i < MB_KEPT; i++) n += mb.kept_ok[i]; return n; }
static box_t *mb_box(uint32_t at, int *editable)   /* boxes in order: carols, scribble, kept */
{
    uint32_t i;
    *editable = 0;
    if (at < MB_CAROLS) return &mb.carol[at];
    if (at == MB_CAROLS) { *editable = 1; return &mb.scribble; }
    at -= MB_CAROLS + 1u;
    for (i = 0; i < MB_KEPT; i++) if (mb.kept_ok[i] && at-- == 0u) return &mb.kept[i];
    return &mb.scribble;
}
static void mb_load_kept(void)
{
    uint32_t i;
    for (i = 0; i < MB_KEPT; i++) mb.kept_ok[i] = (uint8_t)(slot_load(i, &mb.kept[i], sizeof mb.kept[i]) == 0 && box_valid(&mb.kept[i]));
}
static void mb_enter(void)
{
    uint32_t i;
    if (!mb.loaded) {
        for (i = 0; i < MB_CAROLS; i++) box_from_carol(&mb.carol[i], &TN_CAROLS[MB_CAROL_OF[i] % TN_NCAROLS]);
        box_clear(&mb.scribble);
        mb_load_kept();
        mb.loaded = 1;
    }
    mb.wind = 0;
    if (mb.at >= mb_count()) mb.at = 0;
}
static int mb_keep_scribble(void)              /* the panel: the scribble into the first free kept box (1..4), 0 if full or empty */
{
    uint32_t i;
    if (!mb.scribble.n) return 0;
    for (i = 0; i < MB_KEPT; i++)
        if (!mb.kept_ok[i]) {
            mb.kept[i] = mb.scribble;
            if (slot_save(i, &mb.kept[i], sizeof mb.kept[i])) return 0;
            mb.kept_ok[i] = 1;
            return (int)i + 1;
        }
    return 0;
}
static void mb_clear_kept(void)
{
    uint32_t i;
    box_t empty;
    memset(&empty, 0, sizeof empty);             /* (an invalid box: reads as nothing kept) */
    for (i = 0; i < MB_KEPT; i++) if (mb.kept_ok[i]) { slot_save(i, &empty, sizeof empty); mb.kept_ok[i] = 0; }
    mb.at = 0;
}

static void mb_play_step(const box_t *b, uint32_t step, uint32_t now)
{
    uint32_t k, p = b->pin[step % BOX_STEPS];
    for (k = 0; k < 27u; k++)
        if (p >> k & 1u) { snd_ping(TI_MUSICBOX, 65u + k, 95, ((int32_t)k - 13) * 4); mb.t_tooth[k] = now; }
}
static void mb_turn(const box_t *b, int32_t dq8, uint32_t now)   /* the cylinder moves; every step it crosses plays */
{
    int32_t from = mb.pos >> 8, to, s;
    mb.pos += dq8;
    while (mb.pos < 0) mb.pos += (int32_t)BOX_STEPS << 8, from += (int32_t)BOX_STEPS;
    while (mb.pos >= (int32_t)BOX_STEPS << 8) mb.pos -= (int32_t)BOX_STEPS << 8, from -= (int32_t)BOX_STEPS;
    to = mb.pos >> 8;
    if (to > from) for (s = from + 1; s <= to; s++) mb_play_step(b, (uint32_t)(s + (int32_t)BOX_STEPS * 4) % BOX_STEPS, now);
    else if (to < from) for (s = from; s > to; s--) mb_play_step(b, (uint32_t)(s - 1 + (int32_t)BOX_STEPS * 4) % BOX_STEPS, now);
    mb.crank_angle += dq8;
}

static void mb_input(const tn_input_t *in)
{
    int ed;
    box_t *b = mb_box(mb.at, &ed);
    uint32_t now = in->now, k;
    int32_t turn = tn_enc_sum(in);
    if (turn) { mb.wind = 0; mb_turn(b, turn * 256, now); mb.t_last_crank = now; }
    if (in->btn_down >> TB_PLAY & 1u) { mb.wind = mb.wind ? 0 : 4 << 8; if (mb.wind) { snd_ping(TI_TOY, 96, 40, -40); snd_ping_at(60, TI_TOY, 100, 40, -40); } }
    if (in->btn_down >> TB_OCT_UP & 1u) { mb.at = (uint8_t)((mb.at + 1u) % mb_count()); mb.pos = 0; snd_ping(TI_MUSICBOX, 77 + mb.at, 70, 0); }
    if (in->btn_down >> TB_OCT_DOWN & 1u) { mb.at = (uint8_t)((mb.at + mb_count() - 1u) % mb_count()); mb.pos = 0; snd_ping(TI_MUSICBOX, 77 + mb.at, 70, 0); }
    if ((in->btn_down >> TB_REC & 1u) && ed && mb.scribble.n) { box_clear(&mb.scribble); snd_fx(SFX_SPARKLE); }
    for (k = 0; k < 27u; k++)
        if (in->key_down >> k & 1u) {
            if (ed) box_pin(&mb.scribble, (uint32_t)((mb.pos + 128) >> 8) % BOX_STEPS, k);
            snd_ping(TI_MUSICBOX, 65u + k, 100, ((int32_t)k - 13) * 4);
            mb.t_tooth[k] = now;
        }
}

static void mb_tick(uint32_t now, uint32_t dt)
{
    int ed;
    if (!mb.wind) return;
    mb_turn(mb_box(mb.at, &ed), mb.wind * (int32_t)dt / 1000, now);
    mb.wind -= mb.wind / 400 + 1;                /* running down, slowly (about half a minute) */
    if (mb.wind < 40) mb.wind = 0;
}

static void mb_draw(uint32_t now)
{
    int ed;
    const box_t *b = mb_box(mb.at, &ed);
    int32_t y, s, k;
    g_clear(P_NIGHT);
    g_rect(8, 30, 224, 150, P_BROWN_D);          /* the box */
    g_rect(12, 34, 216, 142, P_BROWN);
    g_rect(28, 38, 168, 134, P_GOLD_D);          /* the cylinder, from above: steps run right to the comb */
    for (y = 0; y < 27; y++) g_hline(28, 195, 40 + y * 5 + 2, (y & 1) ? P_GOLD_D : P_GOLD_D);
    for (s = -33; s <= 0; s++) {                 /* 34 steps on view, the comb at the right */
        int32_t step = ((mb.pos >> 8) + s + (int32_t)BOX_STEPS * 4) % (int32_t)BOX_STEPS;
        int32_t x = 192 + s * 5 - ((mb.pos & 255) * 5 >> 8);
        uint32_t p = b->pin[step];
        if (x < 30) continue;
        if (step % 8 == 0) g_rect(x, 38, 1, 134, P_BROWN);
        for (k = 0; k < 27; k++) if (p >> k & 1u) g_rect(x - 1, 40 + (26 - k) * 5, 3, 3, P_SNOW);
    }
    for (k = 0; k < 27; k++) {                   /* the comb: a tooth a key, lit when struck */
        int lit = now - mb.t_tooth[k] < 180u;
        g_rect(196, 40 + (26 - k) * 5, lit ? 30 : 26 - k / 3, 3, lit ? P_YELLOW : P_SNOW_SH);
    }
    {                                            /* the crank, at the left, turning with the cylinder */
        int32_t a = (mb.crank_angle >> 4) & 63, cx = 18, cy = 105, dx, dy;
        static const int8_t CS[16] = {0, 4, 7, 9, 10, 9, 7, 4, 0, -4, -7, -9, -10, -9, -7, -4};
        dx = CS[(a / 4) & 15]; dy = CS[((a / 4) + 4) & 15];
        g_line(cx, cy, cx + dx, cy + dy, P_GREY);
        g_disc(cx + dx, cy + dy, 3, P_RED);
        g_disc(cx, cy, 3, P_GREY);
    }
    {                                            /* which box */
        uint32_t i, n = mb_count();
        if (mb.at < MB_CAROLS) pic_draw(TN_CAROLS[MB_CAROL_OF[mb.at] % TN_NCAROLS].picture, 40, 206, 16, now);
        else pic_draw(ed ? PIC_WAND : PIC_PRESENT, 40, 206, 14, now);
        g_text(68, 194, &FONT_S, mb.at < MB_CAROLS ? TN_CAROLS[MB_CAROL_OF[mb.at] % TN_NCAROLS].title : ed ? "YOUR TUNE" : "KEPT TUNE", P_SNOW, P_TEXT_AA);
        for (i = 0; i < n; i++) g_disc(74 + (int32_t)i * 10, 222, i == mb.at ? 3 : 2, i == mb.at ? P_GOLD : P_GREY);
        if (!mb.wind && now - mb.t_last_crank > 3000u) ui_play_sign(214, 206, now, 0);
    }
}

static void mb_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = now - mb.t_tooth[k] < 200u ? 64u : 3u;
    lv[TB_PLAY] = mb.wind ? 24u : (uint8_t)tn_pulse(now, 8, 48);
    lv[TB_OCT_DOWN] = lv[TB_OCT_UP] = 8;
}
#endif
