/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Grotto: home (DESIGN section 7.1). A tree with a bauble above every key, snow, presents, a snowman, the moon,
 * and a robin the knobs steer. Nothing here can go wrong and nothing here changes a setting.
 *
 *   keys        a handbell each (a major pentatonic: any cluster is sweet); the bauble above the key lights
 *   many keys   a sleigh-bell flurry and the whole tree sparkles
 *   knobs       the robin (SELECT KNOB1 PRESETS across, ALGORITHM KNOB2..4 up and down); what it lands on reacts
 *   PLAY        the big moment: a fanfare, a snowstorm, every light
 *   OCT- OCT+   the bells lower or higher, the snow lighter or heavier
 *   FX SEL ENV LFO GLO ARP SEQ REC   a reindeer, a shooting star, the moon winks, a snow puff, the lights chase,
 *               a sparkle, the church bell, the robin sings
 *   left alone  after a minute the Grotto plays to itself; after five the screen gets sleepy */
#ifndef TN_GROTTO_H
#define TN_GROTTO_H

#define GR_APEX_Y 34
#define GR_BASE_Y 196
#define GR_HALF 118
#define GR_NFLAKE 64u
#define GR_NHOT 8u


static struct {
    int16_t bx[TN_NKEYS], by[TN_NKEYS];         /* each key's bauble */
    uint32_t lit_until[TN_NKEYS];
    uint32_t fx[TN_NKEYS];                      /* each key's LED: when it was struck */
    int32_t fx_q8[GR_NFLAKE], fy_q8[GR_NFLAKE]; /* snowflakes */
    uint8_t fsz[GR_NFLAKE], fspd[GR_NFLAKE];
    uint32_t nflake, storm_until, sparkle_until, chase_from, wink_from, deer_from, star_from, puff_from;
    int32_t rx, ry;                             /* the robin, pixels */
    uint8_t rflip, hot_in;                      /* which hot spot the robin sits on (+1), 0 none */
    uint32_t hot_from[GR_NHOT];
    int32_t oct;
    uint32_t last_input, idle_next, idle_step, rng, recent[6];
    uint8_t dim;
} gr;

/* the hot spots: x, y, radius (rough circles are plenty for a robin) */
static const int16_t GR_HOT[GR_NHOT][3] = {
    {120, 28, 16},                              /* 0 the star */
    {32, 30, 18},                               /* 1 the moon */
    {64, 194, 14}, {90, 200, 10}, {158, 198, 12}, {184, 194, 12},   /* 2..5 the presents */
    {214, 198, 22},                             /* 6 the snowman */
    {120, 206, 10},                             /* 7 the trunk (a little mouse lives there) */
};

static uint32_t gr_note(uint32_t k)            /* key -> a handbell in C major pentatonic, near the key's own pitch */
{
    static const uint8_t SNAP[12] = {0, 0, 2, 2, 4, 4, 7, 7, 7, 9, 9, 12};
    uint32_t m = 53u + k, pc = m % 12u;
    int32_t n = (int32_t)(m - pc + SNAP[pc]) + 12 * (gr.oct + 1);
    return n < 36 ? 36u : n > 108 ? 108u : (uint32_t)n;
}
static int32_t gr_pan(int32_t x) { return (x - 120) * 64 / 120; }

