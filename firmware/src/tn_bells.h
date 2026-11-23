/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Bell Tower (DESIGN section 7.4; the rules: tn_ring.h). Six bells in a tower, ropes below, the rows going by.
 *
 *   stopped     keys ring the bells (the keyboard in six groups, treble on the left); any knob picks the method;
 *               PLAY starts the ringing
 *   in rounds   keys are call changes: a key of place p swaps the bells in places p and p+1 at the next row
 *               (QUEENS, TITTUMS and BACK ROUNDS get a cheer)
 *   a method    OCT- OCT+ choose your bell (or none): when its turn comes its keys light, press one. Little Elf:
 *               the tower waits for you. Elf, Santa: in time, or the tower rings it for you
 *               ARP calls a bob, SEQ a single, at the next lead end
 *   PLAY        stand the bells (stop) */
#ifndef TN_BELLS_H
#define TN_BELLS_H

#define BT_STRIKE_MS 270u
static const uint8_t BT_NOTE[6] = {69, 67, 65, 64, 62, 60};   /* treble to tenor: a major scale down to C */
static struct {
    ring_t r;
    uint8_t method, ringing, place, mine, waiting, call_shown;
    uint8_t swap_next, named;                   /* a call change to make (place + 1); the named row reached */
    uint32_t t_next, t_named, t_called, t_strike[6];
    uint16_t good, late;
    uint8_t hist[6][6];                         /* the last rows, for the screen */
} bt;

static uint32_t bt_group_of(uint32_t key) { return key * 6u / 27u; }   /* the keys in six groups */

static void bt_strike(uint32_t bell, uint32_t vel, uint32_t now)
{
    if (bell < 1u || bell > 6u) return;
    snd_ping(TI_CHURCH, BT_NOTE[bell - 1u], vel, ((int32_t)bell - 3) * 20);
    bt.t_strike[bell - 1u] = now;
}

static void bt_enter(void) { bt.ringing = 0; ring_start(&bt.r, bt.method); }

static void bt_row_done(uint32_t now)           /* the end of a row: the next one */
{
    uint32_t i;
    for (i = 5; i > 0; i--) memcpy(bt.hist[i], bt.hist[i - 1u], 6);
    memcpy(bt.hist[0], bt.r.row, 6);
    if (bt.r.method == RM_ROUNDS) {
        if (bt.swap_next) { ring_swap(&bt.r, bt.swap_next - 1u); bt.swap_next = 0; }
        {
            uint32_t n = ring_named(&bt.r);
            if (n > 1u && n != bt.named) { bt.named = (uint8_t)n; bt.t_named = now; snd_fx(SFX_CHEER); }
            if (n != bt.named) bt.named = (uint8_t)n;
        }
        bt.r.rows++;
    } else {
        ring_next(&bt.r);
        if (bt.r.rounds_again && bt.r.rows > 2u) { bt.ringing = 0; bt.t_named = now; bt.named = 1; snd_fx(SFX_CHEER); }   /* come round: stand */
    }
    bt.place = 0;
    bt.t_next = now + BT_STRIKE_MS + ((bt.r.rows & 1u) ? 0u : BT_STRIKE_MS);   /* the handstroke gap */
}

