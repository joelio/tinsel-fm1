/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The screens of Jingle It!, Cracker Pull, Snowball Rally and Sleigh Ride (the rules: tn_games.h). Each starts on a
 * page with its picture, its level as stars and the best score; PLAY (or any key or button) starts; PLAY again, or
 * the end, goes back to the page. */
#ifndef TN_PLAY_H
#define TN_PLAY_H

/* ---- shared ---------------------------------------------------------------------------------------------- */
static void pl_sky(void)
{
    int32_t y;
    uint32_t i;
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    for (i = 0; i < 26u; i++) g_pset((int32_t)((i * 97u + 11u) % 240u), (int32_t)((i * 61u + 5u) % 120u), P_GREY);
}
static void pl_start_page(uint32_t pic, const char *name, uint32_t game, uint32_t now)
{
    uint32_t i, lv = set.level[game] % 3u;
    char b[24];
    pl_sky();
    g_disc(108, 96, 56, P_NIGHT2);
    pic_draw(pic, 108, 96, 38, now);
    g_text_c(120, 166, &FONT_S, name, P_SNOW, P_TEXT_AA);
    for (i = 0; i <= lv; i++) pic_star(120 - (int32_t)lv * 14 + (int32_t)i * 28, 196, 10, P_GOLD);
    if (set.best[game]) { memcpy(b, "BEST ", 5); tn_utoa(b + 5, set.best[game]); g_text_c(120, 214, &FONT_S, b, P_SNOW_SH, P_TEXT_AA); }
    ui_play_sign(206, 96, now, 1);
}
static void pl_score(uint32_t score, uint32_t lives, uint32_t max_lives)
{
    char b[12];
    uint32_t i;
    for (i = 0; i < score % 10u; i++) pic_star(10 + (int32_t)i * 16, 14, 6, P_GOLD);
    tn_utoa(b, score);
    g_text(206, 6, &FONT_S, b, P_SNOW, P_TEXT_AA);
    for (i = 0; i < max_lives; i++) g_disc(208 + (int32_t)i * 9, 28, 3, i < lives ? P_RED : P_GREY);
}
static void pl_over(uint32_t score, int best, uint32_t now)
{
    char b[12];
    g_rect(30, 70, 180, 100, P_NIGHT);
    g_rect(30, 70, 180, 3, P_GOLD);
    g_rect(30, 167, 180, 3, P_GOLD);
    pic_star(120, 98, 14, P_GOLD);
    tn_utoa(b, score);
    g_text_c(120, 122, &FONT_L, b, P_GOLD, P_GOLD_AA);
    if (best) g_text_c(120, 76, &FONT_S, (now / 300u) & 1u ? "BEST EVER!" : "", P_YELLOW, 0xFFu);
}
static int pl_keep_best(uint32_t game, uint32_t score)
{
    if (score <= set.best[game]) return 0;
    set.best[game] = (uint16_t)(score > 65535u ? 65535u : score);
    set_touch();
    return 1;
}
static void pl_tune_note(carol_play_t *t, uint32_t inst, uint32_t vel)   /* the next note of Jingle Bells, with its chord */
{
    carol_ev_t ev;
    if (t->done || t->mode != CM_MAGIC) carol_start(t, 0, CM_MAGIC, 0);
    if (!carol_key(t, 0, &ev)) return;
    snd_ping(inst, ev.mel + 12u, vel, 0);
    if (ev.chord) {
        uint8_t ch[4];
        uint32_t n = carol_chord_notes(ev.root, ev.kind, 55u, ch), i;
        snd_ping(TI_SOFTBASS, 40u + ev.root % 12u, 75, 0);
        for (i = 0; i < n; i++) snd_ping(TI_PAD, ch[i], 50, (int32_t)i * 30 - 45);
    }
}

/* ---- Jingle It! ------------------------------------------------------------------------------------------ */
static struct { jingle_t j; uint8_t playing, over, best; uint32_t t_over, t_good, t_bad; } ji;