static void gr_enter(void)
{
    uint32_t k, s = 0x51u;
    for (k = 0; k < TN_NKEYS; k++) {
        int32_t x = 28 + (int32_t)k * 184 / 26, dx = x < 120 ? 120 - x : x - 120;
        int32_t ymin = GR_APEX_Y + 16 + dx * (GR_BASE_Y - GR_APEX_Y) / GR_HALF, ymax = GR_BASE_Y - 10;
        if (ymin > ymax) ymin = ymax;
        gr.bx[k] = (int16_t)x;
        gr.by[k] = (int16_t)(ymin + (int32_t)(tn_rand(&s) % (uint32_t)(ymax - ymin + 1)));
        gr.lit_until[k] = 0;
    }
    gr.nflake = 24;
    for (k = 0; k < GR_NFLAKE; k++) {
        gr.fx_q8[k] = (int32_t)(tn_rand(&s) % 240u) << 8;
        gr.fy_q8[k] = (int32_t)(tn_rand(&s) % 240u) << 8;
        gr.fsz[k] = (uint8_t)(1u + tn_rand(&s) % 2u);
        gr.fspd[k] = (uint8_t)(40u + tn_rand(&s) % 80u);
    }
    if (!gr.rx) { gr.rx = 40; gr.ry = 120; }
    gr.rng = 0xC0FFEEu;
    gr.last_input = gr.idle_next = 0;
    gr.hot_in = 0;
}

static void gr_strike(uint32_t k, uint32_t now, uint32_t vel)
{
    snd_ping(TI_HANDBELL, gr_note(k), vel, gr_pan(gr.bx[k]));
    gr.lit_until[k] = now + 900u;
    gr.fx[k] = now;
}

static void gr_hot(uint32_t i, uint32_t now)   /* the robin landed on something */
{
    static const uint8_t NOTE[GR_NHOT] = {91, 84, 72, 76, 79, 74, 67, 86};
    gr.hot_from[i] = now;
    switch (i) {
    case 0: snd_fx(SFX_SPARKLE); break;
    case 1: snd_ping(TI_CELESTA, 84, 90, -50); snd_ping_at(160, TI_CELESTA, 79, 80, -50); break;
    case 6: snd_fx(SFX_BOING); break;
    case 7: snd_ping(TI_TOY, 96, 60, 0); snd_ping_at(90, TI_TOY, 100, 50, 0); break;
    default: snd_fx(SFX_POP); snd_ping_at(60, TI_GLOCK, NOTE[i], 90, gr_pan(GR_HOT[i][0])); break;
    }
}

