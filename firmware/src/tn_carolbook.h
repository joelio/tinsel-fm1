/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Carol Book (DESIGN section 7.2).
 *
 *   choosing   any knob turns the pages (a picture a carol); OCT- OCT+ choose the way: Magic (a wand: any key plays
 *              the next note), Follow (a lit key: press it), Listen (an ear: it plays itself); any key or PLAY starts
 *   playing    Magic and Follow: the keys; Listen: sit back. A knob changes the melody's sound. PLAY: back to the page.
 *   the end    snow fireworks, then back to the page */
#ifndef TN_CAROLBOOK_H
#define TN_CAROLBOOK_H

static const uint8_t CB_SOUNDS[] = {TI_CELESTA, TI_MUSICBOX, TI_HANDBELL, TI_GLOCK, TI_CHIME};
static struct {
    uint8_t page, mode, playing, sound, ended;
    carol_play_t p;
    uint32_t t_end, t_note, t_turn, last_key_t;
    int32_t turn_dir;
    uint8_t last_mel, last_key;
} cb;

static void cb_enter(void) { cb.playing = 0; cb.ended = 0; cb.t_turn = 0; }

static void cb_strike(const carol_ev_t *ev, uint32_t now, uint32_t vel)
{
    const tn_carol_t *c = &TN_CAROLS[cb.p.carol];
    if (ev->mel) {
        snd_ping(CB_SOUNDS[cb.sound % sizeof CB_SOUNDS], ev->mel + (CB_SOUNDS[cb.sound % sizeof CB_SOUNDS] == TI_HANDBELL ? 0u : 12u), vel, 0);
        cb.last_mel = ev->mel;
        cb.t_note = now;
    }
    if (ev->chord) {                            /* the backing: a soft bass and a pad */
        uint8_t ch[4];
        uint32_t n = carol_chord_notes(ev->root, ev->kind, 55u, ch), i;
        snd_ping(TI_SOFTBASS, 36u + (ev->root + 12u - 0u) % 12u + (ev->root < 4u ? 12u : 0u), 90, 0);
        for (i = 0; i < n; i++) snd_ping(TI_PAD, ch[i], 70, (int32_t)i * 30 - 45);
    }
    if (ev->beat && c->jolly) snd_shake(ev->beat == 2u ? 110u : 60u);
}

static void cb_input(const tn_input_t *in)
{
    uint32_t now = in->now;
    int32_t turn = tn_enc_sum(in);
    if (!cb.playing) {
        if (turn) {
            cb.page = (uint8_t)((cb.page + TN_NCAROLS + (uint32_t)(turn > 0 ? 1 : (int32_t)TN_NCAROLS - 1)) % TN_NCAROLS);
            cb.turn_dir = turn > 0 ? 1 : -1;
            cb.t_turn = now;
            snd_ping(TI_CHIME, 84u + cb.page * 2u, 60, cb.turn_dir * 40);
        }
        if (in->btn_down >> TB_OCT_UP & 1u) { cb.mode = (uint8_t)((cb.mode + 1u) % CM_COUNT); snd_ping(TI_GLOCK, 79u + cb.mode * 4u, 80, 0); }
        if (in->btn_down >> TB_OCT_DOWN & 1u) { cb.mode = (uint8_t)((cb.mode + CM_COUNT - 1u) % CM_COUNT); snd_ping(TI_GLOCK, 79u + cb.mode * 4u, 80, 0); }
        if (tn_go(in, 1u << TB_OCT_UP | 1u << TB_OCT_DOWN)) {   /* (OCT- OCT+ choose the way) */
            carol_start(&cb.p, cb.page, cb.mode, now);
            cb.playing = 1;
            cb.ended = 0;
            snd_fx(SFX_DOOR);
        }
        return;
    }
    if (cb.ended) {
        if (now - cb.t_end > 3500u || (in->btn_down >> TB_PLAY & 1u)) cb.playing = 0;
        return;
    }
    if (turn) { cb.sound = (uint8_t)((cb.sound + (uint32_t)(turn > 0 ? 1 : (int32_t)sizeof CB_SOUNDS - 1)) % sizeof CB_SOUNDS); }
    if (in->btn_down >> TB_PLAY & 1u) { snd_hush(); cb.playing = 0; return; }
    if (in->key_down) {
        uint32_t k;
        for (k = 0; k < TN_NKEYS; k++)
            if (in->key_down >> k & 1u) {
                carol_ev_t ev;
                cb.last_key = (uint8_t)k;
                cb.last_key_t = now;
                if (carol_key(&cb.p, k, &ev)) cb_strike(&ev, now, 105);
                else if (cb.p.mode == CM_FOLLOW) snd_ping(CB_SOUNDS[cb.sound % sizeof CB_SOUNDS], 53u + k + 12u, 30, 0);   /* quiet, never a buzzer */
            }
    }
    if (cb.p.mode == CM_LISTEN) {
        carol_ev_t ev;
        if (carol_tick(&cb.p, now, &ev)) cb_strike(&ev, now, 95);
    }
    if (cb.p.done && !cb.ended) {
        cb.ended = 1;
        cb.t_end = now;
        snd_fx(SFX_CHEER);
        snd_ping_at(400, TI_HANDBELL, 79, 100, -40);
        snd_ping_at(550, TI_HANDBELL, 84, 100, 40);
    }
}

