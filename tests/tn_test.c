/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel's own checks, on the host: the parts with no platform in them (the store's window, the sound engine, the
 * carols and their player, the games' rules, the Advent calendar, the grown-up gesture).
 *   cc -O1 -Wall -Ifirmware/hal -Ifirmware/src -Ifirmware/gen -o build/host/tn_test tests/tn_test.c && build/host/tn_test */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fm1_cc.h"
#include "../firmware/src/store.h"
#define TN_POOL
#include "../firmware/src/tn_snd.h"
#include "../firmware/src/tn_ui.h"
#include "../firmware/src/tn_carol.h"
#include "../firmware/src/tn_catch.h"
#include "../firmware/src/tn_set.h"
#include "../firmware/src/tn_advent.h"
#include "../firmware/src/tn_ring.h"
#include "../firmware/src/tn_games.h"

static int bad, total;
static void check(const char *what, int ok)
{
    total++;
    if (!ok) { printf("FAIL  %s\n", what); bad++; }
}

/* ---- the store: Tinsel writes only its window (DESIGN section 14) ---- */
static void test_store(void)
{
    uint32_t s, leak = 0, inwin = 0;
    check("store: settings at 0xB7000", SETTINGS_AT == 0xB7000u);
    check("store: blobs at 0xB8000", BLOBS_AT == 0xB8000u);
    check("store: slots at 0xC0000, ending at 0xE0000", SLOTS_AT == 0xC0000u && SLOTS_AT + NSLOT * SLOT_SECT * 0x1000u == 0xE0000u);
    check("store: magic TNSL", STORE_MAGIC == 0x4C534E54u);
    for (s = 0; s < 0x100000u; s += 0x1000u) {
        int ok = FL_RANGE_OK(s, 0x1000u);
        int mine = (s >= 0xB7000u && s < 0xE0000u) || (s >= FL_OTA_LO && s < FL_OTA_HI);
        if (ok && !mine) leak++;
        if (ok && mine) inwin++;
    }
    check("store: no sector outside the window and the update staging may be written", leak == 0);
    check("store: every sector of the window may be written", inwin == 41u + (FL_OTA_HI - FL_OTA_LO) / 0x1000u);
    check("store: Melisma's slots (0x97000..0xB6FFF) refused", !FL_RANGE_OK(0x97000u, 0x1000u) && !FL_RANGE_OK(0xB6000u, 0x1000u));
    check("store: Dinghy's blobs and settings (0xF4000, 0xFC000) refused", !FL_RANGE_OK(0xF4000u, 0x1000u) && !FL_RANGE_OK(0xFC000u, 0x1000u));
    check("store: a range straddling into the window from below refused", !FL_RANGE_OK(0xB6FF0u, 0x20u));
    check("store: a range running past the window refused", !FL_RANGE_OK(0xDF000u, 0x2000u) || FL_IN(0xDF000u, 0x2000u, FL_OTA_LO, FL_OTA_HI));
    check("store: a wrapping range refused", !FL_RANGE_OK(0xB7000u, 0xFFFFF000u));
}