static void gr_input(const tn_input_t *in)
{
    uint32_t k, now = in->now, down = in->key_down, b = in->btn_down;
    int any = down || b || in->enc_x || in->enc_y;
    if (any) {
        gr.last_input = now;
        if (gr.dim) { gr.dim = 0; pal_load(0); }
    }
    for (k = 0; k < TN_NKEYS; k++)
        if (down >> k & 1u) gr_strike(k, now, 100u);
    if (down) {                                 /* a flurry of keys: sleigh bells and a sparkling tree */
        uint32_t i, n = 0;
        for (i = 5; i > 0; i--) gr.recent[i] = gr.recent[i - 1u];
        gr.recent[0] = now;
        for (i = 0; i < 6u; i++) if (gr.recent[i] && now - gr.recent[i] < 180u) n++;
        if (tn_popcount(down) >= 4u || n >= 5u) {
            snd_shake(110);
            gr.sparkle_until = now + 1200u;
        }
    }
    if (in->enc_x || in->enc_y) {               /* the robin */
        gr.rx += in->enc_x * 7;
        gr.ry -= in->enc_y * 7;
        if (in->enc_x) gr.rflip = in->enc_x < 0;
        if (gr.rx < 0) gr.rx = 0;
        if (gr.rx > 216) gr.rx = 216;
        if (gr.ry < 6) gr.ry = 6;
        if (gr.ry > 214) gr.ry = 214;
        {
            uint32_t i, on = 0;
            int32_t cx = gr.rx + 12, cy = gr.ry + 10;
            for (i = 0; i < GR_NHOT; i++) {
                int32_t dx = cx - GR_HOT[i][0], dy = cy - GR_HOT[i][1];
                if (dx * dx + dy * dy <= (GR_HOT[i][2] + 6) * (GR_HOT[i][2] + 6)) { on = i + 1u; break; }
            }
            if (on && on != gr.hot_in) gr_hot(on - 1u, now);
            if (!on)                             /* over a bauble: it rings */
                for (i = 0; i < TN_NKEYS; i++) {
                    int32_t dx = cx - gr.bx[i], dy = cy - gr.by[i];
                    if (dx * dx + dy * dy <= 49 && now > gr.lit_until[i]) { gr_strike(i, now, 70u); break; }
                }
            gr.hot_in = (uint8_t)on;
        }
    }
    if (b >> TB_PLAY & 1u) {                    /* the big moment */
        snd_fx(SFX_FANFARE);
        snd_say_at(900, VO_HOHOHO);
        gr.storm_until = now + 4000u;
        gr.sparkle_until = now + 2500u;
        gr.chase_from = now;
    }
    if (b >> TB_OCT_UP & 1u && gr.oct < 2) { gr.oct++; snd_ping(TI_HANDBELL, gr_note(13), 90, 0); }
    if (b >> TB_OCT_DOWN & 1u && gr.oct > -1) { gr.oct--; snd_ping(TI_HANDBELL, gr_note(13), 90, 0); }
    if (b >> TB_FX & 1u) { gr.deer_from = now; snd_shake(90); snd_shake_at(300, 70); snd_shake_at(600, 90); }
    if (b >> TB_SEL & 1u) { gr.star_from = now; snd_fx(SFX_TWINKLE); }
    if (b >> TB_ENV & 1u) { gr.wink_from = now; snd_ping(TI_CELESTA, 88, 80, -50); }
    if (b >> TB_LFO & 1u) { gr.puff_from = now; snd_ping(TI_CHIME, 93, 60, 0); snd_ping_at(70, TI_CHIME, 98, 50, 0); }
    if (b >> TB_GLO & 1u) { gr.chase_from = now; for (k = 0; k < 9u; k++) snd_ping_at(k * 70u, TI_GLOCK, gr_note(k * 3u) + 12u, 60, (int32_t)k * 14 - 56); }
    if (b >> TB_ARP & 1u) { snd_fx(SFX_SPARKLE); gr.sparkle_until = now + 900u; }
    if (b >> TB_SEQ & 1u) snd_ping(TI_CHURCH, 60, 110, 0);
    if (b >> TB_REC & 1u) { snd_ping(TI_TOY, 98, 70, gr_pan(gr.rx)); snd_ping_at(80, TI_TOY, 103, 60, gr_pan(gr.rx)); snd_ping_at(150, TI_TOY, 98, 50, gr_pan(gr.rx)); }

    if (!gr.last_input) gr.last_input = now;    /* left alone: it plays to itself, then gets sleepy */
    if (now - gr.last_input > 60000u && (int32_t)(now - gr.idle_next) >= 0) {
        static const uint8_t TUNE[] = {9, 9, 9, 9, 9, 9, 9, 13, 3, 6, 9, 255, 10, 10, 10, 10, 10, 9, 9, 9, 9, 6, 6, 9, 6, 13, 255};   /* (keys) */
        uint32_t kk = TUNE[gr.idle_step++ % sizeof TUNE];
        if (kk < TN_NKEYS) gr_strike(kk, now, 70u);
        gr.idle_next = now + (gr.idle_step % 4u == 3u ? 700u : 350u);
    }
    if (!gr.dim && now - gr.last_input > 300000u) { gr.dim = 1; pal_load(2); }
}