static void ji_cue(uint32_t call)               /* each call has its own sound: no reading needed */
{
    uint32_t i;
    static const uint8_t SAY[JC_COUNT] = {VO_RING, VO_TWIST, VO_JINGLE, VO_SHH};
    snd_say_at(call == JC_SHH ? 0u : 380u, SAY[call % JC_COUNT]);   /* (a private build says it, after its tune) */
    switch (call) {
    case JC_RING: snd_ping(TI_HANDBELL, 72, 100, 0); snd_ping_at(120, TI_HANDBELL, 76, 100, 0); snd_ping_at(240, TI_HANDBELL, 79, 110, 0); break;
    case JC_TWIST: for (i = 0; i < 8u; i++) snd_ping_at(i * 40u, TI_CHIME, 76u + i * 2u, 60, (int32_t)i * 16 - 56); break;
    case JC_JINGLE: snd_shake(120); snd_shake_at(220, 90); snd_shake_at(440, 120); break;
    default: snd_ping(TI_CELESTA, 72, 40, 0); snd_ping_at(300, TI_CELESTA, 67, 30, 0); break;   /* SHH: soft, falling */
    }
}
static void ji_enter(void) { ji.playing = 0; }
static void ji_input(const tn_input_t *in)
{
    uint32_t now = in->now, e = JE_NONE, k;
    if (!ji.playing) { if (tn_go(in, 0)) { jingle_start(&ji.j, set.level[GM_JINGLE] % 3u, now | 1u, now); ji.playing = 1; ji.over = 0; } return; }
    if (ji.over) { if (now - ji.t_over > 4000u || (tn_go(in, 0) && now - ji.t_over > 1200u)) ji.playing = 0; return; }
    for (k = 0; k < 27u && e == JE_NONE; k++) if (in->key_down >> k & 1u) e = jingle_act(&ji.j, JA_KEY, k, now);
    if (e == JE_NONE && tn_enc_sum(in)) e = jingle_act(&ji.j, JA_TWIST, 0, now);
    if (e == JE_NONE && (in->btn_down & TN_GO_BUTTONS)) {
        if ((in->btn_down >> TB_PLAY & 1u) && !ji.j.active && ji.j.score == 0u && ji.j.level == LV_LITTLE) {}   /* (a press of PLAY to start is not an answer) */
        else e = jingle_act(&ji.j, JA_BUTTON, 0, now);
    }
    if (e == JE_GOOD) { ji.t_good = now; snd_fx(SFX_CHEER); if (ji.j.score % 5u == 0u) snd_say_at(250, VO_WELLDONE); }
    else if (e == JE_WRONG) snd_fx(SFX_NOPE);
    else if (e == JE_MISS) { ji.t_bad = now; snd_ping(TI_TOY, 48, 70, 0); }
    else if (e == JE_OVER) { ji.over = 1; ji.t_over = now; ji.best = (uint8_t)pl_keep_best(GM_JINGLE, ji.j.score); snd_fx(ji.best ? SFX_FANFARE : SFX_CHEER); }
}
static void ji_tick(uint32_t now)
{
    uint32_t e;
    if (!ji.playing || ji.over) return;
    e = jingle_tick(&ji.j, now);
    if (e == JE_CALL) ji_cue(ji.j.call);
    else if (e == JE_GOOD) { ji.t_good = now; snd_fx(SFX_CHEER); }   /* (SHH passed) */
    else if (e == JE_MISS) { ji.t_bad = now; snd_ping(TI_TOY, 48, 70, 0); }
    else if (e == JE_OVER) { ji.over = 1; ji.t_over = now; ji.best = (uint8_t)pl_keep_best(GM_JINGLE, ji.j.score); snd_fx(SFX_CHEER); }
}
static void ji_draw(uint32_t now)
{
    uint32_t t = now - ji.t_good;
    int32_t bounce = t < 300u ? (int32_t)(300u - t) / 30 : 0;
    if (!ji.playing) { pl_start_page(PIC_BELLS, "JINGLE IT!", GM_JINGLE, now); return; }
    pl_sky();
    pl_score(ji.j.score, ji.j.level == LV_LITTLE ? 0u : ji.j.lives, ji.j.level == LV_LITTLE ? 0u : 3u);
    if (ji.j.active) {
        int32_t cx = 120, cy = 100 - bounce;
        switch (ji.j.call) {
        case JC_RING: pic_bell(cx, cy, 40, P_GOLD, P_GOLD_D); g_text_c(120, 160, &FONT_S, "RING IT!", P_GOLD, P_GOLD_AA); break;
        case JC_TWIST: {                         /* a candy cane, spinning */
            int32_t a = (int32_t)(now / 60u) % 8, dx[8] = {0, 21, 30, 21, 0, -21, -30, -21}, dy[8] = {-30, -21, 0, 21, 30, 21, 0, -21}, i;
            for (i = 0; i < 6; i++) g_disc(cx + dx[a] * i / 5, cy + dy[a] * i / 5, 5, i & 1 ? P_RED : P_SNOW);
            for (i = 0; i < 6; i++) g_disc(cx - dx[a] * i / 5, cy - dy[a] * i / 5, 5, i & 1 ? P_SNOW : P_RED);
            g_ring(cx, cy, 44, P_GOLD_D);
            g_text_c(120, 160, &FONT_S, "TWIST IT!", P_GOLD, P_GOLD_AA);
            break;
        }
        case JC_JINGLE: pic_draw(PIC_BELLS, cx, cy, 40, now); ui_play_sign(200, 60, now, 1); g_text_c(120, 160, &FONT_S, "JINGLE IT!", P_GOLD, P_GOLD_AA); break;
        default:                                 /* SHH: a sleeping reindeer */
            g_sprite(&DEER, cx - 32, cy - 10, 4, 0);
            g_text(cx + 34, cy - 40 - (int32_t)((now / 300u) % 3u) * 4, &FONT_S, "z z", P_SNOW_SH, 0xFFu);
            g_text_c(120, 160, &FONT_S, "SHH!", P_SNOW_SH, P_TEXT_AA);
            break;
        }
        if (ji.j.level != LV_LITTLE) {           /* the time left */
            uint32_t el = now - ji.j.t_call, w = el >= ji.j.window ? 0u : (ji.j.window - el) * 200u / ji.j.window;
            g_rect(20, 184, 200, 6, P_NIGHT3);
            g_rect(20, 184, (int32_t)w, 6, w < 60u ? P_RED : P_GOLD);
        }
    }
    if (now - ji.t_bad < 500u) g_rect(0, 0, G_W, 4, P_RED);
    if (ji.over) pl_over(ji.j.score, ji.best, now);
}
static void ji_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    if (!ji.playing) { lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64); return; }
    if (!ji.j.active) return;
    switch (ji.j.call) {
    case JC_RING:
        for (k = 0; k < 27u; k++) {
            int on = ji.j.level == LV_LITTLE ? 1 : ji.j.level == LV_ELF ? catch_lane_of(k) == catch_lane_of(ji.j.key) : k == ji.j.key;
            lv[TN_KEY_LED(k)] = on ? (ji.j.level == LV_LITTLE ? 24u : 64u) : 0u;
        }
        break;
    case JC_JINGLE: for (k = 0; k < 14u; k++) if (TN_GO_BUTTONS >> k & 1u) lv[k] = 10; lv[TB_PLAY] = (uint8_t)tn_pulse(now * 3u, 16, 64); break;
    case JC_TWIST: for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = (k + now / 50u) % 9u == 0u ? 32u : 0u; break;
    default: break;
    }
}

