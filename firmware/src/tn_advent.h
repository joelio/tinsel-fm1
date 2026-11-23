/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The Advent calendar's rules (DESIGN section 13). Pure: the tests include it.
 *
 * The FM-1 keeps no date when it is off. A small web page (web/advent.html) sends today's date as four control
 * changes on MIDI channel 16: CC 85 the year minus 2000, CC 86 the month, CC 87 the day, CC 88 a check
 * ((y + m + d) & 127). Tinsel keeps the latest date it has been told; it never goes back. Doors up to today open.
 * Never told a date, it opens a door for each day it is switched on (the first power-on after a door was opened);
 * the grown-up panel can set the date by hand, or open everything. Doors only ever open: nothing is taken away. */
#ifndef TN_ADVENT_H
#define TN_ADVENT_H

#define ADV_DOORS 24u
#define ADV_ALL 0xFFFFFFu

/* how many doors may be open now (0..24) */
static uint32_t advent_open_upto(const tn_settings_t *s)
{
    if (s->adv_all) return ADV_DOORS;
    if (s->date_y) {
        if (s->date_m == 12u) return s->date_d > ADV_DOORS ? ADV_DOORS : s->date_d;
        if (s->date_m <= 2u) return ADV_DOORS;   /* (Christmas has been: the whole calendar) */
        return 0;                                /* before December */
    }
    return s->power_days > ADV_DOORS ? ADV_DOORS : s->power_days;
}

/* at power-on, a calendar never told a date counts a day: when it is first switched on, and after a door opened */
static int advent_boot(tn_settings_t *s)
{
    if (s->date_y || s->power_days >= ADV_DOORS) return 0;
    if (s->power_days == 0u || (s->doors >> (s->power_days - 1u) & 1u)) { s->power_days++; return 1; }
    return 0;
}

static int advent_may_open(const tn_settings_t *s, uint32_t door)   /* door 1..24 */
{
    return door >= 1u && door <= ADV_DOORS && door <= advent_open_upto(s);
}
static int advent_is_open(const tn_settings_t *s, uint32_t door) { return door >= 1u && door <= ADV_DOORS && (s->doors >> (door - 1u) & 1u); }
static int advent_open(tn_settings_t *s, uint32_t door)   /* 1 if it opened now */
{
    if (!advent_may_open(s, door) || advent_is_open(s, door)) return 0;
    s->doors |= 1u << (door - 1u);
    return 1;
}
static int advent_today(const tn_settings_t *s)   /* the newest door that may open and has not (0: none) */
{
    uint32_t d = advent_open_upto(s);
    for (; d >= 1u; d--) if (!advent_is_open(s, d)) return (int)d;
    return 0;
}

/* a date, by hand or from the page: kept if it is a real date and not before the one kept; 1 if it changed */
static int advent_set_date(tn_settings_t *s, uint32_t y, uint32_t m, uint32_t d)
{
    static const uint8_t DAYS[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t now = (uint32_t)s->date_y * 512u + s->date_m * 32u + s->date_d, new_ = y * 512u + m * 32u + d;
    if (y < 2024u || y > 2099u || m < 1u || m > 12u || d < 1u || d > DAYS[m - 1u]) return 0;
    if (s->date_y && new_ <= now) return 0;
    s->date_y = (uint16_t)y; s->date_m = (uint8_t)m; s->date_d = (uint8_t)d;
    return 1;
}

/* the page's frame, a message at a time: 1 when a whole, checked frame changed the date */
typedef struct { uint8_t y, m, d, have; } advent_rx_t;
static int advent_midi(tn_settings_t *s, advent_rx_t *rx, uint8_t st, uint8_t d1, uint8_t d2)
{
    if (st != 0xBFu) return 0;                   /* control change, channel 16 */
    switch (d1) {
    case 85: rx->y = d2; rx->have |= 1u; break;
    case 86: rx->m = d2; rx->have |= 2u; break;
    case 87: rx->d = d2; rx->have |= 4u; break;
    case 88: {
        int ok = rx->have == 7u && ((rx->y + rx->m + rx->d) & 127u) == d2;
        rx->have = 0;
        return ok && advent_set_date(s, 2000u + rx->y, rx->m, rx->d);
    }
    default: return 0;
    }
    return 0;
}

/* what is behind each door */
enum { GIFT_CAROL, GIFT_JOKE, GIFT_DECOR, GIFT_SOUND };
typedef struct { uint8_t kind, pic, arg; const char *line1, *line2; } advent_gift_t;
static const advent_gift_t ADV_GIFT[ADV_DOORS] = {
    {GIFT_CAROL, PIC_BELLS, 0, "JINGLE BELLS", 0},
    {GIFT_DECOR, PIC_HOLLY, 1, "A CANDY CANE", "FOR THE GROTTO"},
    {GIFT_JOKE, PIC_PRESENT, 0, "WHAT DOES A BAUBLE", "DO ALL DAY? HANG ABOUT!"},
    {GIFT_DECOR, PIC_STAR, 2, "A GINGERBREAD MAN", "FOR THE GROTTO"},
    {GIFT_CAROL, PIC_HOLLY, 2, "DECK THE HALLS", 0},
    {GIFT_DECOR, PIC_CANDLE, 3, "A STOCKING", "ST NICHOLAS DAY"},
    {GIFT_JOKE, PIC_PUDDING, 1, "WHAT IS A ROBIN'S", "FAVOURITE CAROL? TWEET NIGHT"},
    {GIFT_CAROL, PIC_STAR, 3, "SILENT NIGHT", 0},
    {GIFT_JOKE, PIC_TREE, 2, "WHY ARE TREES BAD", "AT SEWING? NEEDLES DROP"},
    {GIFT_SOUND, PIC_BELL, 0, "A SLEIGH BELL", 0},
    {GIFT_CAROL, PIC_CROWN, 4, "GOOD KING WENCESLAS", 0},
    {GIFT_JOKE, PIC_BELLS, 3, "WHAT DO SNOWMEN", "EAT? ICEBERG-ERS"},
    {GIFT_CAROL, PIC_ANGEL, 5, "JOY TO THE WORLD", 0},
    {GIFT_JOKE, PIC_PRESENT, 4, "WHAT DID THE PRESENT", "SAY? I'M ALL WRAPPED UP"},
    {GIFT_SOUND, PIC_STAR, 1, "A SHOOTING STAR", 0},
    {GIFT_CAROL, PIC_CANDLE, 6, "GOD REST YE MERRY", "GENTLEMEN"},
    {GIFT_JOKE, PIC_HOLLY, 5, "WHO HIDES IN THE", "BAKERY? A MINCE SPY"},
    {GIFT_CAROL, PIC_BELL, 7, "SHCHEDRYK", 0},
    {GIFT_JOKE, PIC_TREE, 6, "WHAT DO REINDEER", "HANG UP? HORN-AMENTS"},
    {GIFT_SOUND, PIC_BELLS, 2, "A FANFARE", 0},
    {GIFT_JOKE, PIC_PUDDING, 7, "WHAT IS A SNOWMAN'S", "BREAKFAST? SNOWFLAKES"},
    {GIFT_CAROL, PIC_PUDDING, 1, "WE WISH YOU A", "MERRY CHRISTMAS"},
    {GIFT_JOKE, PIC_STAR, 0, "WHO SAYS OH OH OH?", "SANTA WALKING BACKWARDS"},
    {GIFT_CAROL, PIC_STAR, 3, "SILENT NIGHT", "MERRY CHRISTMAS EVE"},
};
#endif
