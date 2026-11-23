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
#ifndef TN_NO_LED_LEVELS
#define FM1_LED_LEVELS                          /* per-LED brightness (hal/fm1_input.h); TN_NO_LED_LEVELS: Dinghy's lit and glow only */
#endif
#ifndef TN_LCD_BAUD
#define TN_LCD_BAUD 4u                          /* 12 MHz, Dinghy's: the safe one until a unit has shown a faster one works */
#endif
#define LCD_BAUD TN_LCD_BAUD
#ifndef TN_NO_SAVE
#define TN_NO_SAVE 0
#endif
#include "platform.c"
#ifdef DINGHY_HOST
#define TN_POOL
#endif
#if defined(TN_NO_LED_LEVELS) && !defined(DINGHY_HOST)
static void fm1_led_level(uint32_t id, uint32_t lvl)   /* (the safe build: two levels, as Dinghy has them) */
{
    fm1_led_key(id, lvl >= 24u);
    fm1_led_key_glow(id, lvl && lvl < 24u);
}
#endif
#include "tn_ui.h"
#include "tn_set.h"
#include "tn_snd.h"
#include "tn_gfx.h"
#include "tn_pal.h"
#include "tn_pics.h"
#include "tn_carol.h"
#include "tn_catch.h"
#include "tn_advent.h"
#include "tn_ring.h"
#include "tn_box.h"
#include "tn_games.h"

/* ---- what is kept (tn_set.h) ----------------------------------------------------------------------------- */
static tn_settings_t set;
static uint32_t set_dirty, set_changed_at;
static void set_touch(void) { set_dirty = 1; set_changed_at = fm1_ms; }

/* ---- the screens ----------------------------------------------------------------------------------------- */
enum { SC_BOOT, SC_GROTTO, SC_SLEIGH, SC_CAROL, SC_CATCH, SC_ADVENT, SC_PANEL, SC_MUSICBOX, SC_BELLS, SC_JINGLE, SC_GLOBE,
       SC_CRACKER, SC_RALLY, SC_RIDE, SC_COUNT };
static uint32_t scene = SC_BOOT, scene_t0;
static uint8_t led_lv[TN_NLED], led_shown[TN_NLED];
static gesture_t gesture;
static void scene_go(uint32_t s);

#include "tn_grotto.h"
#include "tn_sleigh.h"
#include "tn_carolbook.h"
#include "tn_catchgame.h"
#include "tn_adventscr.h"
#include "tn_musicbox.h"
#include "tn_bells.h"
#include "tn_globe.h"
#include "tn_play.h"
#include "tn_panel.h"

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
    case SC_SLEIGH: sl_enter(); break;
    case SC_CAROL: cb_enter(); break;
    case SC_CATCH: cg_enter(); break;
    case SC_ADVENT: av_enter(); break;
    case SC_PANEL: pn_enter(); break;
    case SC_MUSICBOX: mb_enter(); break;
    case SC_BELLS: bt_enter(); break;
    case SC_JINGLE: ji_enter(); break;
    case SC_GLOBE: gb_enter(); break;
    case SC_CRACKER: ck_enter(); break;
    case SC_RALLY: rl_enter(); break;
    case SC_RIDE: rd_enter(); break;
    default: break;
    }
}