/* ---- the sound ---- */
static int32_t blk[512];
static int16_t *wav;
static uint32_t wav_n;
static uint32_t render_ms(uint32_t ms)          /* render ms of sound; the loudest sample */
{
    uint32_t b, i, pk = 0, nb = ms * 441u / 10u / 256u + 1u;
    for (b = 0; b < nb; b++) {
        memset(blk, 0, sizeof blk);
        snd_render(blk, 256);
        for (i = 0; i < 512u; i++) {
            uint32_t a = (uint32_t)(blk[i] < 0 ? -blk[i] : blk[i]);
            if (a > pk) pk = a;
        }
        if (wav) { memcpy(wav + wav_n * 2u, blk, 0); for (i = 0; i < 512u; i++) wav[wav_n * 2u + i] = (int16_t)blk[i]; wav_n += 256u; }
    }
    return pk;
}
static void snd_reset(void)
{
    memset(snd_v, 0, sizeof snd_v);
    memset(&snd_sb, 0, sizeof snd_sb);
    memset(rv_buf, 0, sizeof rv_buf);
    memset(snd_later, 0, sizeof snd_later);
    snd_qr = snd_qw = 0;
    snd_lim = 32768;
}
static void test_sound(void)
{
    uint32_t i, pk, c;
    char what[96];
    static const uint32_t CEIL[3] = {16384, 22937, 32767};
    for (i = 0; i < TN_NINST; i++) {             /* every instrument sounds, then stops */
        snd_reset();
        snd_ping(i, 69, 100, 0);
        pk = render_ms(300);
        snprintf(what, sizeof what, "sound: instrument %u sounds (peak %u)", i, pk);
        check(what, pk > 800u);
        render_ms(13000);
        snprintf(what, sizeof what, "sound: instrument %u has stopped after 13 s", i);
        check(what, snd_active() == 0);
    }
    {                                            /* pitch: a celesta A4, its zero crossings after the attack */
        uint32_t b, k, up = 0;
        int32_t last = 0;
        snd_reset();
        snd_set_reverb(0);
        snd_ping(TI_CELESTA, 69, 100, 0);
        render_ms(200);
        for (b = 0; b < 172u; b++) {             /* 1 s */
            memset(blk, 0, sizeof blk);
            snd_render(blk, 256);
            for (k = 0; k < 256u; k++) { if (last < 0 && blk[2u * k] >= 0) up++; last = blk[2u * k]; }
        }
        snprintf(what, sizeof what, "sound: a celesta A4 is 440 Hz within 1 %% (%u crossings a second; inc %u, voice on %u)", up, snd_v[0].inc[0], snd_v[0].on);
        check(what, up >= 436u && up <= 444u);
        snd_set_reverb(160);
    }
    for (c = 0; c < 3u; c++) {                   /* everything at once stays under each ceiling */
        snd_reset();
        snd_set_ceiling(CEIL[c]);
        for (i = 0; i < 27u; i++) snd_ping(i % TN_NINST, 53u + i, 127, (int32_t)i * 4 - 52);
        snd_shake(127);
        snd_fx(SFX_FANFARE);
        render_ms(30);
        pk = render_ms(3000);
        snprintf(what, sizeof what, "sound: 27 notes, a shake and a fanfare stay under the ceiling %u (peak %u)", CEIL[c], pk);
        check(what, pk <= CEIL[c]);
        check("sound: and they are loud enough to hear", pk > CEIL[c] / 3u);
    }
    snd_set_ceiling(16384);
    snd_reset();                                 /* hush */
    for (i = 0; i < 10u; i++) snd_ping(TI_CHURCH, 60u + i, 127, 0);
    snd_shake(127);
    render_ms(200);
    snd_hush();
    render_ms(60);
    rv_lp[0] = rv_lp[1] = rv_lp[2] = rv_lp[3] = 0;
    memset(rv_buf, 0, sizeof rv_buf);
    pk = render_ms(30);
    snprintf(what, sizeof what, "sound: hushed within 60 ms (peak after %u)", pk);
    check(what, pk < 300u);
    snd_reset();                                 /* stealing: forty notes, sixteen voices, the newest sound */
    for (i = 0; i < 40u; i++) { snd_ping(TI_HANDBELL, 60u + (i % 20u), 100, 0); render_ms(6); }
    check("sound: never more than 16 voices", snd_active() <= TN_VOICES);
    check("sound: the newest note is among them", snd_v[0].on || snd_v[1].on);
    {                                            /* sleigh bells alone ring and stop */
        snd_reset();
        snd_shake(127);
        pk = render_ms(150);
        snprintf(what, sizeof what, "sound: sleigh bells ring (peak %u)", pk);
        check(what, pk > 1000u);
        render_ms(2000);
        check("sound: sleigh bells settle", snd_sb.energy < 64);
    }
}

