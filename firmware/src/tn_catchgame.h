/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Present Catch, on the screen and in the bells (the rules: tn_catch.h). Santa's sleigh crosses the top, presents fall
 * towards seven chimneys, the keys under the lowest present light up. Every catch plays the next note of Jingle
 * Bells, so a good run plays the song. Any other key rings softly: never a buzzer. */
#ifndef TN_CATCHGAME_H
#define TN_CATCHGAME_H

#define CG_TOP 44                               /* where presents start */
static struct {
    catch_t g;
    carol_play_t tune;
    uint8_t playing, over, level;
    uint32_t t_start, t_over, t_catch, t_land, last_lane_flash;
    int32_t catch_x;
    uint16_t best_before;
} cg;

static int32_t cg_lane_x(uint32_t lane) { return 17 + (int32_t)lane * 34; }

static void cg_note(uint32_t vel)               /* the next note of the tune */
{
    carol_ev_t ev;
    if (cg.tune.done) carol_start(&cg.tune, 0, CM_MAGIC, 0);
    if (carol_key(&cg.tune, 0, &ev)) {
        snd_ping(TI_GLOCK, ev.mel + 12u, vel, 0);
        if (ev.chord) {
            uint8_t ch[4];
            uint32_t n = carol_chord_notes(ev.root, ev.kind, 55u, ch), i;
            snd_ping(TI_SOFTBASS, 40u + ev.root % 12u, 80, 0);
            for (i = 0; i < n; i++) snd_ping(TI_PAD, ch[i], 55, (int32_t)i * 30 - 45);
        }
        if (ev.beat == 2u) snd_shake(70);
    }
}

static void cg_enter(void)
{
    cg.playing = 0;
    cg.over = 0;
    cg.level = set.level[0] > LV_SANTA ? LV_LITTLE : set.level[0];
}

static void cg_begin(uint32_t now)
{
    catch_start(&cg.g, cg.level, now * 2654435761u + 1u);
    carol_start(&cg.tune, 0, CM_MAGIC, 0);
    cg.playing = 1;
    cg.over = 0;
    cg.t_start = now;
    cg.best_before = set.best[0];
    snd_fx(SFX_DOOR);
}

static void cg_input(const tn_input_t *in)
{
    uint32_t now = in->now, k;
    if (!cg.playing) {
        if (in->key_down || (in->btn_down >> TB_PLAY & 1u)) cg_begin(now);
        return;
    }
    if (cg.over) {
        if (now - cg.t_over > 4000u || ((in->key_down || (in->btn_down >> TB_PLAY & 1u)) && now - cg.t_over > 1200u)) cg.playing = 0;
        return;
    }
    for (k = 0; k < TN_NKEYS; k++)
        if (in->key_down >> k & 1u) {
            if (catch_key(&cg.g, k)) {
                cg.t_catch = now;
                cg.catch_x = cg_lane_x(catch_lane_of(k));
                cg_note(110);
            } else
                snd_ping(TI_CHIME, gr_note(k) + 12u, 35, 0);   /* soft, in tune */
        }
    if (in->btn_down >> TB_PLAY & 1u) cg.playing = 0;
}

static void cg_tick(uint32_t now, uint32_t dt)
{
    uint32_t landed;
    if (!cg.playing || cg.over) return;
    landed = catch_step(&cg.g, dt);
    if (landed) {
        cg.t_land = now;
        if (cg.g.level == LV_LITTLE) { snd_fx(SFX_BOING); cg_note(80); }   /* the snowman caught it */
        else snd_ping(TI_TOY, 48, 70, 0);
    }
    if (cg.g.done) {
        cg.over = 1;
        cg.t_over = now;
        if (cg.g.score > set.best[0]) { set.best[0] = cg.g.score; set_touch(); snd_fx(SFX_FANFARE); }
        else snd_fx(SFX_CHEER);
    }
}

static void cg_draw_present(int32_t cx, int32_t y, uint32_t col)
{
    static const uint8_t C[5] = {P_RED, P_BLUE, P_GREEN, P_PURPLE, P_ORANGE};
    g_rect(cx - 9, y - 8, 19, 16, C[col % 5u]);
    g_rect(cx - 2, y - 8, 4, 16, P_GOLD);
    g_rect(cx - 9, y - 2, 19, 4, P_GOLD);
    g_disc(cx - 3, y - 10, 3, P_GOLD);
    g_disc(cx + 3, y - 10, 3, P_GOLD);
}

