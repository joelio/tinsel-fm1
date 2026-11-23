/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The grown-up panel (DESIGN section 10): reached only by holding EDIT and SAVE together for 2 seconds. A plain list:
 * SELECT moves, ALGORITHM changes (or, for something that cannot be undone, asks: then PLAY does it), HOME leaves and
 * keeps what changed. The screen test lets a unit show which screen speeds it takes. */
#ifndef TN_PANEL_H
#define TN_PANEL_H

enum { PN_VOLUME, PN_CATCH, PN_ALL, PN_OPEN_ALL, PN_CLOSE_ALL, PN_DATE, PN_SCORES, PN_SCREEN, PN_ABOUT, PN_COUNT };
static const char *const PN_NAME[PN_COUNT] = {"VOLUME", "PRESENT CATCH", "ALL GAMES", "ADVENT: OPEN ALL", "ADVENT: CLOSE ALL",
                                              "DATE", "HIGH SCORES", "SCREEN TEST", "ABOUT"};
static const char *const PN_LEVEL[3] = {"LITTLE ELF", "ELF", "SANTA"};
static const char *const PN_VOL[3] = {"GENTLE", "MEDIUM", "FULL"};
static const char *const PN_MON[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const uint8_t PN_BAUDS[5] = {4, 3, 2, 1, 0};   /* SPI1: 60 MHz / (b + 1): 12, 15, 20, 30, 60 MHz */
static struct {
    uint8_t at, asking, baud_i, changed;
    uint32_t t_asked, t_frame, frames, flush_ms;
} pn;

static void pn_set_baud(uint32_t b)
{
#ifndef DINGHY_HOST
    lcd_sync();
    fm1_lcd_baud(b);
#else
    (void)b;
#endif
}

static void pn_enter(void) { pn.at = 0; pn.asking = 0; pn.changed = 0; pn.baud_i = 0; snd_ping(TI_CELESTA, 72, 70, 0); }
static void pn_leave(void)
{
    pn_set_baud(TN_LCD_BAUD);                   /* (the test's speed is never kept) */
    if (pn.changed) set_touch();
}

static void pn_date_step(int32_t dir)          /* the date by hand: a day at a time (from 1 December if none) */
{
    static const uint8_t DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t y = set.date_y ? set.date_y : 2026u, m = set.date_y ? set.date_m : 12u, d = set.date_y ? set.date_d : 0u;
    if (dir > 0) { d++; if (d > DAYS[m - 1u]) { d = 1; if (++m > 12u) { m = 1; y++; } } }
    else if (set.date_y) { if (d > 1u) d--; else { if (m > 1u) m--; else { m = 12; y--; } d = DAYS[m - 1u]; } }
    else return;
    set.date_y = (uint16_t)y; set.date_m = (uint8_t)m; set.date_d = (uint8_t)d;   /* (by hand it may go back) */
    pn.changed = 1;
}

static void pn_input(const tn_input_t *in)
{
    int32_t mv = in->enc[TE_SELECT] + in->enc[TE_KNOB1], ch = in->enc[TE_ALGORITHM] + in->enc[TE_KNOB2];
    uint32_t i;
    if (mv) {
        pn.at = (uint8_t)((pn.at + PN_COUNT + (uint32_t)(mv > 0 ? 1 : (int32_t)PN_COUNT - 1)) % PN_COUNT);
        pn.asking = 0;
        if (pn.at != PN_SCREEN) pn_set_baud(TN_LCD_BAUD);
        snd_ping(TI_CHIME, 84, 40, 0);
    }
    if (ch) {
        int32_t s = ch > 0 ? 1 : -1;
        switch (pn.at) {
        case PN_VOLUME: set.ceiling = (uint8_t)((set.ceiling + 3u + (uint32_t)s) % 3u); snd_set_ceiling(TN_CEILINGS[set.ceiling]); pn.changed = 1; snd_fx(SFX_CHEER); break;
        case PN_CATCH: set.level[0] = (uint8_t)((set.level[0] + 3u + (uint32_t)s) % 3u); pn.changed = 1; break;
        case PN_ALL: { uint8_t l = (uint8_t)((set.level[0] + 3u + (uint32_t)s) % 3u); for (i = 0; i < 8u; i++) set.level[i] = l; pn.changed = 1; break; }
        case PN_OPEN_ALL: case PN_CLOSE_ALL: case PN_SCORES: pn.asking = 1; pn.t_asked = in->now; break;
        case PN_DATE: pn_date_step(s); break;
        case PN_SCREEN: pn.baud_i = (uint8_t)((pn.baud_i + 5u + (uint32_t)s) % 5u); pn_set_baud(PN_BAUDS[pn.baud_i]); pn.frames = 0; pn.flush_ms = 0; break;
        default: break;
        }
        snd_ping(TI_GLOCK, 76, 60, 0);
    }
    if (pn.asking && (in->btn_down >> TB_PLAY & 1u)) {
        if (pn.at == PN_OPEN_ALL) { set.adv_all = 1; set.doors = ADV_ALL; }
        if (pn.at == PN_CLOSE_ALL) { set.doors = 0; set.adv_all = 0; set.power_days = 0; }
        if (pn.at == PN_SCORES) memset(set.best, 0, sizeof set.best);
        pn.asking = 0;
        pn.changed = 1;
        snd_fx(SFX_DOOR);
    }
    if (pn.asking && in->now - pn.t_asked > 6000u) pn.asking = 0;
}

static void pn_copy(char *d, const char *s) { while ((*d++ = *s++) != 0) {} }
static void pn_value(uint32_t i, char *b)
{
    char *p = b;
    b[0] = 0;
    switch (i) {
    case PN_VOLUME: pn_copy(b, PN_VOL[set.ceiling]); break;
    case PN_CATCH: pn_copy(b, PN_LEVEL[set.level[0] % 3u]); break;
    case PN_ALL: memcpy(b, "TURN", 5); break;
    case PN_OPEN_ALL: case PN_CLOSE_ALL: case PN_SCORES: memcpy(b, pn.asking && pn.at == i ? "PLAY: YES" : "TURN", pn.asking && pn.at == i ? 10 : 5); break;
    case PN_DATE:
        if (!set.date_y) { memcpy(b, "NOT TOLD", 9); break; }
        p = tn_utoa(p, set.date_d); *p++ = ' '; memcpy(p, PN_MON[set.date_m - 1u], 3); p += 3; *p++ = ' '; tn_utoa(p, set.date_y);
        break;
    case PN_SCREEN: {
        static const uint8_t MHZ[5] = {12, 15, 20, 30, 60};
        p = tn_utoa(p, MHZ[pn.baud_i]); memcpy(p, " MHZ", 5);
        break;
    }
    case PN_ABOUT: memcpy(b, "TINSEL " APP_VER, sizeof "TINSEL " APP_VER); break;
    }
}

static void pn_draw(uint32_t now)
{
    uint32_t i;
    char b[24];
    if (pn.at == PN_SCREEN) {                    /* the screen test: everything moves, every frame */
        uint32_t t0 = fm1_ms, x;
        int32_t y;
        for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_BAUBLE_LIT + ((uint32_t)y / 8u + now / 40u) % 8u);
        for (x = 0; x < 8u; x++) g_disc((int32_t)((now / 8u + x * 30u) % 240u), 60 + (int32_t)x * 16, 8, P_WHITE);
        g_rect(20, 90, 200, 60, P_NIGHT);
        pn_value(PN_SCREEN, b);
        g_text_c(120, 96, &FONT_L, b, P_GOLD, P_GOLD_AA);
        {
            char c[32], *p = c;
            uint32_t fps = pn.flush_ms ? 1000u * pn.frames / pn.flush_ms : 0;
            p = tn_utoa(p, pn.frames ? pn.flush_ms / pn.frames : 0); memcpy(p, " MS  ", 5); p += 5;
            p = tn_utoa(p, fps > 99u ? 99u : fps); memcpy(p, " FPS", 5);
            g_text_c(120, 128, &FONT_S, c, P_SNOW, P_TEXT_AA);
        }
        g_all = 1;
        g_flush();
        pn.flush_ms += fm1_ms - t0;
        if (++pn.frames > 60u) { pn.frames = 1; pn.flush_ms = fm1_ms - t0; }
        return;
    }
    g_clear(P_NIGHT);
    g_text(12, 8, &FONT_S, "FOR GROWN-UPS", P_GOLD, P_GOLD_AA);
    g_rect(12, 26, 216, 1, P_DIM);
    for (i = 0; i < PN_COUNT; i++) {
        int32_t y = 34 + (int32_t)i * 20;
        if (i == pn.at) g_rect(6, y - 2, 228, 20, P_NIGHT3);
        g_text(12, y, &FONT_S, PN_NAME[i], i == pn.at ? P_SNOW : P_GREY, i == pn.at ? P_TEXT_AA : 0xFFu);
        pn_value(i, b);
        g_text(228 - text_w(&FONT_S, b), y, &FONT_S, b, i == pn.at ? P_GOLD : P_GREY, 0xFFu);
    }
    g_text(12, 220, &FONT_S, "SELECT  ALGORITHM  HOME", P_DIM, 0xFFu);
    g_flush();
}

static void pn_leds(uint8_t *lv, uint32_t now)
{
    lv[TB_HOME] = 32;
    lv[TB_PLAY] = pn.asking ? (uint8_t)((now / 200u) & 1u ? 64u : 8u) : 0u;
}
#endif
