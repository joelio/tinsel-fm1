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

int main(void)
{
    test_store();
    printf("tinsel: %d checks, %d failed\n", total, bad);
    return bad != 0;
}