static void cg_draw(uint32_t now)
{
    uint32_t i;
    int32_t y, lit = cg.playing ? catch_lit_lane(&cg.g) : -1;
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    for (i = 0; i < 26u; i++) g_pset((int32_t)((i * 97u + 11u) % 240u), (int32_t)((i * 61u + 5u) % 120u), P_GREY);
    {                                            /* the sleigh crosses the top */
        int32_t sx = (int32_t)((now / 12u) % 320u) - 40;
        g_rect(sx, 22, 30, 8, P_RED);
        g_rect(sx - 2, 30, 36, 2, P_GOLD);
        g_disc(sx + 6, 16, 5, P_RED);            /* Santa */
        g_disc(sx + 6, 10, 3, P_SKIN);
        g_sprite(&DEER, sx + 34, 12, 1, 0);
        g_sprite(&DEER, sx + 52, 12, 1, 0);
    }
    for (i = 0; i < CATCH_LANES; i++) {          /* the rooftops and chimneys */
        int32_t x = cg_lane_x(i), on = (int32_t)i == lit;
        g_rect(x - 17, 214, 34, 26, i & 1u ? P_ROOF : P_RED_D);
        g_rect(x - 17, 212, 34, 3, P_SNOW);
        g_rect(x - 8, 196, 16, 18, on ? P_GOLD : P_BROWN);
        g_rect(x - 10, 194, 20, 4, P_SNOW);
        if (on) g_rect(x - 6, 190, 12, 3, (now / 100u) & 1u ? P_YELLOW : P_GOLD);
    }
    if (cg.playing) {
        for (i = 0; i < 4u; i++)
            if (cg.g.p[i].on) cg_draw_present(cg_lane_x(cg.g.p[i].lane), CG_TOP + (cg.g.p[i].y >> 8) * (186 - CG_TOP) / (CATCH_FLOOR >> 8), cg.g.p[i].col);
        if (now - cg.t_catch < 400u) {          /* a sparkle out of the chimney */
            uint32_t j, t = now - cg.t_catch;
            for (j = 0; j < 10u; j++) g_rect(cg.catch_x + (int32_t)(j * 9u % 19u) - 9, 188 - (int32_t)(t / 8u) - (int32_t)(j * 5u % 11u), 2, 2, j & 1u ? P_YELLOW : P_WHITE);
        }
        {                                        /* the score: a star a catch (to ten), then the number */
            char b[12];
            uint32_t n = cg.g.score, s = n % 10u;
            for (i = 0; i < s; i++) pic_star(10 + (int32_t)i * 16, 40, 6, P_GOLD);
            tn_utoa(b, n);
            g_text(200, 32, &FONT_S, b, P_SNOW, P_TEXT_AA);
            if (cg.g.level != LV_LITTLE)
                for (i = 0; i < (cg.g.level == LV_ELF ? 5u : 3u); i++) g_disc(206 + (int32_t)i * 7 - (cg.g.level == LV_ELF ? 14 : 0), 54, 2, i < cg.g.misses ? P_GREY : P_RED);
        }
        if (cg.over) {
            char b[24];
            g_rect(30, 80, 180, 90, P_NIGHT);
            g_rect(30, 80, 180, 3, P_GOLD);
            g_rect(30, 167, 180, 3, P_GOLD);
            pic_draw(PIC_PRESENT, 120, 110, 18, now);
            tn_utoa(b, cg.g.score);
            g_text_c(120, 132, &FONT_L, b, P_GOLD, P_GOLD_AA);
            if (cg.g.score > cg.best_before) g_text_c(120, 92, &FONT_S, "BEST EVER!", P_YELLOW, 0xFFu);
        }
    } else {                                     /* the start: a present, the level's stars */
        pic_draw(PIC_PRESENT, 120, 110, 36, now);
        for (i = 0; i <= cg.level; i++) pic_star(120 - (int32_t)cg.level * 14 + (int32_t)i * 28, 158, 10, P_GOLD);
        if (set.best[0]) {
            char b[24];
            memcpy(b, "BEST ", 5);
            tn_utoa(b + 5, set.best[0]);
            g_text_c(120, 172, &FONT_S, b, P_SNOW_SH, P_TEXT_AA);
        }
    }
}

static void cg_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    int lit = cg.playing && !cg.over ? catch_lit_lane(&cg.g) : -1;
    for (k = 0; k < TN_NKEYS; k++) lv[TN_KEY_LED(k)] = lit >= 0 && catch_lane_of(k) == (uint32_t)lit ? 64u : 2u;
    if (!cg.playing) { lv[TB_PLAY] = (uint8_t)(16u + now / 30u % 32u); for (k = 0; k < TN_NKEYS; k++) lv[TN_KEY_LED(k)] = (now / 150u + k) % 7u == 0u ? 32u : 3u; }
}
#endif
