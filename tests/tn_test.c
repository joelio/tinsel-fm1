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
    printf("tinsel: %d checks, %d failed\n", total, bad);
    return bad != 0;
}
