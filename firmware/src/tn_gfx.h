/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel's drawing: the whole screen as one byte a pixel, a palette of 256 colours, and a flush that sends only what
 * changed since the last one.
 *
 * Each frame a screen draws itself completely into g_fb (it is cheap: a few milliseconds of the processor). g_flush
 * compares it with what the panel shows (g_shown), and for each band of G_BAND rows sends the columns that differ,
 * expanded to RGB565 through g_pal, one band building while the last goes out by DMA. The SPI link is the slow part
 * (DESIGN section 12), so a small change costs a small transfer.
 *
 * Drawing calls clip to the screen. Colours are palette indices; tn_pal.h names them. Integers only. */
#ifndef TN_GFX_H
#define TN_GFX_H
#include <stdint.h>

#define G_W 240
#define G_H 240
#define G_BAND 16
#define G_NBAND (G_H / G_BAND)
static uint8_t g_fb[G_W * G_H] TN_POOL;          /* what the screen draws */
static uint8_t g_shown[G_W * G_H] TN_POOL;       /* what the panel shows */
static uint16_t g_px[2][G_W * G_BAND] TN_POOL;   /* two bands of RGB565, byte-swapped, for the DMA */
static uint16_t g_pal[256];                      /* RGB565, byte-swapped for the panel */
static uint8_t g_all;                            /* the next flush sends everything (a palette change, a new screen) */
static uint32_t g_sent_px;                       /* pixels sent by the last flush (tests, the panel's screen test) */

static void g_pal_set(uint32_t i, uint32_t r, uint32_t g, uint32_t b)
{
    uint32_t c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    g_pal[i & 255u] = (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8));
    g_all = 1;
}

static void g_clear(uint32_t c) { memset(g_fb, (int)c, sizeof g_fb); }

static inline void g_pset(int32_t x, int32_t y, uint32_t c)
{
    if ((uint32_t)x < G_W && (uint32_t)y < G_H) g_fb[(uint32_t)y * G_W + (uint32_t)x] = (uint8_t)c;
}

static void g_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c)
{
    int32_t x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > G_W) x1 = G_W;
    if (y1 > G_H) y1 = G_H;
    for (; y < y1; y++)
        if (x1 > x) memset(&g_fb[(uint32_t)y * G_W + (uint32_t)x], (int)c, (uint32_t)(x1 - x));
}

static void g_hline(int32_t x0, int32_t x1, int32_t y, uint32_t c) { if (x1 >= x0) g_rect(x0, y, x1 - x0 + 1, 1, c); }

static void g_disc(int32_t cx, int32_t cy, int32_t r, uint32_t c)
{
    int32_t y, x = r;
    for (y = 0; y <= r; y++) {
        while (x > 0 && x * x + y * y > r * r + r) x--;
        g_hline(cx - x, cx + x, cy - y, c);
        if (y) g_hline(cx - x, cx + x, cy + y, c);
    }
}

static void g_ring(int32_t cx, int32_t cy, int32_t r, uint32_t c)   /* a circle's outline */
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        g_pset(cx + x, cy + y, c); g_pset(cx - x, cy + y, c); g_pset(cx + x, cy - y, c); g_pset(cx - x, cy - y, c);
        g_pset(cx + y, cy + x, c); g_pset(cx - y, cy + x, c); g_pset(cx + y, cy - x, c); g_pset(cx - y, cy - x, c);
        y++;
        if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
}

/* a triangle with a flat bottom: apex (ax, ay), base from (bx0, by) to (bx1, by) */
static void g_tri_up(int32_t ax, int32_t ay, int32_t bx0, int32_t bx1, int32_t by, uint32_t c)
{
    int32_t y, h = by - ay;
    if (h <= 0) return;
    for (y = 0; y <= h; y++)
        g_hline(ax + (bx0 - ax) * y / h, ax + (bx1 - ax) * y / h, ay + y, c);
}