static void bt_input(const tn_input_t *in)
{
    uint32_t now = in->now, k, level = set.level[5] % 3u;
    int32_t turn = tn_enc_sum(in);
    if (!bt.ringing) {
        if (turn) {
            bt.method = (uint8_t)((bt.method + RM_COUNT + (uint32_t)(turn > 0 ? 1 : (int32_t)RM_COUNT - 1)) % RM_COUNT);
            ring_start(&bt.r, bt.method);
            snd_ping(TI_CHURCH, BT_NOTE[bt.method % 6u], 60, 0);
        }
        for (k = 0; k < TN_NKEYS; k++) if (in->key_down >> k & 1u) bt_strike(bt_group_of(k) + 1u, 100, now);
        if (in->btn_down >> TB_PLAY & 1u) {
            ring_start(&bt.r, bt.method);
            bt.ringing = 1; bt.place = 0; bt.good = bt.late = 0; bt.named = 1; bt.waiting = 0;
            bt.t_next = now + 400u;
            memset(bt.hist, 0, sizeof bt.hist);
        }
        if (in->btn_down >> TB_OCT_UP & 1u) bt.mine = (uint8_t)((bt.mine + 1u) % 7u);
        if (in->btn_down >> TB_OCT_DOWN & 1u) bt.mine = (uint8_t)((bt.mine + 6u) % 7u);
        return;
    }
    if (in->btn_down >> TB_PLAY & 1u) { bt.ringing = 0; return; }
    if (in->btn_down >> TB_OCT_UP & 1u) bt.mine = (uint8_t)((bt.mine + 1u) % 7u);
    if (in->btn_down >> TB_OCT_DOWN & 1u) bt.mine = (uint8_t)((bt.mine + 6u) % 7u);
    if (in->btn_down >> TB_ARP & 1u) { ring_call(&bt.r, CALL_BOB); bt.call_shown = CALL_BOB; bt.t_called = now; snd_ping(TI_CHIME, 84, 60, 0); }
    if (in->btn_down >> TB_SEQ & 1u) { ring_call(&bt.r, CALL_SINGLE); bt.call_shown = CALL_SINGLE; bt.t_called = now; snd_ping(TI_CHIME, 88, 60, 0); }
    for (k = 0; k < TN_NKEYS; k++) {
        if (!(in->key_down >> k & 1u)) continue;
        if (bt.r.method == RM_ROUNDS) {          /* a call change: the bells at this place and the next swap */
            uint32_t p = bt_group_of(k);
            if (p > 4u) p = 4u;
            bt.swap_next = (uint8_t)(p + 1u);
            snd_ping(TI_CHIME, 79u + p * 2u, 40, 0);
        } else if (bt.mine && bt.place < bt.r.bells && bt.r.row[bt.place] == bt.mine && bt_group_of(k) + 1u == bt.mine) {
            int32_t off = (int32_t)(now - bt.t_next);   /* how far from its time */
            if (off < 0) off = -off;
            if (bt.waiting || off < (level == LV_SANTA ? 90 : 160)) bt.good++;
            else bt.late++;
            bt_strike(bt.mine, 115, now);
            bt.waiting = 0;
            bt.place++;
            bt.t_next = now + BT_STRIKE_MS;
            if (bt.place >= bt.r.bells) bt_row_done(now);
        }
    }
}

static void bt_tick(uint32_t now)
{
    uint32_t level = set.level[5] % 3u;
    if (!bt.ringing || (int32_t)(now - bt.t_next) < 0) return;
    if (bt.place < bt.r.bells && bt.mine && bt.r.method != RM_ROUNDS && bt.r.row[bt.place] == bt.mine) {
        if (level == LV_LITTLE) { bt.waiting = 1; return; }   /* the tower waits for you */
        if ((int32_t)(now - bt.t_next) < 220) return;         /* (your moment, a little longer) */
        bt.late++;
        bt_strike(bt.mine, 50, now);             /* rung for you, softly */
        bt.place++;
        bt.t_next = now + BT_STRIKE_MS;
        if (bt.place >= bt.r.bells) bt_row_done(now);
        return;
    }
    if (bt.place < bt.r.bells) {
        bt_strike(bt.r.row[bt.place], 100, now);
        bt.place++;
        bt.t_next += BT_STRIKE_MS;
        if ((int32_t)(now - bt.t_next) > 300) bt.t_next = now + BT_STRIKE_MS;
    }
    if (bt.place >= bt.r.bells) bt_row_done(now);
}

