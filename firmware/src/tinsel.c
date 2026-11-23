/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel for the M-VAVE FM-1: Christmas music toys and games, all in one, for every age.
 * Built on Dinghy (platform.c and everything below it: Felucca / SLOOP's platform). The design: docs/DESIGN.md.
 *
 *   HOME        back to the Grotto, from anywhere (in the Grotto: the Sleigh, the menu)
 *   EDIT + SAVE held 2 s   the grown-up panel (the only way to change a setting or delete anything)
 *   EDIT held   a help card for this screen
 *   the rest    belongs to the screen you are on: tn_grotto.h, tn_sleigh.h, tn_carolbook.h, tn_catch.h, tn_advent.h
 *
 * A brightly lit key means "press me"; a glowing key means "next". Integers only (docs/RULES.md). */
static void app_idle(void);
#define LCD_IDLE app_idle                       /* while a band goes out by DMA: the keys still ring */
#define APP_NAME "TINSEL"
#define FM1_LED_LEVELS                          /* per-LED brightness (hal/fm1_input.h) */
#ifndef TN_LCD_BAUD
#define TN_LCD_BAUD 4u                          /* 12 MHz, Dinghy's: the safe one until a unit has shown a faster one works */
#endif
#define LCD_BAUD TN_LCD_BAUD
#include "platform.c"
#ifdef DINGHY_HOST
#define TN_POOL
#endif
#include "tn_ui.h"
#include "tn_snd.h"
#include "tn_gfx.h"
#include "tn_pal.h"

/* ---- what is kept (DESIGN section 14) -------------------------------------------------------------------- */
#define TN_SET_VER 1u
typedef struct {
    uint8_t ver, ceiling, sound, oct;           /* ceiling: 0 gentle, 1 medium, 2 full */
    uint8_t level[8];                           /* each game's level: 0 Little Elf, 1 Elf, 2 Santa */
    uint32_t doors;                             /* Advent doors opened, bit d-1 */
    uint16_t date_y;                            /* the latest date Tinsel was told (0: never) */
    uint8_t date_m, date_d;
    uint8_t power_days, pad[3];                 /* days switched on, for a calendar that was never told the date */
    uint16_t best[8];                           /* high scores */
} tn_settings_t;
static tn_settings_t set;
static uint32_t set_dirty, set_changed_at;
static void set_defaults(void) { memset(&set, 0, sizeof set); set.ver = TN_SET_VER; }
static void set_touch(void) { set_dirty = 1; set_changed_at = fm1_ms; }
static const uint16_t CEILINGS[3] = {16384, 22937, 32767};   /* gentle 50 %, medium 70 %, full */

/* ---- the screens ----------------------------------------------------------------------------------------- */
enum { SC_BOOT, SC_GROTTO, SC_SLEIGH, SC_CAROL, SC_CATCH, SC_ADVENT, SC_PANEL, SC_COUNT };
static uint32_t scene = SC_BOOT, scene_t0;
static uint8_t led_lv[TN_NLED], led_shown[TN_NLED];
static gesture_t gesture;
static void scene_go(uint32_t s);

#include "tn_grotto.h"

/* the boot plate: TINSEL in gold on a red ribbon, snow, a sleigh-bell flourish (any key skips) */
static void boot_draw(uint32_t now)
{
    uint32_t t = now - scene_t0, k;
    int32_t y;
    for (y = 0; y < G_H; y++) g_rect(0, y, G_W, 1, P_SKY + (uint32_t)y / 30u);
    for (k = 0; k < 50u; k++) {
        int32_t x = (int32_t)((k * 97u + 11u) % 240u), yy = (int32_t)(((k * 61u + 5u) % 240u + t / (12u + k % 5u)) % 240u);
        g_rect(x, yy, 1 + (int32_t)(k & 1u), 1 + (int32_t)(k & 1u), P_SNOW);
    }
    g_rect(0, 96, G_W, 48, P_RED);               /* the ribbon */
    g_rect(0, 96, G_W, 3, P_RED_D);
    g_rect(0, 141, G_W, 3, P_RED_D);
    g_rect(0, 102, G_W, 2, P_GOLD_D);
    g_rect(0, 136, G_W, 2, P_GOLD_D);
    g_tri_up(120, 60, 92, 120, 96, P_RED);       /* the bow */
    g_tri_up(120, 60, 120, 148, 96, P_RED);
    g_disc(120, 94, 7, P_RED_D);
    {
        int32_t w = text_w(&FONT_L, "TINSEL"), x = 120 - w / 2, shine = (int32_t)(t / 6u % 400u) - 80;
        g_text(x + 2, 106, &FONT_L, "TINSEL", P_RED_D, 0xFFu);
        g_text(x, 104, &FONT_L, "TINSEL", P_GOLD, P_GOLD_AA);
        if (shine > x && shine < x + w) g_rect(shine, 106, 3, 28, P_YELLOW);   /* a glint runs across */
    }
    g_text_c(120, 214, &FONT_S, "MERRY CHRISTMAS", P_SNOW_SH, P_TEXT_AA);
}

static void scene_go(uint32_t s)
{
    scene = s % SC_COUNT;
    scene_t0 = fm1_ms;
    snd_hush();
    g_all = 1;
    switch (scene) {
    case SC_GROTTO: gr_enter(); break;
    default: break;
    }
}

/* ---- the panel, read once a pass ------------------------------------------------------------------------- */
static uint32_t prev_keys;
static void read_panel(tn_input_t *in)
{
    uint32_t rel, k, e;
    memset(in, 0, sizeof *in);
    in->now = fm1_ms;
    in->keys = fm1_in.notes;
    in->key_down = (fm1_input_note_edges() | (in->keys & ~prev_keys)) & 0x7FFFFFFu;
    in->key_up = prev_keys & ~in->keys;
    prev_keys = in->keys;
    in->btn = fm1_in.buttons;
    in->btn_down = fm1_input_edges(&rel);
    in->btn_up = rel;
    for (e = 0; e < 7u; e++) in->enc[e] = fm1_enc_take(e);
    tn_cursor(in);
    (void)k;
}

static void ui_pass(void)                       /* input: every pass of the main loop, and while a band goes out */
{
    tn_input_t in;
    read_panel(&in);
    if (gesture_step(&gesture, in.btn, in.now)) { scene_go(SC_GROTTO); return; }   /* (the panel: to come) */
    if (in.btn_down >> TB_HOME & 1u) {
        scene_go(scene == SC_GROTTO ? SC_GROTTO : SC_GROTTO);
        return;
    }
    switch (scene) {
    case SC_BOOT:
        if (in.key_down || (in.btn_down & ~(1u << TB_EDIT | 1u << TB_SAVE)) || in.now - scene_t0 > 2800u) scene_go(SC_GROTTO);
        break;
    case SC_GROTTO: gr_input(&in); break;
    default: break;
    }
}

static void draw_frame(uint32_t now, uint32_t dt)
{
    memset(led_lv, 0, sizeof led_lv);
    switch (scene) {
    case SC_BOOT: boot_draw(now); break;
    case SC_GROTTO: gr_tick(now, dt); gr_draw(now); gr_leds(led_lv, now); break;
    default: g_clear(P_NIGHT); break;
    }
    g_flush();
}

static void leds_out(void)
{
    uint32_t i;
    for (i = 0; i < TN_NLED; i++)
        if (led_lv[i] != led_shown[i]) { fm1_led_level(i, led_lv[i]); led_shown[i] = led_lv[i]; }
}

/* ---- the sound and the loop ------------------------------------------------------------------------------ */
#ifdef TN_BENCH                                 /* a test build: the interrupt's times, where a dump of the memory finds them */
static volatile uint32_t bench_load[5] = {0x44414F4Cu, 0, 0, 0, 0x4B52414Du};   /* "LOAD", the longest, the sum, how many, "MARK" */
#endif
static void app_audio(int32_t *out, uint32_t n)
{
#if defined(TN_BENCH) && !defined(DINGHY_HOST)
    uint32_t t0 = fm1_ticks(), d;
#endif
    snd_render(out, n);
#if defined(TN_BENCH) && !defined(DINGHY_HOST)
    d = fm1_ticks() - t0;
    if (d > bench_load[1]) bench_load[1] = d;
    bench_load[2] += d;
    bench_load[3]++;
#endif
}

static uint32_t idle_ok;
static void app_idle(void)
{
    if (!idle_ok) return;
    idle_ok = 0;                                /* (never inside itself) */
    ui_pass();
    idle_ok = 1;
}

static void app_main(void)
{
    uint32_t last_draw = 0;
    if (settings_load(&set, sizeof set) || set.ver != TN_SET_VER || set.ceiling > 2u) set_defaults();
    snd_set_ceiling(CEILINGS[set.ceiling]);
    pal_load(0);
    gr.oct = (int8_t)set.oct > 2 || (int8_t)set.oct < -1 ? 0 : (int8_t)set.oct;
    scene_go(SC_BOOT);
    snd_shake(127); snd_shake_at(350, 90); snd_shake_at(700, 127);
    snd_ping_at(150, TI_CELESTA, 84, 90, -30); snd_ping_at(400, TI_CELESTA, 88, 90, 0); snd_ping_at(650, TI_CELESTA, 91, 100, 30);
    for (;;) {
        uint32_t now;
        uint8_t st, d1, d2;
        platform_poll();
        while (midi_get(&st, &d1, &d2)) {}       /* (the Advent date arrives here: to come) */
        ui_pass();
        now = fm1_ms;
        if (now - last_draw >= 33u) {
            uint32_t dt = now - last_draw > 100u ? 100u : now - last_draw;
            last_draw = now;
            idle_ok = 1;
            draw_frame(now, dt);
            idle_ok = 0;
            leds_out();
        }
        if (set_dirty && !snd_active() && fm1_ms - set_changed_at > 2000u) {   /* saved when silent (an erase stalls the sound) */
            settings_save(&set, sizeof set);
            set_dirty = 0;
        }
    }
}
