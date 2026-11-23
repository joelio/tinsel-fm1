/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* What Tinsel keeps (DESIGN section 14), in the settings sector of its own store window. Pure: the tests include it. */
#ifndef TN_SET_H
#define TN_SET_H
#include <stdint.h>

#define TN_SET_VER 1u
typedef struct {
    uint8_t ver, ceiling, sound, oct;           /* ceiling: 0 gentle, 1 medium, 2 full */
    uint8_t level[8];                           /* each game's level: 0 Little Elf, 1 Elf, 2 Santa (0: Present Catch) */
    uint32_t doors;                             /* Advent doors opened: bit d-1 */
    uint16_t date_y;                            /* the latest date Tinsel was told (0: never) */
    uint8_t date_m, date_d;
    uint8_t power_days;                         /* days switched on, for a calendar never told the date */
    uint8_t adv_all, pad[2];                    /* the grown-up opened the whole calendar */
    uint16_t best[8];                           /* high scores (0: Present Catch) */
} tn_settings_t;
enum { GM_CATCH, GM_JINGLE, GM_CRACKER, GM_RALLY, GM_RIDE, GM_BELLS, GM_COUNT };   /* level[] and best[] */
static const uint16_t TN_CEILINGS[3] = {16384, 22937, 32767};   /* gentle 50 %, medium 70 %, full */

static void tn_set_defaults(tn_settings_t *s) { memset(s, 0, sizeof *s); s->ver = TN_SET_VER; }
static int tn_set_valid(const tn_settings_t *s)
{
    uint32_t i;
    if (s->ver != TN_SET_VER || s->ceiling > 2u || s->power_days > 24u || (int8_t)s->oct < -1 || (int8_t)s->oct > 2) return 0;
    for (i = 0; i < 8u; i++) if (s->level[i] > 2u) return 0;
    if (s->date_y && (s->date_m < 1u || s->date_m > 12u || s->date_d < 1u || s->date_d > 31u)) return 0;
    return 1;
}
#endif