/* ---- Cracker Pull ----------------------------------------------------------------------------------------- */
static const char *const CR_JOKES[8][2] = {
    {"WHAT DO YOU CALL", "A SNOWMAN IN JULY? A PUDDLE"},
    {"WHY DID THE TURKEY", "JOIN THE BAND? DRUMSTICKS"},
    {"WHAT DOES SANTA DO", "IN HIS GARDEN? HO HO HOE"},
    {"WHAT DO ELVES LEARN", "AT SCHOOL? THE ELF-ABET"},
    {"HOW DOES A SNOWMAN", "GET ABOUT? AN ICICLE"},
    {"WHAT IS A CHRISTMAS", "TREE'S FAVOURITE SWEET? ORNA-MINTS"},
    {"WHAT DID ONE CRACKER", "SAY TO THE OTHER? MY POP IS BIGGER"},
    {"WHY IS SANTA SO GOOD", "AT KARATE? HIS BLACK BELT"},
};
static struct { cracker_t c; uint8_t playing, two, joke; uint32_t t_bang, t_beat, last_beat; int32_t shake; } ck;

static void ck_enter(void) { ck.playing = 0; }
static void ck_input(const tn_input_t *in)
{
    uint32_t now = in->now, k;
    if (!ck.playing) {
        if (in->btn_down >> TB_OCT_UP & 1u || in->btn_down >> TB_OCT_DOWN & 1u) { ck.two ^= 1u; snd_ping(TI_GLOCK, ck.two ? 79 : 72, 80, 0); }
        if (tn_go(in, 1u << TB_OCT_UP | 1u << TB_OCT_DOWN)) { cracker_start(&ck.c, set.level[GM_CRACKER] % 3u, ck.two, now); ck.playing = 1; ck.t_bang = 0; ck.last_beat = 0; }
        return;
    }
    if (ck.c.done) { if (now - ck.t_bang > 9000u || (tn_go(in, 0) && now - ck.t_bang > 1500u)) ck.playing = 0; return; }
    for (k = 0; k < 27u; k++)
        if (in->key_down >> k & 1u) {
            uint32_t side = k <= 12u ? 0u : k >= 14u ? 1u : 2u, p;
            if (side == 2u) continue;
            if (side == 1u && !ck.c.two) ck.c.two = 1;   /* someone took the other end: two players */
            p = cracker_tap(&ck.c, side, now);
            snd_ping(TI_TOY, side ? 67u + p * 2u : 60u + p * 2u, 40u + p * 15u, side ? 50 : -50);
            ck.shake = (int32_t)p;
        }
}
static void ck_bang(uint32_t now)
{
    ck.t_bang = now;
    ck.joke = (uint8_t)((now / 7u) % 8u);
    snd_ping(TI_TOY, 36, 127, 0);                /* BANG */
    snd_shake(127); snd_shake_at(80, 127);
    snd_ping_at(40, TI_CHURCH, 48, 90, 0);
    snd_fx(SFX_SPARKLE);
    snd_ping_at(700, TI_HANDBELL, 79, 90, -40); snd_ping_at(850, TI_HANDBELL, 84, 90, 40);
}
static void ck_tick(uint32_t now)
{
    uint32_t beat;
    if (!ck.playing || ck.c.done) return;
    if (cracker_tick(&ck.c, now)) { snd_ping(TI_TOY, 70, 45, 50); ck.shake = 2; }
    beat = (now - ck.c.t0) / CR_BEAT;
    if (ck.c.level != LV_LITTLE && beat != ck.last_beat) { ck.last_beat = beat; ck.t_beat = now; snd_ping(TI_GLOCK, beat % 4u ? 84 : 91, 40, 0); }
    if (ck.c.done) ck_bang(now);
}
static void ck_hat(int32_t cx, int32_t y, uint32_t c)   /* a paper crown */
{
    int32_t i;
    g_rect(cx - 18, y, 36, 10, c);
    for (i = 0; i < 4; i++) g_tri_up(cx - 18 + 4 + i * 10, y - 8, cx - 18 + i * 10, cx - 18 + 9 + i * 10, y, c);
}
static void ck_draw(uint32_t now)
{
    int32_t stretch, mid, wob;
    uint32_t total;
    if (!ck.playing) {
        pl_start_page(PIC_PRESENT, ck.two ? "CRACKER PULL: TWO" : "CRACKER PULL", GM_CRACKER, now);
        g_text(4, 4, &FONT_S, ck.two ? "OCT: 2 PLAYERS" : "OCT: 1 PLAYER", P_DIM, 0xFFu);
        return;
    }
    pl_sky();
    total = (uint32_t)(ck.c.pull[0] + ck.c.pull[1]);
    if (ck.c.done) {
        uint32_t t = now - ck.t_bang;
        if (t < 120u) { g_clear(P_WHITE); return; }   /* the flash */
        g_text_c(120, 40, &FONT_L, "BANG!", P_GOLD, P_GOLD_AA);
        if (ck.c.winner != 1u) ck_hat(60, 96, P_YELLOW);
        if (ck.c.winner != 0u) ck_hat(180, 96, P_PINK);
        g_rect(20, 110, 70, 26, P_RED); g_rect(150, 110, 70, 26, P_RED);   /* the two halves */
        g_rect(30, 110, 6, 26, P_GOLD); g_rect(204, 110, 6, 26, P_GOLD);
        if (t > 1200u) {
            g_text_c(120, 160, &FONT_S, CR_JOKES[ck.joke][0], P_SNOW, P_TEXT_AA);
            g_text_c(120, 180, &FONT_S, CR_JOKES[ck.joke][1], P_SNOW, P_TEXT_AA);
        }
        return;
    }
    stretch = (int32_t)total * 40 / CR_BANG;    /* it stretches as it is pulled */
    mid = 120 + (ck.c.pull[1] - ck.c.pull[0]) * 2;
    if (mid < 80) mid = 80;
    if (mid > 160) mid = 160;
    wob = ck.shake ? (int32_t)((now / 30u) % 3u) - 1 : 0;
    if (now % 200u < 40u) ck.shake = 0;
    g_rect(mid - 40 - stretch, 106 + wob, 80 + 2 * stretch, 28, P_RED);
    g_rect(mid - 60 - stretch, 100 + wob, 22, 40, P_RED_D);   /* the ends */
    g_rect(mid + 38 + stretch, 100 + wob, 22, 40, P_RED_D);
    g_rect(mid - 38 - stretch, 106 + wob, 4, 28, P_GOLD);
    g_rect(mid + 34 + stretch, 106 + wob, 4, 28, P_GOLD);
    g_rect(mid - 4, 112 + wob, 8, 16, P_GOLD);
    g_disc(mid - 70 - stretch, 120, 10, P_SKIN); g_disc(mid + 70 + stretch, 120, 10, P_SKIN);   /* hands */
    if (ck.c.level != LV_LITTLE) {               /* the beat: pull when it flashes */
        uint32_t t = now - ck.t_beat;
        g_disc(120, 190, t < 120u ? 12 : 7, t < 120u ? P_GOLD : P_NIGHT3);
    }
    g_text(8, 206, &FONT_S, "LOW KEYS", P_DIM, 0xFFu);
    g_text(232 - text_w(&FONT_S, ck.c.two ? "HIGH KEYS" : "ELF"), 206, &FONT_S, ck.c.two ? "HIGH KEYS" : "ELF", P_DIM, 0xFFu);
}
static void ck_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k, beat = ck.playing && !ck.c.done && ck.c.level != LV_LITTLE && now - ck.t_beat < 120u;
    if (!ck.playing) { lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64); lv[TB_OCT_UP] = lv[TB_OCT_DOWN] = 10; return; }
    if (ck.c.done) return;
    for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = k == 13u ? 0u : beat ? 64u : (k <= 12u || ck.c.two) ? 8u : 0u;
}

