/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Snow Globe (DESIGN section 7.5): turn any knob to shake it; the snow swirls, tiny bells follow the snow; stop,
 * and it settles into a chord. Keys choose what is inside (a church, a tree, a snowman, a sleigh, a house, a
 * reindeer), each with its own sound. Nothing here can go wrong. */
#ifndef TN_GLOBE_H
#define TN_GLOBE_H

#define GB_N 90u
#define GB_CX 120
#define GB_CY 104
#define GB_R 86
static struct {
    int32_t x[GB_N], y[GB_N], vx[GB_N], vy[GB_N];   /* Q8, relative to the globe's middle */
    uint8_t inside, settled;
    uint32_t energy, t_spark, t_change, rng;
    int32_t swirl;
} gb;

static void gb_enter(void)
{
    uint32_t i;
    gb.rng = 0x5EED5u;
    for (i = 0; i < GB_N; i++) {
        gb.x[i] = ((int32_t)(tn_rand(&gb.rng) % 140u) - 70) << 8;
        gb.y[i] = (60 + (int32_t)(tn_rand(&gb.rng) % 16u)) << 8;   /* settled at the bottom */
        gb.vx[i] = gb.vy[i] = 0;
    }
    gb.energy = 0;
    gb.settled = 1;
}

static void gb_input(const tn_input_t *in)
{
    uint32_t k, i;
    int32_t turn = tn_enc_sum(in);
    if (turn) {                                  /* a shake: every flake kicked, a swirl the way the knob went */
        int32_t a = turn < 0 ? -turn : turn;
        gb.swirl += turn * 40;
        for (i = 0; i < GB_N; i++) {
            gb.vx[i] += ((int32_t)(tn_rand(&gb.rng) % 200u) - 100) * a * 40;   /* (Q8 pixels a second) */
            gb.vy[i] -= (int32_t)(tn_rand(&gb.rng) % 160u) * a * 50;
        }
        gb.energy += (uint32_t)a * 30u;
        if (gb.energy > 2000u) gb.energy = 2000u;
        gb.settled = 0;
        snd_shake((uint32_t)(a > 4 ? 100 : 40 + a * 15));
    }
    for (k = 0; k < 27u; k++)
        if (in->key_down >> k & 1u) {
            static const uint8_t INSIDE_NOTE[6][2] = {{60, TI_CHURCH}, {84, TI_CELESTA}, {72, TI_TOY}, {76, TI_HANDBELL}, {79, TI_MUSICBOX}, {67, TI_GLOCK}};
            gb.inside = (uint8_t)(k * 6u / 27u);
            gb.t_change = in->now;
            snd_ping(INSIDE_NOTE[gb.inside][1], INSIDE_NOTE[gb.inside][0], 100, 0);
            if (gb.inside == 3u) snd_shake(90);
        }
    if (in->btn_down & TN_GO_BUTTONS) {          /* any other button: a big shake */
        for (i = 0; i < GB_N; i++) { gb.vx[i] += ((int32_t)(tn_rand(&gb.rng) % 200u) - 100) * 300; gb.vy[i] -= (int32_t)(tn_rand(&gb.rng) % 160u) * 380; }
        gb.energy = 1500;
        gb.settled = 0;
        snd_shake(127);
    }
}

static void gb_tick(uint32_t now, uint32_t dt)
{
    uint32_t i, moving = 0;
    static const uint8_t PENT[] = {84, 86, 88, 91, 93, 96, 98, 100};
    for (i = 0; i < GB_N; i++) {
        int32_t r2, x, y;
        gb.vy[i] += (int32_t)dt * 14;           /* it falls, slowly (water) */
        gb.vx[i] += (-gb.y[i] >> 8) * gb.swirl / 40;    /* the swirl */
        gb.vx[i] -= gb.vx[i] * (int32_t)dt / 700;   /* the water holds it back */
        gb.vy[i] -= gb.vy[i] * (int32_t)dt / 700;
        gb.x[i] += gb.vx[i] * (int32_t)dt / 1000;
        gb.y[i] += gb.vy[i] * (int32_t)dt / 1000;
        x = gb.x[i] >> 8; y = gb.y[i] >> 8;
        r2 = x * x + y * y;
        if (y > 72) { gb.y[i] = 72 << 8; gb.vy[i] = 0; gb.vx[i] /= 2; }   /* the ground inside the globe */
        if (r2 > (GB_R - 6) * (GB_R - 6)) { gb.x[i] = gb.x[i] * 15 / 16; gb.y[i] = gb.y[i] * 15 / 16; gb.vx[i] = -gb.vx[i] / 2; gb.vy[i] = -gb.vy[i] / 2; }
        if (gb.vx[i] > 1500 || gb.vx[i] < -1500 || gb.vy[i] < -1500 || (gb.vy[i] > 1500 && y < 70)) moving++;
    }
    gb.swirl -= gb.swirl * (int32_t)dt / 900;
    if (moving > 6u && now - gb.t_spark > 90u + 600u / (moving + 1u)) {   /* tiny bells following the snow */
        gb.t_spark = now;
        snd_ping(TI_CHIME, PENT[tn_rand(&gb.rng) % sizeof PENT], 20 + (moving > 40u ? 40u : moving), (int32_t)(tn_rand(&gb.rng) % 100u) - 50);
    }
    if (!gb.settled && moving < 3u) {            /* settled: a chord */
        gb.settled = 1;
        snd_ping(TI_PAD, 60, 70, -30); snd_ping(TI_PAD, 64, 70, 0); snd_ping(TI_PAD, 67, 70, 30); snd_ping(TI_CELESTA, 84, 60, 0);
    }
}