/* ---- the carols ---- */
static void test_carols(void)
{
    uint32_t c, i;
    char what[120];
    carol_play_t p;
    carol_ev_t ev;
    check("carols: at least eight", TN_NCAROLS >= 8u);
    for (c = 0; c < TN_NCAROLS; c++) {
        const tn_carol_t *k = &TN_CAROLS[c];
        uint32_t lo = 255, hi = 0, notes = 0, n;
        for (i = 0; i < k->n; i++) if (k->pitch[i]) { if (k->pitch[i] < lo) lo = k->pitch[i]; if (k->pitch[i] > hi) hi = k->pitch[i]; notes++; }
        snprintf(what, sizeof what, "carols: %s lies on the keys (MIDI %u..%u)", k->title, lo, hi);
        check(what, lo >= 53u && hi <= 79u);
        carol_start(&p, c, CM_MAGIC, 0);         /* Magic: any key, one note each, all of them, then done */
        for (n = 0; n < 1000u && !p.done; n++) {
            uint32_t before = p.i;
            check("carols: Magic moves on with any key", carol_key(&p, (n * 7u) % 27u, &ev) == 1 && ev.mel && p.i > before);
        }
        snprintf(what, sizeof what, "carols: %s in Magic takes one press a note (%u presses, %u notes)", k->title, n, notes);
        check(what, n == notes);
        check("carols: at the end, done; a key does nothing more", p.done && carol_key(&p, 3, &ev) == 0 && !ev.mel);
        carol_start(&p, c, CM_FOLLOW, 0);        /* Follow: a wrong key stays, the right one moves on */
        {
            int want = carol_next_key(&p, 0), wrong = (want + 5) % 27, ok = 1;
            uint32_t presses = 0;
            ok &= carol_key(&p, (uint32_t)wrong, &ev) == 0 && p.i == 0;
            while (!p.done && presses < 1000u) {
                int k2 = carol_next_key(&p, 0);
                ok &= k2 >= 0 && k2 < 27;
                ok &= carol_key(&p, (uint32_t)k2, &ev) == 1;
                presses++;
            }
            snprintf(what, sizeof what, "carols: %s in Follow: wrong keys stay, the lit key moves on", k->title);
            check(what, ok && presses == notes);
        }
        carol_start(&p, c, CM_LISTEN, 0);        /* Listen: by itself, in time */
        {
            uint32_t t, played = 0, ms = 0;
            for (t = 0; t < 600000u && !p.done; t += 5u) if (carol_tick(&p, t, &ev)) { played++; ms = t; }
            snprintf(what, sizeof what, "carols: %s plays itself through (%u notes in %u s)", k->title, played, ms / 1000u);
            check(what, p.done && played == notes && ms > 5000u && ms < 300000u);
            check("carols: Listen ignores keys", carol_key(&p, 0, &ev) == 0);
        }
    }
    carol_start(&p, 0, CM_FOLLOW, 0);
    check("carols: the next key and the one after are different notes of the tune", carol_next_key(&p, 0) >= 0 && carol_next_key(&p, 1) >= 0);
}