static void gr_draw_tree(uint32_t now)
{
    uint32_t k;
    g_tri_up(120, GR_APEX_Y, 120 - 52, 120 + 52, 96, P_GREEN_D);
    g_tri_up(120, 64, 120 - 86, 120 + 86, 146, P_GREEN_D);
    g_tri_up(120, 100, 120 - GR_HALF, 120 + GR_HALF, GR_BASE_Y, P_GREEN_D);
    g_tri_up(120, GR_APEX_Y + 6, 120 - 40, 120 + 40, 96, P_GREEN);   /* a lighter face */
    g_tri_up(120, 72, 120 - 70, 120 + 70, 146, P_GREEN);
    g_tri_up(120, 108, 120 - 100, 120 + 100, GR_BASE_Y - 4, P_GREEN);
    g_rect(110, GR_BASE_Y, 20, 14, P_BROWN);
    for (k = 0; k < 6u; k++) {                   /* tinsel garlands */
        int32_t y = 60 + (int32_t)k * 24, x;
        for (x = -60; x <= 60; x += 3) {
            int32_t yy = y + (x * x) / 400 + (x / 4), half = (yy - GR_APEX_Y) * GR_HALF / (GR_BASE_Y - GR_APEX_Y) - 8;
            if (x > -half && x < half && yy < GR_BASE_Y - 4) g_pset(120 + x, yy, ((uint32_t)(x + 60) / 3u + now / 120u) % 5u ? P_GOLD_D : P_GOLD);
        }
    }
    for (k = 0; k < TN_NKEYS; k++) {             /* the baubles */
        int lit = now < gr.lit_until[k], sp = now < gr.sparkle_until && ((now / 80u + k * 7u) % 5u == 0u);
        uint32_t chase = gr.chase_from && now - gr.chase_from < 2000u && (now - gr.chase_from) / 60u % TN_NKEYS == k;
        uint32_t c = (k * 3u) % 8u;
        int32_t r = tn_black(k) ? 4 : 5;
        if (lit || sp || chase) {
            g_disc(gr.bx[k], gr.by[k], r + 3, P_BAUBLE_LIT + c);
            g_disc(gr.bx[k], gr.by[k], r + 1, P_WHITE);
            g_disc(gr.bx[k], gr.by[k], r, P_BAUBLE_LIT + c);
        } else {
            g_disc(gr.bx[k], gr.by[k], r, P_BAUBLE + c);
            g_pset(gr.bx[k] - 2, gr.by[k] - 2, P_BAUBLE_LIT + c);
        }
        g_pset(gr.bx[k], gr.by[k] - r - 1, P_GOLD_D);
    }
    {                                            /* the star */
        uint32_t t = gr.hot_from[0] && now - gr.hot_from[0] < 1500u, c = t && (now / 100u) & 1u ? P_WHITE : P_GOLD;
        int32_t s = t ? 15 : 12;
        g_tri_up(120, 28 - s, 120 - s * 2 / 3, 120 + s * 2 / 3, 28 + s / 3, c);
        g_rect(120 - s, 24, 2 * s + 1, 4, c);
        g_tri_up(120, 20, 120 - s, 120 + s, 28, c);
        if (t) { g_line(120, 4, 120, 10, P_WHITE); g_line(98, 18, 104, 22, P_WHITE); g_line(142, 18, 136, 22, P_WHITE); }
    }
}