static void g_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 1000;
    while (guard--) {
        int32_t e2 = 2 * err;
        g_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* A sprite drawn in text: one string a row, each character a colour from map ("r" -> index), '.' or ' ' clear.
 * scale 1..4 makes chunky pixels; flip mirrors it. Small art lives in the source, readable as a picture. */
typedef struct { const char *const *rows; uint8_t w, h; const char *keys; const uint8_t *cols; } g_sprite_t;
static void g_sprite(const g_sprite_t *s, int32_t x, int32_t y, int32_t scale, int flip)
{
    uint32_t r, k;
    for (r = 0; r < s->h; r++) {
        const char *row = s->rows[r];
        for (k = 0; k < s->w && row[k]; k++) {
            const char *p;
            uint32_t ch = (uint8_t)row[flip ? s->w - 1u - k : k];
            if (ch == '.' || ch == ' ') continue;
            for (p = s->keys; *p && (uint8_t)*p != ch; p++) {}
            if (!*p) continue;
            g_rect(x + (int32_t)k * scale, y + (int32_t)r * scale, scale, scale, s->cols[p - s->keys]);
        }
    }
}

/* text from the platform's fonts (4-bit alpha): solid where it is mostly ink, aa where it is half (0xFF: none) */
static int32_t g_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint32_t c, uint32_t aa)
{
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v >= 9u) g_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, c);
                else if (v >= 4u && aa != 0xFFu) g_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, aa);
            }
        x += f->adv[gi];
    }
    return x;
}
static void g_text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint32_t c, uint32_t aa)
{
    g_text(cx - text_w(f, s) / 2, y, f, s, c, aa);
}

/* send what changed: per band, the stretches of columns that differ (whole words of 4 pixels; stretches closer than
 * G_GAP words are sent as one, a window command costing about as much as that many pixels) */
#define G_GAP 6u
static void g_send(uint32_t y0, uint32_t x0, uint32_t x1, uint32_t *which)
{
    uint32_t w = x1 - x0, y, x;
    uint16_t *px = g_px[*which];
    for (y = 0; y < G_BAND; y++) {
        const uint8_t *src = &g_fb[(y0 + y) * G_W + x0];
        uint16_t *dst = px + y * w;
        for (x = 0; x < w; x++) dst[x] = g_pal[src[x]];
        memcpy(&g_shown[(y0 + y) * G_W + x0], src, w);
    }
    lcd_blit(x0, y0, w, G_BAND, px);             /* (it waits for the other buffer's transfer before it starts) */
    g_sent_px += w * G_BAND;
    *which ^= 1u;
}
static void g_flush(void)
{
    uint32_t band, which = 0;
    g_sent_px = 0;
    for (band = 0; band < G_NBAND; band++) {
        uint32_t y0 = band * G_BAND, y, i, lo, hi;
        uint32_t m[2] = {0, 0};                  /* changed words: 60 of them, bits 0..31 and 32..59 */
        if (g_all) { g_send(y0, 0, G_W, &which); continue; }
        for (y = y0; y < y0 + G_BAND; y++) {
            const uint32_t *a = (const uint32_t *)(const void *)&g_fb[y * G_W], *b = (const uint32_t *)(const void *)&g_shown[y * G_W];
            for (i = 0; i < G_W / 4u; i++)
                if (a[i] != b[i]) m[i >> 5] |= 1u << (i & 31u);
        }
        if (!m[0] && !m[1]) continue;
        for (i = 0; i < G_W / 4u;) {
            if (!(m[i >> 5] >> (i & 31u) & 1u)) { i++; continue; }
            lo = i;
            hi = i;
            for (; i < G_W / 4u; i++) {          /* the stretch, through gaps shorter than G_GAP */
                if (m[i >> 5] >> (i & 31u) & 1u) hi = i;
                else if (i - hi > G_GAP) break;
            }
            g_send(y0, lo * 4u, hi * 4u + 4u, &which);
            i = hi + 1u;
        }
    }
    g_all = 0;
}
#endif