/* the help card: EDIT held alone for a moment shows what this screen's controls do (for grown-ups to read out) */
static const char *const HELP[SC_COUNT][7] = {
    [SC_GROTTO] = {"KEYS: BELLS AND BAUBLES", "KNOBS: FLY THE ROBIN", "PLAY: A SNOWSTORM", "OCT: BELLS UP AND DOWN", "OTHER BUTTONS: SURPRISES", "HOME: THE SLEIGH (MENU)", 0},
    [SC_SLEIGH] = {"KNOBS: CHOOSE", "PLAY OR ANY KEY: GO IN", "HOME: THE GROTTO", 0},
    [SC_CAROL] = {"KNOBS: CHOOSE A CAROL", "OCT: MAGIC FOLLOW LISTEN", "PLAY OR A KEY: START", "MAGIC: ANY KEY PLAYS ON", "FOLLOW: PRESS THE LIT KEY", "KNOB WHILE PLAYING: SOUND", 0},
    [SC_CATCH] = {"PRESENTS FALL TO CHIMNEYS", "PRESS THE LIT KEYS", "TO CATCH THEM", "LEVEL: IN THE PANEL", 0},
    [SC_ADVENT] = {"A KEY A DOOR: 1 TO 24", "KNOBS + PLAY: THE SAME", "THE DATE: WEB/ADVENT.HTML", 0},
    [SC_MUSICBOX] = {"KNOBS: TURN THE CRANK", "PLAY: WIND IT UP", "OCT: ANOTHER BOX", "KEYS: NOTES (YOUR TUNE:", "  ALSO PINS)", "REC: EMPTY YOUR TUNE", "PANEL: KEEP YOUR TUNE"},
    [SC_BELLS] = {"KEYS: RING THE BELLS", "KNOBS: CHOOSE A METHOD", "PLAY: RING / STAND", "ROUNDS: KEYS SWAP BELLS", "OCT: YOUR BELL (BLUE)", "ARP: BOB   SEQ: SINGLE", 0},
    [SC_JINGLE] = {"RING IT: PRESS A LIT KEY", "TWIST IT: TURN A KNOB", "JINGLE IT: PRESS PLAY", "SHH: DO NOTHING", 0},
    [SC_GLOBE] = {"KNOBS: SHAKE IT", "KEYS: WHAT IS INSIDE", "BUTTONS: A BIG SHAKE", 0},
    [SC_CRACKER] = {"LOW KEYS PULL LEFT", "HIGH KEYS PULL RIGHT", "ON THE BEAT PULLS HARDER", "OCT ON THE PAGE: PLAYERS", 0},
    [SC_RALLY] = {"LEFT: KNOB1 KNOB2 SELECT", "RIGHT: KNOB3 KNOB4 PRESETS", "NOBODY AT AN END: AN ELF", "PLAY: STOP", 0},
    [SC_RIDE] = {"KNOBS: STEER", "PRESS THE LIT KEYS", "AS THE BELLS PASS", "PLAY: STOP", 0},
};
static uint32_t help_since, help_on;
static void help_draw(uint32_t now)
{
    uint32_t i, n = 0;
    int32_t y;
    if (fm1_in.buttons >> TB_SAVE & 1u) {        /* the gesture on its way: say so */
        uint32_t t = gesture.held ? now - gesture.since : 0u, w = t > TN_GESTURE_MS ? 200u : t * 200u / TN_GESTURE_MS;
        g_rect(10, 90, 220, 60, P_NIGHT);
        g_rect(10, 90, 220, 2, P_GOLD);
        g_text_c(120, 98, &FONT_S, "KEEP HOLDING: GROWN-UPS", P_GOLD, P_GOLD_AA);
        g_rect(20, 126, 200, 8, P_NIGHT3);
        g_rect(20, 126, (int32_t)w, 8, P_GOLD);
        return;
    }
    while (n < 7u && HELP[scene][n]) n++;
    if (!n) return;
    y = 120 - (int32_t)n * 10 - 10;
    g_rect(6, y - 6, 228, (int32_t)n * 20 + 16, P_NIGHT);
    g_rect(6, y - 6, 228, 2, P_GOLD);
    for (i = 0; i < n; i++) g_text(14, y + 2 + (int32_t)i * 20, &FONT_S, HELP[scene][i], P_SNOW, P_TEXT_AA);
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
    if (in.btn >> TB_EDIT & 1u) { if (!help_since) help_since = in.now | 1u; help_on = in.now - help_since > 700u && scene != SC_PANEL; }
    else help_since = help_on = 0;
    if (gesture_step(&gesture, in.btn, in.now)) { if (scene != SC_PANEL) scene_go(SC_PANEL); return; }
    if (in.btn_down >> TB_HOME & 1u) {           /* HOME: back to the Grotto, from anywhere; in the Grotto, the Sleigh */
        if (scene == SC_PANEL) pn_leave();
        scene_go(scene == SC_GROTTO ? SC_SLEIGH : SC_GROTTO);
        return;
    }
    switch (scene) {
    case SC_BOOT:
        if (in.key_down || (in.btn_down & ~(1u << TB_EDIT | 1u << TB_SAVE)) || in.now - scene_t0 > 2800u) scene_go(SC_GROTTO);
        break;
    case SC_GROTTO: gr_input(&in); break;
    case SC_SLEIGH: sl_input(&in); break;
    case SC_CAROL: cb_input(&in); break;
    case SC_CATCH: cg_input(&in); break;
    case SC_ADVENT: av_input(&in); break;
    case SC_PANEL: pn_input(&in); break;
    case SC_MUSICBOX: mb_input(&in); break;
    case SC_BELLS: bt_input(&in); break;
    case SC_JINGLE: ji_input(&in); break;
    case SC_GLOBE: gb_input(&in); break;
    case SC_CRACKER: ck_input(&in); break;
    case SC_RALLY: rl_input(&in); break;
    case SC_RIDE: rd_input(&in); break;
    default: break;
    }
}