static void gr_draw(uint32_t now)
{
    uint32_t k;
    int32_t y;
    for (y = 0; y < 200; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 25u);
    {                                            /* the moon (it winks) */
        uint32_t w = gr.wink_from && now - gr.wink_from < 600u, h = gr.hot_from[1] && now - gr.hot_from[1] < 1000u;
        g_disc(32, 30, h ? 16 : 14, P_MOON);
        g_disc(39, 26, 11, P_SKY);              /* a crescent */
        if (w) g_line(24, 30, 30, 30, P_GREY); else g_pset(27, 30, P_GREY);
    }
    for (k = 0; k < 40u; k++) {                  /* stars, twinkling */
        uint32_t x = (k * 97u + 13u) % 240u, yy = (k * 53u + 7u) % 120u, tw = (now / 300u + k) % 11u == 0u;
        if ((x > 14u && x < 52u && yy < 50u) || (x > 96u && x < 144u && yy < 44u)) continue;
        g_pset((int32_t)x, (int32_t)yy, tw ? P_WHITE : P_GREY);
    }
    if (gr.star_from && now - gr.star_from < 900u) {   /* a shooting star */
        int32_t t = (int32_t)(now - gr.star_from), sx = 230 - t / 4, sy = 10 + t / 12;
        g_line(sx, sy, sx + 26, sy - 8, P_GREY);
        g_line(sx, sy, sx + 12, sy - 4, P_WHITE);
        g_rect(sx - 1, sy - 1, 3, 3, P_WHITE);
    }
    if (gr.deer_from && now - gr.deer_from < 2600u) {   /* a reindeer gallops across the sky */
        int32_t t = (int32_t)(now - gr.deer_from), dx = -40 + t * 300 / 2600, dy = 70 - ((t / 90) % 2) * 3;
        g_sprite(&DEER, dx, dy, 2, 0);
    }
    gr_draw_tree(now);
    g_rect(0, 200, G_W, 40, P_SNOW);             /* the snow on the ground */
    g_rect(0, 200, G_W, 2, P_SNOW_SH);
    for (k = 0; k < 4u; k++) {                   /* presents (they wobble when the robin lands) */
        static const int16_t P[4][5] = {{52, 184, 26, 20, P_RED}, {80, 192, 20, 14, P_BLUE}, {146, 188, 24, 18, P_PURPLE}, {172, 184, 24, 22, P_GREEN}};
        uint32_t h = gr.hot_from[2u + k] && now - gr.hot_from[2u + k] < 800u;
        int32_t wob = h ? (int32_t)((now / 60u) % 3u) - 1 : 0, lift = h ? 4 - (int32_t)((now - gr.hot_from[2u + k]) / 200u) : 0;
        int32_t x = P[k][0] + wob, yy = P[k][1] - (lift > 0 ? lift : 0);
        g_rect(x, yy, P[k][2], P[k][3], (uint32_t)P[k][4]);
        g_rect(x + P[k][2] / 2 - 2, yy, 4, P[k][3], P_GOLD);
        g_rect(x, yy + P[k][3] / 2 - 2, P[k][2], 4, P_GOLD);
        g_disc(x + P[k][2] / 2 - 4, yy - 2, 3, P_GOLD);
        g_disc(x + P[k][2] / 2 + 4, yy - 2, 3, P_GOLD);
        if (h && now - gr.hot_from[2u + k] < 500u) {   /* a sparkle out of the top */
            uint32_t j;
            for (j = 0; j < 6u; j++) g_pset(x + P[k][2] / 2 + (int32_t)(j * 7u % 13u) - 6, yy - 6 - (int32_t)((now - gr.hot_from[2u + k]) / 30u) - (int32_t)j, P_YELLOW);
        }
    }
    {                                            /* the snowman (his hat hops) */
        uint32_t h = gr.hot_from[6] && now - gr.hot_from[6] < 700u;
        int32_t hop = h ? (int32_t)(10 - (int32_t)(now - gr.hot_from[6]) / 35) : 0;
        if (hop < 0) hop = -hop > 10 ? 0 : 0;
        g_disc(214, 212, 15, P_SNOW_SH);
        g_disc(213, 211, 14, P_SNOW);
        g_disc(214, 190, 10, P_SNOW);
        g_pset(210, 188, P_BLACK); g_pset(218, 188, P_BLACK);
        g_rect(214, 191, 7, 2, P_CARROT);
        g_rect(204, 199, 20, 3, P_RED);          /* a scarf */
        g_rect(206, 176 - hop, 16, 3, P_BLACK);
        g_rect(209, 166 - hop, 10, 11, P_BLACK);
        g_pset(214, 206, P_BLACK); g_pset(214, 212, P_BLACK);
    }
    if (gr.hot_from[7] && now - gr.hot_from[7] < 900u) {   /* the mouse peeks out */
        g_disc(122, 206, 3, P_GREY);
        g_disc(119, 203, 2, P_PINK);
        g_disc(125, 203, 2, P_PINK);
    }
    if (gr.puff_from && now - gr.puff_from < 900u) {   /* a puff of snow off the tree */
        uint32_t j, t = now - gr.puff_from;
        for (j = 0; j < 20u; j++)
            g_pset(120 + (int32_t)((j * 37u) % 120u) - 60 + (int32_t)((j & 1u) ? t / 30u : 0u - t / 30u), 60 + (int32_t)((j * 23u) % 90u) + (int32_t)(t / 15u), P_SNOW);
    }
    {                                            /* snow */
        uint32_t n = now < gr.storm_until ? GR_NFLAKE : gr.nflake + (uint32_t)(gr.oct + 1) * 8u;
        if (n > GR_NFLAKE) n = GR_NFLAKE;
        for (k = 0; k < n; k++) {
            int32_t x = gr.fx_q8[k] >> 8, yy = gr.fy_q8[k] >> 8;
            g_rect(x, yy, gr.fsz[k], gr.fsz[k], P_SNOW);
        }
    }
    if (set.doors >> 1 & 1u) {                   /* door 2: a candy cane in the snow */
        g_rect(18, 204, 4, 26, P_SNOW);
        for (y = 206; y < 230; y += 6) g_rect(18, y, 4, 3, P_RED);
        g_ring(24, 205, 6, P_RED); g_ring(24, 205, 5, P_SNOW);
        g_rect(18, 205, 12, 6, P_SNOW); g_rect(18, 205, 4, 2, P_RED);
    }
    if (set.doors >> 3 & 1u) {                   /* door 4: a gingerbread man */
        g_disc(40, 212, 5, P_BROWN);
        g_rect(36, 216, 9, 10, P_BROWN);
        g_rect(32, 218, 17, 4, P_BROWN);
        g_rect(36, 226, 3, 6, P_BROWN); g_rect(42, 226, 3, 6, P_BROWN);
        g_pset(38, 211, P_SNOW); g_pset(42, 211, P_SNOW); g_pset(40, 219, P_RED); g_pset(40, 222, P_RED);
    }
    if (set.doors >> 5 & 1u) {                   /* door 6: a stocking on the tree */
        g_rect(148, 150, 8, 14, P_RED);
        g_rect(148, 162, 14, 6, P_RED);
        g_rect(147, 148, 10, 4, P_SNOW);
    }
    ui_home_hint(206, 4, now);                  /* HOME: the Sleigh, and everything else */
    g_sprite(&ROBIN, gr.rx, gr.ry, 2, gr.rflip);
}