static void cb_draw_keys(int32_t y, uint32_t now)   /* the 27 keys along the bottom: the next lit, the one after glowing */
{
    int next = cb.playing && !cb.ended ? carol_next_key(&cb.p, 0) : -1, after = cb.playing && !cb.ended ? carol_next_key(&cb.p, 1) : -1;
    uint32_t k;
    int32_t x = 4;
    for (k = 0; k < TN_NKEYS; k++) {             /* the white keys first */
        if (tn_black(k)) continue;
        g_rect(x, y, 13, 34, (int)k == next ? P_GOLD : (int)k == after ? P_YELLOW : P_SNOW);
        if (cb.last_key == k && now - cb.last_key_t < 150u) g_rect(x, y + 28, 13, 6, P_GREY);
        x += 15;
    }
    x = 4;
    for (k = 0; k < TN_NKEYS; k++) {
        if (tn_black(k)) {
            g_rect(x - 5, y, 9, 20, (int)k == next ? P_GOLD : (int)k == after ? P_GOLD_D : P_BLACK);
            continue;
        }
        x += 15;
    }
}

static void cb_draw(uint32_t now)
{
    const tn_carol_t *c = &TN_CAROLS[cb.playing ? cb.p.carol : cb.page];
    uint32_t i;
    g_clear(P_NIGHT);
    for (i = 0; i < 30u; i++) g_pset((int32_t)((i * 83u + 9u) % 240u), (int32_t)((i * 41u + now / (40u + i)) % 180u), P_GREY);
    if (!cb.playing) {
        int32_t slide = cb.t_turn && now - cb.t_turn < 200u ? cb.turn_dir * (int32_t)(200u - (now - cb.t_turn)) / 2 : 0;
        uint32_t m;
        g_disc(120 + slide, 100, 62, P_NIGHT2);
        pic_draw(c->picture, 120 + slide, 100, 44, now);
        g_text_c(120, 166, &FONT_S, c->title, P_SNOW, P_TEXT_AA);
        for (i = 0; i < TN_NCAROLS; i++) g_disc(120 - (int32_t)TN_NCAROLS * 6 + (int32_t)i * 12 + 6, 188, i == cb.page ? 4 : 2, i == cb.page ? P_GOLD : P_GREY);
        for (m = 0; m < CM_COUNT; m++) {        /* the three ways, the chosen one big */
            int32_t x = 50 + (int32_t)m * 70, sel = m == cb.mode;
            if (sel) g_disc(x, 24, 20, P_NIGHT3);
            pic_draw(m == CM_MAGIC ? PIC_WAND : m == CM_FOLLOW ? PIC_FOLLOW : PIC_EAR, x, 24, sel ? 15 : 9, now);
        }
        ui_play_sign(208, 100, now, 1);
        cb_draw_keys(204, now);
        return;
    }
    {                                            /* playing: the picture breathes with each note */
        uint32_t t = now - cb.t_note;
        int32_t s = 40 + (t < 200u ? (int32_t)(200u - t) / 20 : 0);
        pic_draw(c->picture, 120, 88, s, now);
        g_text_c(120, 150, &FONT_S, c->title, P_SNOW_SH, P_TEXT_AA);
        {                                        /* how far through */
            uint32_t w = c->n ? (uint32_t)cb.p.i * 200u / c->n : 0;
            g_rect(20, 176, 200, 4, P_NIGHT3);
            g_rect(20, 176, (int32_t)w, 4, P_GOLD);
        }
    }
    if (cb.ended) {                              /* snow fireworks */
        uint32_t t = now - cb.t_end, j;
        for (j = 0; j < 48u; j++) {
            int32_t a = (int32_t)(j * 7u % 48u), r = (int32_t)(t / 12u);
            int32_t dx = (int32_t)((j * 37u) % 41u) - 20, dy = (int32_t)((j * 53u) % 41u) - 20;
            g_rect(120 + dx * r / 20, 80 + dy * r / 20 + r * r / 900, 2, 2, (uint32_t)(a % 3 ? P_SNOW : P_GOLD));
        }
        g_text_c(120, 120, &FONT_L, "HOORAY!", P_GOLD, P_GOLD_AA);
    }
    cb_draw_keys(204, now);
}

static void cb_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    if (!cb.playing) {
        for (k = 0; k < TN_NKEYS; k++) lv[TN_KEY_LED(k)] = 3;
        lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64);
        lv[TB_OCT_DOWN] = lv[TB_OCT_UP] = 16;
        return;
    }
    if (cb.ended) { for (k = 0; k < TN_NKEYS; k++) lv[TN_KEY_LED(k)] = (now / 80u + k) % 3u ? 4u : 64u; return; }
    if (cb.p.mode == CM_MAGIC) {                 /* every key works: all softly lit, the last one pressed bright */
        for (k = 0; k < TN_NKEYS; k++) lv[TN_KEY_LED(k)] = 12;
        if (now - cb.last_key_t < 200u) lv[TN_KEY_LED(cb.last_key)] = 64;
    } else {
        int n = carol_next_key(&cb.p, 0), a = carol_next_key(&cb.p, 1);
        if (a >= 0) lv[TN_KEY_LED(a)] = 5;      /* the next one: glowing */
        if (n >= 0) lv[TN_KEY_LED(n)] = 64;     /* press me */
    }
    lv[TB_PLAY] = 12;
}
#endif