/* ---- Present Catch ---- */
static uint32_t catch_run(uint32_t level, uint32_t seed, int press, uint32_t ms, catch_t *g)   /* returns a fingerprint */
{
    uint32_t t, fp = 0;
    catch_start(g, level, seed);
    for (t = 0; t < ms && !g->done; t += 20u) {
        catch_step(g, 20);
        if (press) {
            int lane = catch_lit_lane(g);
            if (lane >= 0 && catch_key(g, CATCH_LANE_FIRST[lane] + 1u)) fp = fp * 31u + (uint32_t)lane + t;
        }
    }
    return fp ^ g->score ^ (uint32_t)g->misses << 16;
}
static void test_catch(void)
{
    catch_t g, h;
    uint32_t k, sizes[CATCH_LANES] = {0}, ok = 1, sp;
    for (k = 0; k < 27u; k++) sizes[catch_lane_of(k)]++;
    for (k = 0; k < CATCH_LANES; k++) ok &= sizes[k] == (k == 3u ? 3u : 4u);
    check("catch: the lanes take all 27 keys, 4 4 4 3 4 4 4", ok);
    catch_run(LV_LITTLE, 7, 0, 180000, &g);
    check("catch: Little Elf left alone for 3 minutes never ends, never counts a miss", !g.done && g.misses == 0 && g.saved > 10u);
    sp = (uint32_t)g.speed;
    catch_run(LV_LITTLE, 7, 1, 60000, &g);
    check("catch: Little Elf catches with the lit lane's keys", g.score > 20u && g.misses == 0);
    check("catch: Little Elf never speeds up", (uint32_t)g.speed == sp);
    catch_run(LV_ELF, 7, 0, 600000, &g);
    check("catch: Elf left alone ends after five misses", g.done && g.misses == 5u);
    catch_run(LV_SANTA, 9, 0, 600000, &g);
    check("catch: Santa left alone ends after three misses", g.done && g.misses == 3u);
    catch_start(&g, LV_SANTA, 3);
    for (k = 0; k < 40u; k++) catch_step(&g, 20);
    check("catch: Santa: too early to catch", catch_lit_lane(&g) < 0 || !catch_key(&g, CATCH_LANE_FIRST[catch_lit_lane(&g)]));
    catch_run(LV_SANTA, 5, 1, 60000, &g);
    check("catch: Santa speeds up as you catch", g.speed > (60 << 8) && g.score > 5u);
    check("catch: the same seed plays the same game", catch_run(LV_ELF, 11, 1, 30000, &g) == catch_run(LV_ELF, 11, 1, 30000, &h));
    catch_start(&g, LV_LITTLE, 1);
    for (k = 0; k < 100u; k++) catch_step(&g, 20);
    check("catch: a key in another lane catches nothing", catch_lit_lane(&g) >= 0 && !catch_key(&g, CATCH_LANE_FIRST[(catch_lit_lane(&g) + 3) % 7]));
}

