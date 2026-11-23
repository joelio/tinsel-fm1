/* SPDX-License-Identifier: GPL-3.0-only */
/* Where Dinghy keeps your data in flash, per board (platform.c: settings_*, blob_*, slot_*). Macros and compile-time
 * checks only, so the host tests include it (tests/smk_flash_test.c).
 *
 *              FM-1 (Tinsel's own window)    SMK-37 (Pro, Elite, MKE-P37)
 *   settings   0xB7000, 1 sector             0x9D000, 1 sector
 *   blobs      0xB8000.. NBLOB sectors       0x9E000.. NBLOB sectors
 *   slots      0xC0000.. NSLOT x SLOT_SECT   0xC2000.. NSLOT x SLOT_SECT (USRFLASH, SLOOP's sample slots)
 *
 * Tinsel (docs/DESIGN.md section 14): every Dinghy fork keeps its store at Dinghy's places (settings 0xFC000, blobs
 * 0xF4000.., slots 0x97000..0xB6FFF). A magic stops a firmware reading another's data, not overwriting it, and Tinsel
 * saves by itself (an Advent door, a date): on a unit that goes back and forth between Tinsel and Melisma (or Dinghy)
 * it would wipe theirs. So Tinsel keeps its whole store in 0xB7000..0xDFFFF: inside Felucca's FL_DATA, above Dinghy's
 * slots, below the update staging at 0xE0000, exactly 1 + 8 + 8 x 4 sectors. FL_RANGE_OK allows that window and the
 * update staging, nothing else.
 *   owner tag  -                             0xAF000: "SMK37DATA" + DATA_FAMILY (smk_owner_check)
 *
 * On the FM-1 the other firmwares of the family keep their data in 0x97000..0xFBFFF too: a new firmware finds the
 * magic or the length wrong and starts fresh (give your fork its own STORE_MAGIC). The SMK-37's store is shared by
 * the ports under the fm1-to-smk37 kit's convention: whose data it is says the owner tag, anything else is cleared
 * once at boot (the stock settings, another port's), never parsed. Calibration (0xC1000), BT pairing, the stock
 * resources and key_mac are never in reach: FL_RANGE_OK below is the app's allow-list, and the RAM flash driver
 * refuses every protected range on its own as well (FL_GUARD, hal/fm1_flash_map.h). */
#pragma once
#include <stdint.h>

#ifndef STORE_MAGIC
#define STORE_MAGIC 0x4C534E54u                 /* "TNSL": Tinsel's settings and blobs (slots: "SLOT" ^ this) */
#endif
#define NBLOB 8u                                /* blobs: one 4 KiB sector each */
#define NSLOT 8u                                /* slots: SLOT_SECT sectors each, for something bigger (samples, songs) */
#define SLOT_SECT 4u

#if BOARD_SMK37
#define SETTINGS_AT 0x0009D000u
#define BLOBS_AT 0x0009E000u
#define OWNER_AT 0x000AF000u                    /* (the kit's place for it) */
#define SLOTS_AT 0x000C2000u
#ifndef DATA_FAMILY
#define DATA_FAMILY "DINGHY"                    /* the owner tag's family: 1..7 of A-Z 0-9 (tools/build.py DATA_FAMILY) */
#endif
#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u) || \
                             FL_IN(off, n, BLOBS_AT, BLOBS_AT + NBLOB * 0x1000u) || FL_IN(off, n, OWNER_AT, OWNER_AT + 0x1000u) || \
                             FL_IN(off, n, SLOTS_AT, SLOTS_AT + NSLOT * SLOT_SECT * 0x1000u))
#else
#define TN_STORE_LO 0x000B7000u                 /* Tinsel's window: [TN_STORE_LO, TN_STORE_HI) */
#define TN_STORE_HI 0x000E0000u
#define SETTINGS_AT 0x000B7000u
#define BLOBS_AT 0x000B8000u
#define SLOTS_AT 0x000C0000u
#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_IN(off, n, TN_STORE_LO, TN_STORE_HI))
#endif
#include "fm1_flash_map.h"
#define SECTOR_MAX (4096u - 12u)                /* settings, a blob: the sector minus the header */
#define SLOT_MAX (SLOT_SECT * 0x1000u - 12u)

#if BOARD_SMK37
_Static_assert(SETTINGS_AT >= FL_DATA_LO && SETTINGS_AT + 0x1000u <= BLOBS_AT, "SMK store: settings outside FL_DATA");
_Static_assert(BLOBS_AT + NBLOB * 0x1000u <= OWNER_AT && OWNER_AT + 0x1000u <= FL_DATA_HI, "SMK store: blobs / owner tag");
_Static_assert(SLOTS_AT >= FL_SMP_LO && SLOTS_AT + NSLOT * SLOT_SECT * 0x1000u <= FL_SMP_HI, "SMK store: slots outside FL_SMP");
_Static_assert(FL_WIN_SAFE(SLOTS_AT, SLOTS_AT + NSLOT * SLOT_SECT * 0x1000u), "SMK store: the slots overlap a protected range");
_Static_assert(FL_WIN_SAFE(SETTINGS_AT, OWNER_AT + 0x1000u), "SMK store: overlaps a protected range");
_Static_assert(sizeof DATA_FAMILY - 1u >= 1u && sizeof DATA_FAMILY - 1u <= 7u, "SMK store: family 1..7 characters");

/* the 16-byte owner tag: "SMK37DATA" + the family, zero padded (a 7-letter family fills it, no NUL) */
static void smk_owner_tag(uint8_t tag[16])
{
    static const char FAM[] = DATA_FAMILY;
    uint32_t i;
    for (i = 0; i < 16u; i++)
        tag[i] = i < 9u ? (uint8_t)"SMK37DATA"[i] : i - 9u < sizeof FAM - 1u ? (uint8_t)FAM[i - 9u] : 0u;
}
#else
_Static_assert(TN_STORE_LO >= FL_DATA_LO && TN_STORE_HI <= FL_DATA_HI && TN_STORE_HI <= FL_OTA_LO, "Tinsel store: outside FL_DATA");
_Static_assert(TN_STORE_LO >= 0xB7000u, "Tinsel store: reaches Dinghy's (Melisma's) slots");
_Static_assert(SETTINGS_AT == TN_STORE_LO && SETTINGS_AT + 0x1000u <= BLOBS_AT, "Tinsel store: settings");
_Static_assert(BLOBS_AT + NBLOB * 0x1000u <= SLOTS_AT, "Tinsel store: blobs overlap the slots");
_Static_assert(SLOTS_AT + NSLOT * SLOT_SECT * 0x1000u <= TN_STORE_HI, "Tinsel store: slots past the window");
#endif