static void gr_tick(uint32_t now, uint32_t dt)  /* the snow falls; the lights fade */
{
    uint32_t k, storm = now < gr.storm_until;
    for (k = 0; k < GR_NFLAKE; k++) {
        gr.fy_q8[k] += (int32_t)(gr.fspd[k] * (storm ? 3u : 1u) * dt / 16u);
        gr.fx_q8[k] += (int32_t)((((now / 700u + k) & 3u) == 0u ? 1 : -1) * (int32_t)(dt * (storm ? 24u : 6u)));
        if ((gr.fy_q8[k] >> 8) > 236) { gr.fy_q8[k] = 0; gr.fx_q8[k] = (int32_t)(tn_rand(&gr.rng) % 240u) << 8; }
        if (gr.fx_q8[k] < 0) gr.fx_q8[k] += 240 << 8;
        if ((gr.fx_q8[k] >> 8) >= 240) gr.fx_q8[k] -= 240 << 8;
    }
}

static void gr_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < TN_NKEYS; k++) {
        uint32_t l = 3u, t = now - gr.fx[k];
        if (gr.fx[k] && t < 700u) l = 64u - t * 56u / 700u;
        if (now < gr.sparkle_until && (now / 90u + k * 5u) % 4u == 0u) l = 64u;
        if (gr.chase_from && now - gr.chase_from < 2000u && (now - gr.chase_from) / 60u % TN_NKEYS == k) l = 64u;
        if (now - gr.last_input > 60000u && (now / 400u + k * 7u) % 9u == 0u) l = 16u;   /* idle twinkle */
        lv[TN_KEY_LED(k)] = (uint8_t)l;
    }
    lv[TB_PLAY] = (uint8_t)tn_pulse(now + 500u, 6, 30);   /* PLAY breathes too, the other way */
    lv[TB_HOME] = (uint8_t)tn_pulse(now, 6, 48);   /* HOME breathes: there is more */
}
#endif