/* ---- the Advent calendar, and the grown-up gesture ---- */
static void send_date(tn_settings_t *s, advent_rx_t *rx, uint32_t y, uint32_t m, uint32_t d, int bad_check, int *changed)
{
    *changed = 0;
    advent_midi(s, rx, 0xBF, 85, (uint8_t)(y - 2000u));
    advent_midi(s, rx, 0xBF, 86, (uint8_t)m);
    advent_midi(s, rx, 0xBF, 87, (uint8_t)d);
    *changed = advent_midi(s, rx, 0xBF, 88, (uint8_t)(((y - 2000u + m + d) & 127u) ^ (bad_check ? 1u : 0u)));
}
static void test_advent(void)
{
    tn_settings_t s;
    advent_rx_t rx = {0};
    int ch, i, ok;
    gesture_t g = {0};
    tn_set_defaults(&s);
    check("advent: never told a date, first power-on: one door", advent_boot(&s) && advent_open_upto(&s) == 1u);
    check("advent: switched on again without opening it: still one", !advent_boot(&s) && advent_open_upto(&s) == 1u);
    check("advent: door 2 may not open yet", !advent_open(&s, 2));
    check("advent: door 1 opens, once", advent_open(&s, 1) && !advent_open(&s, 1) && advent_is_open(&s, 1));
    check("advent: the next power-on: two doors", advent_boot(&s) && advent_open_upto(&s) == 2u);
    tn_set_defaults(&s);
    send_date(&s, &rx, 2026, 11, 23, 0, &ch);
    check("advent: 23 November: a date, no doors", ch && advent_open_upto(&s) == 0u && advent_today(&s) == 0);
    send_date(&s, &rx, 2026, 12, 1, 0, &ch);
    check("advent: 1 December: one door", ch && advent_open_upto(&s) == 1u && advent_today(&s) == 1);
    send_date(&s, &rx, 2026, 12, 9, 1, &ch);
    check("advent: a frame with a bad check is ignored", !ch && s.date_d == 1u);
    advent_midi(&s, &rx, 0xBF, 85, 26); advent_midi(&s, &rx, 0xBF, 87, 9);
    check("advent: a frame missing its month is ignored", !advent_midi(&s, &rx, 0xBF, 88, (26 + 9) & 127) && s.date_d == 1u);
    check("advent: other channels are not the page", !advent_midi(&s, &rx, 0xB0, 88, 0));
    send_date(&s, &rx, 2026, 12, 9, 0, &ch);
    check("advent: 9 December: nine doors", ch && advent_open_upto(&s) == 9u);
    send_date(&s, &rx, 2026, 12, 5, 0, &ch);
    check("advent: the date never goes back", !ch && s.date_d == 9u);
    send_date(&s, &rx, 2026, 12, 9, 0, &ch);
    check("advent: the same date twice changes nothing", !ch);
    send_date(&s, &rx, 2026, 12, 32, 0, &ch);
    check("advent: 32 December is not a date", !ch && s.date_d == 9u);
    for (i = 1, ok = 1; i <= 9; i++) ok &= advent_open(&s, (uint32_t)i);
    check("advent: doors 1..9 open on the 9th, 10 does not", ok && !advent_open(&s, 10));
    send_date(&s, &rx, 2026, 12, 26, 0, &ch);
    check("advent: after the 24th: all 24", advent_open_upto(&s) == 24u);
    send_date(&s, &rx, 2027, 1, 3, 0, &ch);
    check("advent: in January: all 24, and opened doors stay open", advent_open_upto(&s) == 24u && advent_is_open(&s, 9));
    check("advent: by hand, a real date only", !advent_set_date(&s, 2027, 2, 30) && advent_set_date(&s, 2027, 2, 28));
    /* the grown-up gesture */
    check("gesture: EDIT alone never", !gesture_step(&g, 1u << TB_EDIT, 0) && !gesture_step(&g, 1u << TB_EDIT, 5000));
    memset(&g, 0, sizeof g);
    check("gesture: SAVE alone never", !gesture_step(&g, 1u << TB_SAVE, 0) && !gesture_step(&g, 1u << TB_SAVE, 5000));
    memset(&g, 0, sizeof g);
    gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 1000);
    check("gesture: both for 1999 ms: not yet", !gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 2999));
    check("gesture: both for 2000 ms: yes", gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 3000));
    check("gesture: and only once per hold", !gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 6000));
    memset(&g, 0, sizeof g);
    gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 0);
    gesture_step(&g, 1u << TB_EDIT, 1500);       /* let go of SAVE, grab it again */
    gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 1600);
    check("gesture: a break starts the 2 s again", !gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 2100) && gesture_step(&g, 1u << TB_EDIT | 1u << TB_SAVE, 3600));
    {                                            /* "go": a key or any button but HOME, EDIT, SAVE */
        tn_input_t in;
        uint32_t b, ok = 1;
        memset(&in, 0, sizeof in);
        for (b = 0; b < 14u; b++) {
            in.btn_down = 1u << b;
            ok &= tn_go(&in, 0) == !(b == TB_HOME || b == TB_EDIT || b == TB_SAVE);
        }
        in.btn_down = 1u << TB_OCT_UP;
        ok &= !tn_go(&in, 1u << TB_OCT_UP);
        in.btn_down = 0; in.key_down = 1u << 20;
        ok &= tn_go(&in, 0);
        check("go: any key or button opens, but HOME, EDIT and SAVE (and what the screen uses) never", ok);
    }
    tn_set_defaults(&s);
    check("settings: the defaults are valid", tn_set_valid(&s));
    s.level[3] = 9;
    check("settings: rubbish (another firmware's, a broken save) is refused", !tn_set_valid(&s));
}

