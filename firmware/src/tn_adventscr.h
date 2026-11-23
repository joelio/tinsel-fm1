/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Advent calendar on the screen (the rules: tn_advent.h). A house with 24 windows. The first 24 keys are the
 * doors (key 1 is door 1): today's door wobbles and its key pulses; open doors glow. Knobs move a cursor too; any key
 * or PLAY opens the chosen door. A door that may not open yet just boings. */
#ifndef TN_ADVENTSCR_H
#define TN_ADVENTSCR_H

static struct {
    uint8_t sel, showing, gift;
    uint32_t t_show, t_nope;
    uint8_t nope_door;
    carol_play_t tune;
} av;

static int32_t av_x(uint32_t d) { return 14 + (int32_t)((d - 1u) % 6u) * 36; }
static int32_t av_y(uint32_t d) { return 58 + (int32_t)((d - 1u) / 6u) * 42; }

static void av_enter(void)
{
    int t = advent_today(&set);
    av.showing = 0;
    av.sel = (uint8_t)(t ? t : 1);
}

static void av_show(uint32_t door, uint32_t now)
{
    const advent_gift_t *g = &ADV_GIFT[door - 1u];
    av.showing = 1;
    av.gift = (uint8_t)door;
    av.t_show = now;
    snd_fx(SFX_DOOR);
    switch (g->kind) {
    case GIFT_CAROL: carol_start(&av.tune, g->arg, CM_LISTEN, now + 600u); break;
    case GIFT_JOKE: snd_ping_at(500, TI_TOY, 50, 110, 0); snd_shake_at(520, 127); snd_fx(SFX_SPARKLE); break;   /* the cracker */
    case GIFT_DECOR: snd_fx(SFX_SPARKLE); break;
    case GIFT_SOUND:
        if (g->arg == 0u) { snd_shake_at(400, 127); snd_shake_at(700, 90); snd_shake_at(1000, 127); }
        else if (g->arg == 1u) snd_fx(SFX_TWINKLE);
        else snd_fx(SFX_FANFARE);
        break;
    }
}

static void av_try(uint32_t door, uint32_t now)
{
    if (advent_open(&set, door)) { set_touch(); av_show(door, now); }
    else if (advent_is_open(&set, door)) av_show(door, now);
    else { av.t_nope = now; av.nope_door = (uint8_t)door; snd_fx(SFX_BOING); }
}

static void av_input(const tn_input_t *in)
{
    uint32_t now = in->now, k;
    if (av.showing) {
        const advent_gift_t *g = &ADV_GIFT[av.gift - 1u];
        if (g->kind == GIFT_CAROL) {
            carol_ev_t ev;
            if (carol_tick(&av.tune, now, &ev) && ev.mel) {
                snd_ping(TI_HANDBELL, ev.mel, 95, 0);
                if (ev.chord) { uint8_t ch[4]; uint32_t n = carol_chord_notes(ev.root, ev.kind, 55u, ch), i; for (i = 0; i < n; i++) snd_ping(TI_PAD, ch[i], 50, (int32_t)i * 30 - 45); }
            }
        }
        if (now - av.t_show > 800u && (in->key_down || (in->btn_down >> TB_PLAY & 1u))) { av.showing = 0; snd_hush(); return; }
        if (g->kind != GIFT_CAROL ? now - av.t_show > 9000u : av.tune.done && now - av.t_show > 2000u) av.showing = 0;
        return;
    }
    if (in->enc_x || in->enc_y) {
        int32_t d = (int32_t)av.sel + in->enc_x - in->enc_y * 6;
        while (d < 1) d += 24;
        while (d > 24) d -= 24;
        av.sel = (uint8_t)d;
        snd_ping(TI_CHIME, 79u + (uint32_t)d % 12u, 40, 0);
    }
    for (k = 0; k < ADV_DOORS; k++)
        if (in->key_down >> k & 1u) { av.sel = (uint8_t)(k + 1u); av_try(k + 1u, now); return; }
    if (in->btn_down >> TB_PLAY & 1u) av_try(av.sel, now);
}

static void av_draw(uint32_t now)
{
    uint32_t d;
    int32_t y;
    int today = advent_today(&set);
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    if (av.showing) {
        const advent_gift_t *g = &ADV_GIFT[av.gift - 1u];
        char b[4];
        uint32_t t = now - av.t_show;
        int32_t s = t < 400u ? (int32_t)t / 10 + 6 : 46;
        g_disc(120, 96, s + 18, P_NIGHT3);
        pic_draw(g->pic, 120, 96, s, now);
        tn_utoa(b, av.gift);
        g_text_c(120, 18, &FONT_L, b, P_GOLD, P_GOLD_AA);
        if (t > 400u) {
            g_text_c(120, 168, &FONT_S, g->line1, P_SNOW, P_TEXT_AA);
            if (g->line2) g_text_c(120, 188, &FONT_S, g->line2, P_SNOW, P_TEXT_AA);
        }
        if (t < 900u) {                          /* sparkles out of the door */
            uint32_t j;
            for (j = 0; j < 24u; j++) {
                int32_t r = (int32_t)t / 8, dx = (int32_t)(j * 37u % 41u) - 20, dy = (int32_t)(j * 53u % 41u) - 20;
                g_rect(120 + dx * r / 20, 96 + dy * r / 20, 2, 2, j & 1u ? P_GOLD : P_WHITE);
            }
        }
        return;
    }
    g_rect(4, 44, 232, 196, P_RED_D);            /* the house */
    for (y = 48; y < 240; y += 8) g_hline(4, 235, y, P_RED_D + 0u);
    g_tri_up(120, 4, -4, 244, 46, P_ROOF);
    g_rect(0, 42, G_W, 5, P_SNOW);
    g_rect(170, 10, 18, 26, P_RED_D);            /* the chimney */
    g_rect(167, 8, 24, 5, P_SNOW);
    for (d = 1; d <= ADV_DOORS; d++) {
        int32_t x = av_x(d), yy = av_y(d), wob = 0;
        int open = advent_is_open(&set, d), may = advent_may_open(&set, d);
        char b[4];
        if ((int)d == today) wob = (int32_t)((now / 90u) % 3u) - 1;
        if (av.nope_door == d && now - av.t_nope < 400u) wob = (int32_t)((now / 40u) % 5u) - 2;
        if (d == av.sel) g_rect(x - 3 + wob, yy - 3, 34, 38, (now / 250u) & 1u ? P_GOLD : P_YELLOW);
        g_rect(x + wob, yy, 28, 32, open ? P_WINDOW : may ? P_NIGHT3 : P_NIGHT);
        if (open) pic_draw(ADV_GIFT[d - 1u].pic, x + 14, yy + 17, 9, now);
        else {
            tn_utoa(b, d);
            g_text_c(x + 14 + wob, yy + 8, &FONT_S, b, may ? P_GOLD : P_GREY, may ? P_GOLD_AA : P_DIM);
        }
        g_rect(x + wob, yy + 32, 28, 3, P_SNOW);
    }
}

static void av_leds(uint8_t *lv, uint32_t now)
{
    uint32_t d;
    int today = advent_today(&set);
    if (av.showing) return;
    for (d = 1; d <= ADV_DOORS; d++)
        lv[TN_KEY_LED(d - 1u)] = (int)d == today ? (uint8_t)(16u + (now / 20u) % 48u) : advent_is_open(&set, d) ? 6u : 0u;
    lv[TB_PLAY] = (uint8_t)tn_pulse(now, 8, 40);
}
#endif