static void draw_frame(uint32_t now, uint32_t dt)
{
    memset(led_lv, 0, sizeof led_lv);
    switch (scene) {
    case SC_BOOT: boot_draw(now); break;
    case SC_GROTTO: gr_tick(now, dt); gr_draw(now); gr_leds(led_lv, now); break;
    case SC_SLEIGH: sl_draw(now); sl_leds(led_lv, now); break;
    case SC_CAROL: cb_draw(now); cb_leds(led_lv, now); break;
    case SC_CATCH: cg_tick(now, dt); cg_draw(now); cg_leds(led_lv, now); break;
    case SC_ADVENT: av_draw(now); av_leds(led_lv, now); break;
    case SC_PANEL: pn_draw(now); pn_leds(led_lv, now); return;   /* (it flushes itself: the screen test times it) */
    case SC_MUSICBOX: mb_tick(now, dt); mb_draw(now); mb_leds(led_lv, now); break;
    case SC_BELLS: bt_tick(now); bt_draw(now); bt_leds(led_lv, now); break;
    case SC_JINGLE: ji_tick(now); ji_draw(now); ji_leds(led_lv, now); break;
    case SC_GLOBE: gb_tick(now, dt); gb_draw(now); gb_leds(led_lv, now); break;
    case SC_CRACKER: ck_tick(now); ck_draw(now); ck_leds(led_lv, now); break;
    case SC_RALLY: rl_tick(now, dt); rl_draw(now); rl_leds(led_lv, now); break;
    case SC_RIDE: rd_tick(now, dt); rd_draw(now); rd_leds(led_lv, now); break;
    default: g_clear(P_NIGHT); break;
    }
    if (help_on) help_draw(now);
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
static advent_rx_t adv_rx;
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
    if (settings_load(&set, sizeof set) || !tn_set_valid(&set)) tn_set_defaults(&set);
    snd_set_ceiling(TN_CEILINGS[set.ceiling]);
    pal_load(0);
    gr.oct = (int8_t)set.oct;
    if (advent_boot(&set)) set_touch();         /* (a calendar never told the date counts the days it is switched on) */
    scene_go(SC_BOOT);
    snd_shake(127); snd_shake_at(350, 90); snd_shake_at(700, 127);
    snd_ping_at(150, TI_CELESTA, 84, 90, -30); snd_ping_at(400, TI_CELESTA, 88, 90, 0); snd_ping_at(650, TI_CELESTA, 91, 100, 30);
    for (;;) {
        uint32_t now;
        uint8_t st, d1, d2;
        platform_poll();
        while (midi_get(&st, &d1, &d2))         /* the Advent date, from web/advent.html */
            if (advent_midi(&set, &adv_rx, st, d1, d2)) { set_touch(); snd_fx(SFX_TWINKLE); }
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
        if (set_dirty && !snd_active() && !snd_peak && fm1_ms - set_changed_at > 2000u && !TN_NO_SAVE) {   /* saved when silent (an erase stalls the sound) */
            settings_save(&set, sizeof set);
            set_dirty = 0;
        }
    }
}