/* ---- change ringing ---- */
static uint32_t ring_code(const ring_t *r) { uint32_t i, c = 0; for (i = 0; i < r->bells; i++) c = c * 10u + r->row[i]; return c; }
static uint32_t ring_course(uint32_t m, uint32_t *lead_head)   /* rows of the plain course; the first lead head */
{
    ring_t r;
    uint32_t n = 0;
    ring_start(&r, m);
    *lead_head = 0;
    do {
        ring_next(&r);
        n++;
        if (r.pos == 0u && !*lead_head) *lead_head = ring_code(&r);
    } while (!r.rounds_again && n < 10000u);
    return n;
}
static uint32_t ring_extent(uint32_t m, const uint8_t *calls, uint32_t ncalls, uint32_t reps)   /* distinct rows of a touch */
{
    static uint8_t seen[100000];
    ring_t r;
    uint32_t leads = 0, rows = 0, distinct = 0;
    memset(seen, 0, sizeof seen);
    ring_start(&r, m);
    while (leads < ncalls * reps && rows < 20000u) {
        if (r.pos == 0u) ring_call(&r, calls[leads % ncalls]);
        ring_next(&r);
        rows++;
        if (!seen[ring_code(&r) % 100000u]) { seen[ring_code(&r) % 100000u] = 1; distinct++; }
        if (r.pos == 0u) leads++;
    }
    return distinct + (ring_is_rounds(&r) ? 0u : 1000u);   /* (it must come round) */
}
static void test_ring(void)
{
    uint32_t lh, n;
    char what[100];
    static const uint8_t PB[4] = {CALL_NONE, CALL_NONE, CALL_NONE, CALL_BOB};
    static const uint8_t GR[6] = {CALL_NONE, CALL_BOB, CALL_NONE, CALL_BOB, CALL_NONE, CALL_SINGLE};
    ring_t r;
    n = ring_course(RM_HUNT5, &lh);
    snprintf(what, sizeof what, "ring: plain hunt on 5 comes round in 10 rows (%u)", n); check(what, n == 10u);
    n = ring_course(RM_HUNT6, &lh);
    snprintf(what, sizeof what, "ring: plain hunt on 6 comes round in 12 rows (%u)", n); check(what, n == 12u);
    n = ring_course(RM_BOB_DOUBLES, &lh);
    snprintf(what, sizeof what, "ring: plain bob doubles: lead head 13524, 40 rows (%u, %u)", lh, n); check(what, lh == 13524u && n == 40u);
    n = ring_course(RM_BOB_MINOR, &lh);
    snprintf(what, sizeof what, "ring: plain bob minor: lead head 135264, 60 rows (%u, %u)", lh, n); check(what, lh == 135264u && n == 60u);
    n = ring_course(RM_GRANDSIRE, &lh);
    snprintf(what, sizeof what, "ring: grandsire doubles: lead head 12534, 30 rows (%u, %u)", lh, n); check(what, lh == 12534u && n == 30u);
    n = ring_extent(RM_BOB_DOUBLES, PB, 4, 3);
    snprintf(what, sizeof what, "ring: plain bob doubles, P P P B three times: the 120-row extent (%u)", n); check(what, n == 120u);
    n = ring_extent(RM_GRANDSIRE, GR, 6, 2);
    snprintf(what, sizeof what, "ring: grandsire doubles, P B P B P S twice: all 120 rows (%u)", n); check(what, n == 120u);
    ring_start(&r, RM_ROUNDS);
    check("ring: rounds on six is rounds", ring_named(&r) == 1u);
    ring_swap(&r, 1); ring_swap(&r, 3); ring_swap(&r, 2);   /* 123456 -> 132456 -> 132546 -> 135246 */
    check("ring: three call changes make Queens", ring_named(&r) == 2u);
    check("ring: no swap past the tenor", !ring_swap(&r, 5));
}