/* ---- Snowball Rally --------------------------------------------------------------------------------------- */
static struct { rally_t r; carol_play_t tune; uint8_t playing; uint32_t t_side[2], t_hit; uint16_t run; } rl;

static void rl_enter(void) { rl.playing = 0; }
static void rl_input(const tn_input_t *in)
{
    int32_t l = in->enc[TE_SELECT] + in->enc[TE_KNOB1] + in->enc[TE_KNOB2], r = in->enc[TE_ALGORITHM] + in->enc[TE_KNOB3] + in->enc[TE_KNOB4] + in->enc[TE_PRESETS];
    if (!rl.playing) {
        if (tn_go(in, 0)) { rally_start(&rl.r, set.level[GM_RALLY] % 3u, in->now | 1u); carol_start(&rl.tune, 0, CM_MAGIC, 0); rl.playing = 1; rl.run = 0; rl.t_side[0] = rl.t_side[1] = in->now - 10000u; }
        return;
    }
    if (in->btn_down >> TB_PLAY & 1u) { rl.playing = 0; return; }
    if (l) { rally_move(&rl.r, 0, -l); rl.t_side[0] = in->now; }   /* clockwise: up */
    if (r) { rally_move(&rl.r, 1, -r); rl.t_side[1] = in->now; }
}
static void rl_tick(uint32_t now, uint32_t dt)
{
    uint32_t e, s;
    if (!rl.playing) return;
    for (s = 0; s < 2u; s++) if (now - rl.t_side[s] > 4000u) rally_elf(&rl.r, s, dt);   /* no one at this end: an elf plays it */
    e = rally_step(&rl.r, dt);
    if (e == RE_HIT_L || e == RE_HIT_R) { pl_tune_note(&rl.tune, TI_GLOCK, 105); rl.t_hit = now; rl.run++; if (pl_keep_best(GM_RALLY, rl.run)) {} }
    else if (e == RE_WALL) snd_ping(TI_CHIME, 96, 30, 0);
    else if (e == RE_MISS_L || e == RE_MISS_R) { snd_ping(TI_TOY, 48, 70, e == RE_MISS_L ? -60 : 60); snd_fx(SFX_BOING); rl.run = 0; }
}
static void rl_snowmen(int32_t x, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n && i < 6u; i++) { int32_t sx = x + (int32_t)(i % 3u) * 14, sy = 224 - (int32_t)(i / 3u) * 22; g_disc(sx, sy, 6, P_SNOW); g_disc(sx, sy - 9, 4, P_SNOW); g_pset(sx + 1, sy - 9, P_CARROT); }
}
static void rl_draw(uint32_t now)
{
    int32_t x;
    char b[8];
    if (!rl.playing) { pl_start_page(PIC_STAR, "SNOWBALL RALLY", GM_RALLY, now); return; }
    pl_sky();
    for (x = 0; x < G_W; x += 16) { g_rect(x, RL_BOT + 2, 15, 6, P_ROOF); g_rect(x + 8, RL_TOP - 8, 7, 6, P_ROOF); }   /* the roofs */
    g_rect(0, RL_TOP - 2, G_W, 2, P_SNOW);
    g_rect(0, RL_BOT, G_W, 2, P_SNOW);
    for (x = RL_TOP; x < RL_BOT; x += 12) g_rect(119, x, 2, 6, P_NIGHT3);   /* the middle line */
    g_rect(RL_X0 - 8, rl.r.py[0] - rl.r.half, 6, 2 * rl.r.half, P_RED);    /* the paddles: two mittens */
    g_rect(RL_X1 + 2, rl.r.py[1] - rl.r.half, 6, 2 * rl.r.half, P_BLUE);
    g_disc(RL_X0 - 5, rl.r.py[0] - rl.r.half, 4, P_RED);
    g_disc(RL_X1 + 5, rl.r.py[1] - rl.r.half, 4, P_BLUE);
    if (!rl.r.wait) g_disc(rl.r.bx >> 8, rl.r.by >> 8, now - rl.t_hit < 120u ? 5 : 4, P_SNOW);
    rl_snowmen(20, rl.r.snowmen[0]);
    rl_snowmen(186, rl.r.snowmen[1]);
    tn_utoa(b, rl.run);
    g_text_c(120, 4, &FONT_S, b, P_GOLD, P_GOLD_AA);
}
static void rl_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    if (!rl.playing) { lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64); return; }
    for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = now - rl.t_hit < 150u ? 40u : 2u;
}