static void gb_draw(uint32_t now)
{
    uint32_t i;
    int32_t y, wob = now - gb.t_change < 300u ? (int32_t)((now / 50u) % 3u) - 1 : 0;
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    g_disc(GB_CX, GB_CY, GB_R + 3, P_SNOW_SH);   /* the glass */
    g_disc(GB_CX, GB_CY, GB_R, P_NIGHT3);
    g_disc(GB_CX, GB_CY + 60, 62, P_SNOW);       /* the snowy ground inside */
    g_rect(GB_CX - GB_R, GB_CY + 76, 2 * GB_R, GB_R, P_NIGHT);   /* (cut off below the glass's floor) */
    g_disc(GB_CX - 50, GB_CY - 50, 6, P_WHITE);  /* a shine on the glass */
    switch (gb.inside) {                         /* what is inside */
    case 0:                                      /* a church */
        g_rect(GB_CX - 22 + wob, GB_CY + 20, 44, 40, P_GREY);
        g_tri_up(GB_CX + wob, GB_CY - 4, GB_CX - 28 + wob, GB_CX + 28 + wob, GB_CY + 20, P_ROOF);
        g_rect(GB_CX - 6 + wob, GB_CY - 34, 12, 32, P_GREY);
        g_tri_up(GB_CX + wob, GB_CY - 52, GB_CX - 8 + wob, GB_CX + 8 + wob, GB_CY - 34, P_ROOF);
        g_rect(GB_CX - 5 + wob, GB_CY + 40, 10, 20, P_BROWN_D);
        g_rect(GB_CX - 16 + wob, GB_CY + 28, 6, 8, P_WINDOW);
        g_rect(GB_CX + 10 + wob, GB_CY + 28, 6, 8, P_WINDOW);
        break;
    case 1: pic_draw(PIC_TREE, GB_CX + wob, GB_CY + 20, 40, now); break;
    case 2:                                      /* a snowman */
        g_disc(GB_CX + wob, GB_CY + 44, 20, P_SNOW);
        g_disc(GB_CX + wob, GB_CY + 12, 14, P_SNOW);
        g_rect(GB_CX - 10 + wob, GB_CY - 10, 20, 4, P_BLACK);
        g_rect(GB_CX - 6 + wob, GB_CY - 24, 12, 15, P_BLACK);
        g_rect(GB_CX + wob, GB_CY + 12, 8, 3, P_CARROT);
        g_pset(GB_CX - 5 + wob, GB_CY + 8, P_BLACK); g_pset(GB_CX + 5 + wob, GB_CY + 8, P_BLACK);
        g_rect(GB_CX - 14 + wob, GB_CY + 24, 28, 4, P_RED);
        break;
    case 3:                                      /* a sleigh */
        g_rect(GB_CX - 30 + wob, GB_CY + 30, 50, 18, P_RED);
        g_rect(GB_CX + 12 + wob, GB_CY + 16, 10, 20, P_RED);
        g_rect(GB_CX - 36 + wob, GB_CY + 50, 64, 3, P_GOLD);
        g_rect(GB_CX - 22 + wob, GB_CY + 18, 14, 12, P_GREEN);
        g_rect(GB_CX - 6 + wob, GB_CY + 22, 12, 8, P_BLUE);
        break;
    case 4:                                      /* a house */
        pic_draw(PIC_HOUSE, GB_CX + wob, GB_CY + 34, 30, now);
        break;
    default: g_sprite(&DEER, GB_CX - 32 + wob, GB_CY + 18, 4, 0); break;
    }
    for (i = 0; i < GB_N; i++) {
        int32_t x = GB_CX + (gb.x[i] >> 8), yy = GB_CY + (gb.y[i] >> 8);
        g_rect(x, yy, 2, 2, (gb.y[i] >> 8) >= 70 ? P_SNOW_SH : P_SNOW);   /* (settled: a little shadow, to show on the snow) */
    }
    g_rect(GB_CX - 70, GB_CY + GB_R - 6, 140, 26, P_RED_D);   /* the stand */
    g_rect(GB_CX - 76, GB_CY + GB_R + 18, 152, 8, P_RED);
    g_rect(GB_CX - 70, GB_CY + GB_R + 2, 140, 2, P_GOLD);
}

static void gb_leds(uint8_t *lv, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) lv[TN_KEY_LED(k)] = k * 6u / 27u == gb.inside ? 12u : (!gb.settled && (now / 70u + k * 3u) % 5u == 0u ? 40u : 3u);
}
#endif