/* ---- the other games ---- */
static void test_games(void)
{
    jingle_t j;
    cracker_t c;
    rally_t r;
    ride_t d;
    uint32_t t, e, good = 0, over = 0, ok;
    /* Jingle It! */
    jingle_start(&j, LV_LITTLE, 5, 0);
    for (t = 0, ok = 1; t < 300000u; t += 10u) {
        e = jingle_tick(&j, t);
        ok &= e != JE_MISS && e != JE_OVER && j.call != JC_SHH;
        if (j.active && t - j.t_call > 30000u) { jingle_act(&j, JA_KEY, 3, t); jingle_act(&j, JA_TWIST, 0, t); jingle_act(&j, JA_BUTTON, 0, t); }
    }
    check("jingle: Little Elf never misses, never ends, never asks for SHH", ok && !j.done);
    jingle_start(&j, LV_LITTLE, 5, 0);
    for (t = 0; t < 2000u; t += 10u) jingle_tick(&j, t);
    check("jingle: Little Elf: a wrong thing is just wrong, the right one is good",
          j.active && (j.call == JC_RING ? jingle_act(&j, JA_TWIST, 0, t) == JE_WRONG && jingle_act(&j, JA_KEY, 26, t) == JE_GOOD
                                         : jingle_act(&j, JA_KEY, 4, t) == JE_WRONG && jingle_act(&j, j.call == JC_TWIST ? JA_TWIST : JA_BUTTON, 0, t) == JE_GOOD));
    jingle_start(&j, LV_ELF, 9, 0);
    for (t = 0; t < 120000u && !j.done; t += 10u) if (jingle_tick(&j, t) == JE_OVER) over++;
    check("jingle: Elf left alone loses three lives (SHH calls are passed by waiting)", j.done && over == 1u && j.lives == 0u);
    jingle_start(&j, LV_SANTA, 3, 0);
    for (t = 0; t < 120000u && !j.done; t += 10u) {
        jingle_tick(&j, t);
        if (j.active && t - j.t_call == 300u) {
            if (j.call == JC_RING) e = jingle_act(&j, JA_KEY, j.key, t);
            else if (j.call == JC_TWIST) e = jingle_act(&j, JA_TWIST, 0, t);
            else if (j.call == JC_JINGLE) e = jingle_act(&j, JA_BUTTON, 0, t);
            else e = JE_NONE;
            if (e == JE_GOOD) good++;
        }
    }
    check("jingle: Santa played right keeps going, and gets quicker", !j.done && good > 30u && j.window < 2500u);
    /* Cracker Pull */
    cracker_start(&c, LV_LITTLE, 0, 0);
    for (t = 0; t < 60000u && !c.done; t += 10u) cracker_tick(&c, t);
    check("cracker: Little Elf goes bang even if nobody pulls", c.done && t < 20000u);
    cracker_start(&c, LV_ELF, 1, 0);
    check("cracker: on the beat pulls harder than off it", cracker_tap(&c, 0, 600) == 4u && cracker_tap(&c, 0, 300) == 1u);
    cracker_start(&c, LV_ELF, 1, 0);
    for (t = 0; !c.done && t < 100000u; t += 600u) { cracker_tap(&c, 0, t); if (t % 1200u == 0u) cracker_tap(&c, 1, t + 300u); }
    check("cracker: two players: the one pulling on the beat wins", c.done && c.winner == 0u);
    /* Snowball Rally */
    rally_start(&r, LV_ELF, 4);
    for (t = 0, ok = 1; t < 60000u; t += 10u) {
        int32_t y = r.by >> 8;
        rally_move(&r, 0, (y - r.py[0]) / 8); rally_move(&r, 1, (y - r.py[1]) / 8);   /* (perfect players) */
        e = rally_step(&r, 10);
        ok &= e != RE_MISS_L && e != RE_MISS_R;
        ok &= (r.by >> 8) >= RL_TOP && (r.by >> 8) <= RL_BOT;
    }
    check("rally: perfect players never miss, and the ball stays on the roof", ok && r.hits > 20u);
    rally_start(&r, LV_LITTLE, 4);
    for (t = 0; t < 60000u; t += 10u) rally_step(&r, 10);
    check("rally: nobody playing: the ball keeps going (hits, or snowmen), nothing ends", r.snowmen[0] + r.snowmen[1] + r.hits > 10u);
    rally_start(&r, LV_SANTA, 7);
    for (t = 0; t < 30000u; t += 10u) { int32_t y = r.by >> 8; rally_move(&r, 0, (y - r.py[0]) / 8); rally_move(&r, 1, (y - r.py[1]) / 8); rally_step(&r, 10); }
    check("rally: Santa speeds up with every hit", r.speed > (140 << 8));
    rally_start(&r, LV_ELF, 1);
    for (t = 0; t < 30000u; t += 10u) { rally_elf(&r, 0, 10); rally_elf(&r, 1, 10); rally_step(&r, 10); }
    check("rally: two elves keep a rally going", r.hits > 10u);
    /* Sleigh Ride */
    ride_start(&d, LV_LITTLE, 2);
    for (t = 0, ok = 1; t < 120000u; t += 20u) { ok &= ride_step(&d, 20) == RD_NONE; }
    check("ride: Little Elf left alone never ends", ok && !d.done);
    ride_start(&d, LV_LITTLE, 2);
    for (t = 0; t < 60000u; t += 20u) { ride_step(&d, 20); ride_key(&d, 13); }
    check("ride: Little Elf: any key rings the posts as they pass", d.score > 20u);
    ride_start(&d, LV_ELF, 2);
    for (t = 0; t < 600000u && !d.done; t += 20u) ride_step(&d, 20);
    check("ride: Elf left alone ends after three posts are missed", d.done && d.lives == 0u);
    ride_start(&d, LV_ELF, 3);
    for (t = 0, ok = 0; t < 60000u && !d.done; t += 20u) {
        int lane = ride_lit_lane(&d);
        ride_step(&d, 20);
        if (lane >= 0) { ride_steer(&d, (ride_post_x((uint32_t)lane) - d.x) / 12); if (ride_key(&d, CATCH_LANE_FIRST[lane])) ok++; }
    }
    check("ride: Elf steered to each post and rung with its keys keeps going", !d.done && ok > 20u);
}