/* ---- Sleigh Ride ------------------------------------------------------------------------------------------ */
static struct { ride_t d; carol_play_t tune; uint8_t playing, over, best; uint32_t t_rung, t_over, t_miss; int32_t rung_x; } rd;

static void rd_enter(void) { rd.playing = 0; }
static void rd_input(const tn_input_t *in)
{
    uint32_t now = in->now, k;
    int32_t steer = in->enc_x + in->enc_y;
    if (!rd.playing) { if (tn_go(in, 0)) { ride_start(&rd.d, set.level[GM_RIDE] % 3u, now | 1u); carol_start(&rd.tune, 0, CM_MAGIC, 0); rd.playing = 1; rd.over = 0; } return; }
    if (rd.over) { if (now - rd.t_over > 4000u || (tn_go(in, 0) && now - rd.t_over > 1200u)) rd.playing = 0; return; }
    if (in->btn_down >> TB_PLAY & 1u) { rd.playing = 0; return; }
    if (steer) ride_steer(&rd.d, steer);
    for (k = 0; k < 27u; k++)
        if (in->key_down >> k & 1u) {
            int p = ride_key(&rd.d, k);
            if (p) { pl_tune_note(&rd.tune, TI_HANDBELL, 110); rd.t_rung = now; rd.rung_x = ride_post_x(rd.d.post[p - 1].lane); }
            else snd_ping(TI_CHIME, gr_note(k) + 12u, 30, 0);
        }
}
static void rd_tick(uint32_t now, uint32_t dt)
{
    uint32_t e;
    if (!rd.playing || rd.over) return;
    e = ride_step(&rd.d, dt);
    if (e == RD_MISSED) { rd.t_miss = now; snd_ping(TI_TOY, 48, 60, 0); }
    if (e == RD_OVER) { rd.over = 1; rd.t_over = now; rd.best = (uint8_t)pl_keep_best(GM_RIDE, rd.d.score); snd_fx(rd.best ? SFX_FANFARE : SFX_CHEER); }
}
static void rd_project(int32_t wx, int32_t z, int32_t *sx, int32_t *sy, int32_t *scale)   /* world to screen (Q8 scale) */
{
    int32_t d = z + 120;
    if (d < 20) d = 20;
    *scale = 256 * 120 / d;
    *sx = 120 + (wx - rd.d.x) * *scale / 256;
    *sy = 76 + 150 * 120 / d;
}
static void rd_draw(uint32_t now)
{
    uint32_t i;
    int32_t y;
    if (!rd.playing) { pl_start_page(PIC_TREE, "SLEIGH RIDE", GM_RIDE, now); return; }
    for (y = 0; y < 78; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 10u);
    for (i = 0; i < 20u; i++) g_pset((int32_t)((i * 97u + 11u) % 240u), (int32_t)((i * 37u + 5u) % 70u), P_GREY);
    g_disc(200, 30, 12, P_MOON);
    g_rect(0, 78, G_W, 162, P_SNOW);
    for (y = 80; y < 240; y += 2) {               /* the track, narrowing to the horizon, moving */
        int32_t sx, sy, sc, w;
        int32_t z = 120 * 150 / (y - 76) - 120;
        rd_project(0, z, &sx, &sy, &sc);
        w = 110 * sc / 256;
        g_hline(sx - w, sx + w, y, ((uint32_t)(z + (int32_t)(now / 3u)) / 60u) & 1u ? P_SNOW_SH : P_SNOW);
    }
    for (i = 0; i < 10u; i++) {                  /* trees at the sides */
        int32_t z = (int32_t)((i * 100u + 1000u - (now * (uint32_t)rd.d.speed / 1000u) % 100u) % 1000u), sx, sy, sc, side;
        for (side = -1; side <= 1; side += 2) {
            rd_project(side * 170, z, &sx, &sy, &sc);
            g_tri_up(sx, sy - 40 * sc / 256, sx - 14 * sc / 256, sx + 14 * sc / 256, sy, P_GREEN_D);
        }
    }
    {                                             /* the bell posts, far to near */
        int lit = ride_lit_lane(&rd.d);
        uint32_t order[RD_NPOST], n = 0, j;
        for (i = 0; i < RD_NPOST; i++) if (rd.d.post[i].on) order[n++] = i;
        for (i = 0; i + 1u < n; i++) for (j = i + 1u; j < n; j++) if (rd.d.post[order[j]].z > rd.d.post[order[i]].z) { uint32_t t = order[i]; order[i] = order[j]; order[j] = t; }
        for (j = 0; j < n; j++) {
            int32_t sx, sy, sc, h;
            i = order[j];
            rd_project(ride_post_x(rd.d.post[i].lane), rd.d.post[i].z, &sx, &sy, &sc);
            h = 60 * sc / 256;
            g_rect(sx - 1 - sc / 256, sy - h, 2 + 2 * sc / 256, h, P_BROWN);
            pic_bell(sx, sy - h - 6 * sc / 256, 4 + 8 * sc / 256, rd.d.post[i].rung ? P_YELLOW : (int)rd.d.post[i].lane == lit ? P_GOLD : P_GREY, P_GOLD_D);
        }
    }
    {                                             /* our sleigh */
        int32_t sx = 120;
        g_rect(sx - 26, 206, 44, 18, P_RED);
        g_rect(sx + 14, 194, 8, 18, P_RED);
        g_rect(sx - 32, 226, 58, 3, P_GOLD);
        g_rect(sx - 18, 196, 12, 10, P_GREEN);
        g_disc(sx - 4, 192, 6, P_RED);           /* Santa's back */
        g_disc(sx - 4, 184, 4, P_SNOW);
    }
    if (now - rd.t_rung < 400u) { uint32_t j; for (j = 0; j < 10u; j++) g_rect(120 + (int32_t)(j * 13u % 40u) - 20, 120 - (int32_t)((now - rd.t_rung) / 10u) - (int32_t)(j * 7u % 20u), 2, 2, P_YELLOW); }
    if (now - rd.t_miss < 400u) g_rect(0, 0, G_W, 4, P_RED);
    pl_score(rd.d.score, rd.d.level == LV_LITTLE ? 0u : rd.d.lives, rd.d.level == LV_LITTLE ? 0u : 3u);
    if (rd.over) pl_over(rd.d.score, rd.best, now);
}
static void rd_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    int lit;
    if (!rd.playing) { lv[TB_PLAY] = (uint8_t)tn_pulse(now, 12, 64); return; }
    lit = rd.over ? -1 : ride_lit_lane(&rd.d);
    for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = lit >= 0 && (rd.d.level == LV_LITTLE || catch_lane_of(k) == (uint32_t)lit) ? 64u : 2u;
}
#endif