static void bt_draw(uint32_t now)
{
    uint32_t i, j;
    int32_t y;
    char b[24];
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, y < 120 ? P_NIGHT2 : P_NIGHT);
    for (i = 0; i < 4u; i++) {                   /* the tower's arches */
        int32_t x = 4 + (int32_t)i * 60;
        g_rect(x, 26, 52, 70, P_NIGHT);
        g_disc(x + 26, 32, 26, P_NIGHT);
        g_rect(x - 4, 26, 4, 80, P_GREY);
    }
    g_rect(0, 96, G_W, 6, P_BROWN_D);            /* the beam */
    g_text_c(120, 4, &FONT_S, RING_METHODS[bt.method].name, P_GOLD, P_GOLD_AA);
    for (i = 0; i < 6u; i++) {
        int32_t x = 24 + (int32_t)i * 38, t = (int32_t)(now - bt.t_strike[i]), swing = 0, rope;
        uint32_t c = (i + 1u) == bt.mine ? P_YELLOW : P_GOLD;
        if (i >= bt.r.bells) { pic_bell(x, 62, 14, P_GREY, P_DIM); continue; }
        if (t >= 0 && t < 500) swing = (t < 250 ? t : 500 - t) / 25;
        pic_bell(x + ((i & 1u) ? swing : -swing), 62 - swing / 2, 15, c, P_GOLD_D);
        rope = 108 + (t >= 0 && t < 400 ? (t < 200 ? t : 400 - t) / 12 : 0);
        g_rect(x - 1, 102, 2, rope - 102 + 10, P_SNOW_SH);
        g_rect(x - 3, rope, 6, 18, (i + 1u) == bt.mine ? P_BLUE : P_RED);   /* the sally */
        g_rect(x - 3, rope + 6, 6, 3, P_SNOW);
        tn_utoa(b, i + 1u);
        g_text_c(x, 140, &FONT_S, b, (i + 1u) == bt.mine ? P_BLUE : P_GREY, 0xFFu);
    }
    for (j = 0; j < 4u; j++) {                   /* the rows going by: the newest bright, your bell blue */
        int32_t yy = 222 - (int32_t)j * 18;
        const uint8_t *row = j == 0u ? bt.r.row : bt.hist[j - 1u];
        if (!row[0]) continue;
        for (i = 0; i < bt.r.bells; i++) {
            b[0] = (char)('0' + row[i]); b[1] = 0;
            g_text(60 + (int32_t)i * 22, yy, &FONT_S, b, row[i] == bt.mine ? P_BLUE : j == 0u && bt.ringing && i < bt.place ? P_GOLD : j ? P_DIM : P_SNOW, 0xFFu);
        }
    }
    if (bt.named > 1u && now - bt.t_named < 2500u) g_text_c(120, 160, &FONT_S, RING_NAMES[bt.named], P_YELLOW, 0xFFu);
    else if (bt.call_shown && now - bt.t_called < 1500u) g_text_c(120, 160, &FONT_S, bt.call_shown == CALL_BOB ? "BOB!" : "SINGLE!", P_YELLOW, 0xFFu);
    else if (bt.waiting) g_text_c(120, 160, &FONT_S, "YOUR BELL!", P_BLUE, 0xFFu);
    if (!bt.ringing) ui_play_sign(214, 172, now, 0);
}

static void bt_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < TN_NKEYS; k++) {
        uint32_t g = bt_group_of(k), l = (g & 1u) ? 3u : 6u;
        if (now - bt.t_strike[g] < 250u) l = 64u;
        if (bt.ringing && bt.mine && g + 1u == bt.mine && bt.place < bt.r.bells && bt.r.row[bt.place] == bt.mine) l = 64u;
        lv[TN_KEY_LED(k)] = (uint8_t)l;
    }
    lv[TB_PLAY] = bt.ringing ? 24u : (uint8_t)tn_pulse(now, 12, 64);
    if (bt.ringing && bt.r.method != RM_ROUNDS && RING_METHODS[bt.r.method].call_n) { lv[TB_ARP] = 10; lv[TB_SEQ] = 10; }
    lv[TB_OCT_DOWN] = lv[TB_OCT_UP] = 8;
}
#endif