static void write_wav(const char *name)
{
    FILE *f = fopen(name, "wb");
    uint32_t bytes = wav_n * 4u, v[] = {16, 0x00020001u, 44100, 44100 * 4u, 0x00100004u};
    if (!f) return;
    fwrite("RIFF", 1, 4, f); bytes += 36; fwrite(&bytes, 4, 1, f); bytes -= 36;
    fwrite("WAVEfmt ", 1, 8, f); fwrite(v, 4, 5, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    fwrite(wav, 4, wav_n, f);
    fclose(f);
    printf("sound -> %s\n", name);
}
static void demo_wav(const char *name)            /* every instrument, a shake, the effects: for a person to listen */
{
    static const uint8_t TUNE[] = {64, 64, 64, 64, 64, 64, 64, 67, 60, 62, 64};
    uint32_t i, k;
    wav = malloc(44100u * 4u * 60u);
    snd_reset();
    for (i = 0; i < TN_NINST; i++) {
        for (k = 0; k < sizeof TUNE; k++) { snd_ping(i, TUNE[k] + (i == TI_SOFTBASS ? 0u : 12u) - (i == TI_SOFTBASS ? 24u : 0u), 100, 0); render_ms(k == 2u || k == 5u ? 500 : 250); }
        render_ms(1200);
    }
    for (k = 0; k < 8u; k++) { snd_shake(k & 1u ? 70 : 127); render_ms(250); }
    render_ms(800);
    for (i = 0; i < TN_NSFX; i++) { snd_fx(i); render_ms(1500); }
    write_wav(name);
}

int main(int argc, char **argv)
{
    if (argc > 2 && !strcmp(argv[1], "wav")) { demo_wav(argv[2]); return 0; }
    test_store();
    test_sound();
    test_carols();
    test_catch();
    test_advent();
    test_ring();
    test_games();
    printf("tinsel: %d checks, %d failed\n", total, bad);
    return bad != 0;
}
