/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* Tinsel's sound: bells and friends, sleigh bells, a few effects, a cathedral, and a ceiling nobody gets past.
 *
 *   snd_ping(inst, midi, vel, pan)      a struck note (bells ring on: there is no note off)
 *   snd_ping_at(ms, inst, midi, vel, pan)   the same, ms from now (fanfares, sparkles, the backing's offbeats)
 *   snd_shake(strength)                 sleigh bells: one shake
 *   snd_fx(SFX_...)                     a little effect
 *   snd_hush()                          everything stops within ~50 ms (HOME, leaving a screen)
 *   snd_set_ceiling(q15), snd_set_reverb(0..255)
 *   snd_render(out, n)                  the audio interrupt: adds n stereo frames
 *
 * A voice is up to four sine partials, each with its own level and decay (the bell recipes in docs/RESEARCH-NOTES.md
 * section 5), the first optionally bent by a modulator (FM: the celesta), a short click of noise at the strike, and
 * an attack ramp for soft sounds. The sleigh bells are Perry Cook's PhISEM model as STK ships it (Shakers.cpp,
 * "Sleigh Bells": 32 jingles, five resonators). The reverb is a four-line feedback delay network. After the mix,
 * a block-wise limiter and a clamp hold everything under the ceiling (DESIGN section 9.3).
 *
 * The main loop asks through a ring (one writer each side); everything else belongs to the interrupt.
 * Integers only; no 64-bit arithmetic at all. */
#ifndef TN_SND_H
#define TN_SND_H
#include <stdint.h>

#ifndef TN_POOL
#define TN_POOL __attribute__((section(".pool")))
#endif
#define TN_VOICES 16u
#define TN_CTL 32u                              /* envelopes move once in this many frames */
#define TN_FS 44100u

/* ---- the pitch table (Melisma's / Dinghy's: the phase step of MIDI 120..131; lower octaves shift right) ---- */
static const uint32_t SND_INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};
static uint32_t snd_inc(uint32_t midi) { if (midi > 127u) midi = 127u; return midi >= 120u ? SND_INC_C9[midi - 120u] : SND_INC_C9[midi % 12u] >> (10u - midi / 12u); }

static int32_t snd_sine(uint32_t ph)            /* a cycle over 2^32 -> -32767..32767 (Dinghy's refined parabola) */
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

/* the sine for the voices: a table of 2048 in RAM, filled from the parabola at the first note (-66 dB is plenty for
 * bells, and a lookup is far cheaper than the polynomial in the inner loop) */
static int16_t snd_sin_tab[2048];
static uint8_t snd_sin_ok;
static void snd_sin_init(void)
{
    uint32_t i;
    for (i = 0; i < 2048u; i++) snd_sin_tab[i] = (int16_t)snd_sine(i << 21);
    snd_sin_ok = 1;
}
#define SND_SIN(ph) ((int32_t)snd_sin_tab[(ph) >> 21])

/* ---- the instruments ----------------------------------------------------------------------------------- */
/* a decay per control tick for a T60 of t seconds, Q16 (folded at compile time: exp(-x) to the cubic term, x small) */
#define SND_X(t) (6.907755 * 32.0 / 44100.0 / (t))
#define DEC(t) ((uint16_t)(65535.0 * (1.0 - SND_X(t) + SND_X(t) * SND_X(t) / 2.0 - SND_X(t) * SND_X(t) * SND_X(t) / 6.0)))
#define RAT(r) ((uint16_t)((r) * 256.0 + 0.5))  /* a partial's frequency, times the note's, Q8 */
#define AMP(a) ((uint16_t)((a) * 32767.0 + 0.5))
typedef struct { uint16_t ratio, amp, dec; } snd_part_t;
typedef struct {
    uint8_t np;                                 /* partials used */
    uint8_t attack;                             /* control ticks to full level (0: struck) */
    uint8_t click;                              /* the strike's noise, /255 */
    uint8_t rev;                                /* how much goes to the cathedral, /255 */
    uint16_t level;                             /* the voice's peak at full velocity, Q15 */
    uint16_t fm_ratio, fm_idx, fm_dec;          /* partial 0 bent by a modulator: its ratio (Q8), index (1304 = 1 rad), decay */
    snd_part_t p[4];
} snd_inst_t;
enum { TI_HANDBELL, TI_CHURCH, TI_CELESTA, TI_MUSICBOX, TI_GLOCK, TI_SOFTBASS, TI_PAD, TI_CHIME, TI_TOY, TI_DEEPBASS, TN_NINST };
static const snd_inst_t SND_INST[TN_NINST] = {
    /* handbell: the fundamental with a twelfth over it [RESEARCH 5] */
    {3, 0, 40, 70, 7000, 0, 0, 0, {{RAT(1.0), AMP(1.0), DEC(3.0)}, {RAT(3.0), AMP(0.4), DEC(1.5)}, {RAT(4.2), AMP(0.12), DEC(0.3)}, {0, 0, 0}}},
    /* church bell: hum, prime, tierce, nominal */
    {4, 0, 30, 140, 6000, 0, 0, 0, {{RAT(0.5), AMP(0.5), DEC(6.0)}, {RAT(1.0), AMP(0.7), DEC(4.0)}, {RAT(1.183), AMP(0.8), DEC(3.0)}, {RAT(2.0), AMP(1.0), DEC(2.5)}}},
    /* celesta: a nearly pure tone, a soft FM attack, a little ping on top */
    {2, 0, 25, 90, 7500, RAT(1.0), 1043, DEC(0.06), {{RAT(1.0), AMP(1.0), DEC(1.5)}, {RAT(4.0), AMP(0.1), DEC(0.03)}, {0, 0, 0}, {0, 0, 0}}},
    /* music box: a tine (cantilever modes 1, 6.27, 17.55) */
    {3, 0, 60, 80, 7500, 0, 0, 0, {{RAT(1.0), AMP(1.0), DEC(2.0)}, {RAT(6.27), AMP(0.2), DEC(0.08)}, {RAT(17.55), AMP(0.05), DEC(0.02)}, {0, 0, 0}}},
    /* glockenspiel: bar modes 1, 2.76, 5.40 */
    {3, 0, 70, 80, 7000, 0, 0, 0, {{RAT(1.0), AMP(1.0), DEC(1.5)}, {RAT(2.76), AMP(0.3), DEC(0.25)}, {RAT(5.40), AMP(0.15), DEC(0.08)}, {0, 0, 0}}},
    /* soft bass: its harmonics carry it on a small speaker (the ear hears the missing fundamental) */
    {4, 8, 0, 20, 6500, 0, 0, 0, {{RAT(1.0), AMP(0.5), DEC(1.2)}, {RAT(2.0), AMP(0.8), DEC(1.0)}, {RAT(3.0), AMP(0.5), DEC(0.7)}, {RAT(4.0), AMP(0.3), DEC(0.5)}}},
    /* pad: slow in, long out */
    {3, 160, 0, 110, 3800, 0, 0, 0, {{RAT(1.0), AMP(0.7), DEC(2.5)}, {RAT(2.0), AMP(0.3), DEC(2.0)}, {RAT(3.0), AMP(0.12), DEC(1.5)}, {0, 0, 0}}},
    /* chime: a sparkle */
    {2, 0, 20, 160, 5000, 0, 0, 0, {{RAT(1.0), AMP(1.0), DEC(0.35)}, {RAT(3.5), AMP(0.3), DEC(0.06)}, {0, 0, 0}, {0, 0, 0}}},
    /* toy: a bright FM blip for pops and boings */
    {1, 0, 10, 60, 7000, RAT(2.0), 2600, DEC(0.04), {{RAT(1.0), AMP(1.0), DEC(0.25)}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
    /* deep bass: the soft bass for headphones, its fundamental at the front (snd_start swaps it in) */
    {3, 8, 0, 20, 6500, 0, 0, 0, {{RAT(1.0), AMP(1.0), DEC(1.2)}, {RAT(2.0), AMP(0.4), DEC(0.9)}, {RAT(3.0), AMP(0.15), DEC(0.6)}, {0, 0, 0}}},
};

typedef struct {
    uint32_t ph[4], inc[4], amp[4];             /* amp: Q30 (the level times the velocity) */
    int32_t a[4], da[4];                        /* the per-sample level (Q15 << 8) and its step across a control tick */
    uint32_t mph, minc, midx;                   /* FM: the modulator's phase and step; its index, Q16 of fm_idx */
    int32_t click, cl_last, gain, dgain, gl, gr;
    uint16_t att;                               /* attack ticks left */
    uint8_t inst, on, hush;
    uint32_t age;
} snd_voice_t;
static snd_voice_t snd_v[TN_VOICES];
static uint32_t snd_rng = 0x2F6E2B1u, snd_ages, snd_ctl_count;
static volatile uint32_t snd_phones;            /* headphones in: the bass plays its real low notes (the main loop sets it) */

static uint32_t snd_rand(void) { snd_rng ^= snd_rng << 13; snd_rng ^= snd_rng >> 17; snd_rng ^= snd_rng << 5; return snd_rng; }

/* x * d / 65536 for x up to 2^31, d up to 65535, in 32 bits */
static uint32_t snd_mul16(uint32_t x, uint32_t d) { return (x >> 16) * d + (((x & 0xFFFFu) * d) >> 16); }

static void snd_start(uint32_t inst, uint32_t midi, uint32_t vel, int32_t pan)
{
    const snd_inst_t *in;
    snd_voice_t *v = 0;
    if (inst == TI_SOFTBASS && snd_phones) inst = TI_DEEPBASS;   /* (the speaker plays nothing below ~100 Hz: harmonics there) */
    in = &SND_INST[inst % TN_NINST];
    uint32_t i, base = snd_inc(midi), oldest = 0, vq;
    for (i = 0; i < TN_VOICES; i++)             /* a free voice, else the quietest of the oldest half */
        if (!snd_v[i].on) { v = &snd_v[i]; break; }
    if (!v) {
        uint32_t best = 0xFFFFFFFFu;
        for (i = 0; i < TN_VOICES; i++) {
            uint32_t lvl = snd_v[i].amp[0] >> 8, age = snd_ages - snd_v[i].age;
            if (age > oldest) oldest = age;
            if (age >= 4u && lvl < best) { best = lvl; v = &snd_v[i]; }
        }
        if (!v) v = &snd_v[0];
    }
    if (vel > 127u) vel = 127u;
    vq = (uint32_t)in->level * (vel + 1u) >> 7;  /* Q15 */
    for (i = 0; i < 4u; i++) {
        uint32_t r = i < in->np ? in->p[i].ratio : 0u;
        v->ph[i] = 0;
        v->inc[i] = (base >> 8) * r;
        if (v->inc[i] > 0x7FFFFFFFu) v->inc[i] = 0;   /* above half the sample rate: silent */
        v->amp[i] = i < in->np ? (vq * in->p[i].amp >> 15) << 15 : 0u;
        v->a[i] = v->da[i] = 0;
    }
    v->minc = in->fm_ratio ? (base >> 8) * in->fm_ratio : 0u;
    v->mph = 0;
    v->midx = (uint32_t)in->fm_idx << 16;
    v->click = (int32_t)in->click * (int32_t)(vel + 1u) * 2;
    v->cl_last = 0;
    v->att = in->attack;
    v->gain = in->attack ? 0 : 32767 << 8;
    v->dgain = 0;
    if (pan < -64) pan = -64;
    if (pan > 64) pan = 64;
    v->gl = 64 - pan / 2;                       /* gentle: never fully one side */
    v->gr = 64 + pan / 2;
    v->inst = (uint8_t)(inst % TN_NINST);
    v->hush = 0;
    v->age = ++snd_ages;
    v->on = 1;
}

/* every TN_CTL frames: each partial's level falls by its decay; the per-sample step towards it is set */
static void snd_control(snd_voice_t *v)
{
    const snd_inst_t *in = &SND_INST[v->inst];
    uint32_t i, live = 0;
    for (i = 0; i < in->np; i++) {
        int32_t target;
        v->amp[i] = v->hush ? snd_mul16(v->amp[i], 52000u) : snd_mul16(v->amp[i], in->p[i].dec);
        if (v->amp[i] < (1u << 15)) v->amp[i] = 0;   /* below one step of Q15: done */
        target = (int32_t)(v->amp[i] >> 7);      /* Q30 -> Q15 << 8 */
        v->da[i] = (target - v->a[i]) / (int32_t)TN_CTL;
        live |= v->amp[i];
    }
    if (in->fm_ratio) v->midx = snd_mul16(v->midx, in->fm_dec);
    if (v->att) {
        int32_t target = (int32_t)(in->attack - v->att + 1u) * (32767 << 8) / in->attack;
        v->dgain = (target - v->gain) / (int32_t)TN_CTL;
        v->att--;
    } else {
        v->dgain = 0;
        v->gain = 32767 << 8;
    }
    if (!live) v->on = 0;
}

/* ---- sleigh bells (PhISEM, after STK's Shakers.cpp "Sleigh Bells") -------------------------------------- */
static const uint16_t SB_FREQ[5] = {2500, 5300, 6500, 8300, 9800};
static const uint16_t SB_GAIN[5] = {8192, 8192, 8192, 4096, 2458};   /* 1, 1, 1, 0.5, 0.3 (Q13) */
static struct {
    int32_t energy, snd;                        /* Q15 << 8 */
    int32_t a1[5], y1[5], y2[5], x1, x2;
} snd_sb;
#define SB_A2 8029                              /* r^2, r = 0.99 (Q13) */
#define SB_YMAX 131071                          /* (each product below stays inside 32 bits) */
static void sb_tune(uint32_t k)                 /* a resonator re-picked within +-3 % (STK's frequency variation) */
{
    int32_t jit = (int32_t)(snd_rand() % 61u) - 30;   /* -30..30 /1000 */
    uint32_t f = SB_FREQ[k] + (uint32_t)((int32_t)SB_FREQ[k] * jit / 1000);
    uint32_t inc = f * 97391u;                   /* f * 2^32 / 44100 (97391.5 a hertz) */
    int32_t cs = snd_sine(inc + 0x40000000u);   /* cos(2 pi f / fs), Q15 */
    snd_sb.a1[k] = -(2 * 8110 * cs) >> 15;       /* -2 r cos, Q13 (r = 0.99: 8110) */
}
static int32_t sb_render(void)                  /* one sample (Q15), or 0 when still */
{
    int32_t x, y, out = 0;
    uint32_t k;
    if (snd_sb.energy < 64 && snd_sb.snd < 64) return 0;
    snd_sb.energy -= (snd_sb.energy >> 11) + (snd_sb.energy > 0 ? 1 : 0);   /* x 0.9995: STK 0.9994 */
    snd_sb.snd -= snd_sb.snd >> 5;               /* x 0.969: STK 0.97 */
    if (snd_rand() % 1024u < 32u) {              /* a jingle strikes */
        snd_sb.snd += snd_sb.energy;
        if (snd_sb.snd > (32767 << 8)) snd_sb.snd = 32767 << 8;
        sb_tune(snd_rand() % 5u);
    }
    x = ((snd_sb.snd >> 8) * ((int32_t)(snd_rand() & 0xFFFFu) - 32768)) >> 17;   /* the excitation, Q13 */
    for (k = 0; k < 5u; k++) {
        y = ((x * SB_GAIN[k]) >> 13) - ((snd_sb.a1[k] * snd_sb.y1[k]) >> 13) - ((SB_A2 * snd_sb.y2[k]) >> 13);
        if (y > SB_YMAX) y = SB_YMAX;
        if (y < -SB_YMAX) y = -SB_YMAX;
        snd_sb.y2[k] = snd_sb.y1[k];
        snd_sb.y1[k] = y;
        out += y;
    }
    y = out - snd_sb.x2;                         /* STK's equaliser: x[n] - x[n-2] */
    snd_sb.x2 = snd_sb.x1;
    snd_sb.x1 = out;
    return y >> 3;
}

/* ---- the cathedral: four delay lines, a Hadamard mix, damping ------------------------------------------- */
#define RV_L0 1931u
#define RV_L1 2467u
#define RV_L2 2971u
#define RV_L3 3533u
static int16_t rv_buf[RV_L0 + RV_L1 + RV_L2 + RV_L3] TN_POOL;
static const uint16_t RV_LEN[4] = {RV_L0, RV_L1, RV_L2, RV_L3};
static const uint16_t RV_OFF[4] = {0, RV_L0, RV_L0 + RV_L1, RV_L0 + RV_L1 + RV_L2};
static const int16_t RV_G[4] = {29360, 28366, 27371, 26276};   /* about 4 s (10^(-3 len / (fs 4)) Q15) */
static uint16_t rv_pos[4];
static int32_t rv_lp[4], rv_hp;

/* ---- the main loop's requests --------------------------------------------------------------------------- */
enum { RQ_PING = 1, RQ_SHAKE, RQ_HUSH };
typedef struct { uint8_t kind, inst, midi, vel; int8_t pan; uint8_t pad; uint16_t delay; } snd_req_t;
static snd_req_t snd_q[64];
static volatile uint32_t snd_qw, snd_qr;        /* main writes snd_qw, the interrupt snd_qr */
#define SND_LATER 24u
static snd_req_t snd_later[SND_LATER];          /* the interrupt's: pings waiting for their time */
static uint32_t snd_later_at[SND_LATER];
static uint32_t snd_frames;                     /* frames rendered (the interrupt's clock) */
static int32_t snd_mix[256][3];                 /* the block: left, right, what goes to the cathedral */
static volatile uint32_t snd_ceiling = 16384, snd_rev = 160;
static int32_t snd_lim = 32768;                 /* the limiter's gain, Q15 */
static volatile uint32_t snd_peak;              /* the last block's loudest sample after the ceiling (tests, the panel) */

static void snd_put(uint32_t kind, uint32_t inst, uint32_t midi, uint32_t vel, int32_t pan, uint32_t delay_ms)
{
    uint32_t w = snd_qw;
    snd_req_t *r;
    if (w - snd_qr >= 64u) return;              /* full: drop it (a toddler pressing everything) */
    r = &snd_q[w % 64u];
    r->kind = (uint8_t)kind; r->inst = (uint8_t)inst; r->midi = (uint8_t)midi; r->vel = (uint8_t)vel;
    r->pan = (int8_t)pan; r->delay = (uint16_t)(delay_ms > 60000u ? 60000u : delay_ms);
    snd_qw = w + 1u;
}
static void snd_ping(uint32_t inst, uint32_t midi, uint32_t vel, int32_t pan) { snd_put(RQ_PING, inst, midi, vel, pan, 0); }
static void snd_ping_at(uint32_t ms, uint32_t inst, uint32_t midi, uint32_t vel, int32_t pan) { snd_put(RQ_PING, inst, midi, vel, pan, ms); }
static void snd_shake(uint32_t strength) { snd_put(RQ_SHAKE, 0, 0, strength, 0, 0); }
static void snd_shake_at(uint32_t ms, uint32_t strength) { snd_put(RQ_SHAKE, 0, 0, strength, 0, ms); }
static void snd_hush(void) { snd_put(RQ_HUSH, 0, 0, 0, 0, 0); }
static void snd_set_ceiling(uint32_t q15) { snd_ceiling = q15 > 32767u ? 32767u : q15 < 2000u ? 2000u : q15; }
static void snd_set_reverb(uint32_t amount) { snd_rev = amount > 255u ? 255u : amount; }

enum { SFX_FANFARE, SFX_SPARKLE, SFX_POP, SFX_CHEER, SFX_DOOR, SFX_BOING, SFX_TWINKLE, SFX_NOPE, TN_NSFX };
static void snd_fx(uint32_t which)
{
    static const uint8_t FAN[] = {67, 72, 76, 79, 76, 79, 84};
    static const uint8_t SPARK[] = {88, 91, 93, 96, 98, 100};
    uint32_t i;
    switch (which) {
    case SFX_FANFARE:                           /* a bell fanfare and a shake */
        for (i = 0; i < sizeof FAN; i++) snd_ping_at(i * 110u + (i == 6u ? 120u : 0u), TI_HANDBELL, FAN[i], 110, (int32_t)i * 16 - 48);
        snd_shake(127); snd_shake_at(440, 90); snd_shake_at(880, 127);
        break;
    case SFX_SPARKLE:
        for (i = 0; i < sizeof SPARK; i++) snd_ping_at(i * 45u, TI_CHIME, SPARK[(i * 5u) % sizeof SPARK], 70, (int32_t)(i * 37u % 129u) - 64);
        break;
    case SFX_POP: snd_ping(TI_TOY, 84, 100, 0); snd_ping_at(40, TI_CHIME, 96, 60, 20); break;
    case SFX_CHEER:
        for (i = 0; i < 4u; i++) snd_ping_at(i * 70u, TI_GLOCK, FAN[i + 1u] + 12u, 90, (int32_t)i * 30 - 45);
        snd_shake_at(100, 80);
        break;
    case SFX_DOOR: snd_ping(TI_CELESTA, 79, 100, -20); snd_ping_at(150, TI_CELESTA, 84, 100, 20); snd_ping_at(300, TI_CHIME, 91, 70, 0); break;
    case SFX_BOING: snd_ping(TI_TOY, 60, 100, 0); snd_ping_at(70, TI_TOY, 67, 80, 0); break;
    case SFX_TWINKLE: snd_ping(TI_CHIME, 96, 60, -30); snd_ping_at(80, TI_CHIME, 100, 50, 30); break;
    case SFX_NOPE: snd_ping(TI_GLOCK, 70, 50, 0); break;   /* (Elf and up: soft, in tune, never a buzzer) */
    }
}

static void snd_take(const snd_req_t *r)
{
    if (r->kind == RQ_PING) snd_start(r->inst, r->midi, r->vel, r->pan);
    else if (r->kind == RQ_SHAKE) {
        int32_t e = snd_sb.energy + (int32_t)(r->vel + 1u) * (32767 / 128 * 256 / 10) * 3;   /* STK: amp x 0.1 (louder here: 3x) */
        snd_sb.energy = e > (32767 << 8) ? 32767 << 8 : e;
    }
}

static void snd_render(int32_t *out, uint32_t n)
{
    uint32_t i, k;
    int32_t peak = 0, g, g0 = snd_lim, ceil = (int32_t)snd_ceiling;
    while (snd_qr != snd_qw) {                  /* the main loop's requests: now, or later */
        snd_req_t r = snd_q[snd_qr % 64u];
        snd_qr++;
        if (r.kind == RQ_HUSH) {
            for (k = 0; k < TN_VOICES; k++) snd_v[k].hush = 1;
            for (k = 0; k < SND_LATER; k++) snd_later[k].kind = 0;
            snd_sb.energy = 0;
            snd_sb.snd >>= 2;
        } else if (r.delay) {
            for (k = 0; k < SND_LATER; k++)
                if (!snd_later[k].kind) { snd_later[k] = r; snd_later_at[k] = snd_frames + r.delay * 441u / 10u; break; }
        } else
            snd_take(&r);
    }
    for (k = 0; k < SND_LATER; k++)             /* pings whose time has come (to the block) */
        if (snd_later[k].kind && (int32_t)(snd_frames + n - snd_later_at[k]) >= 0) { snd_take(&snd_later[k]); snd_later[k].kind = 0; }

    if (n > 256u) n = 256u;
    if (!snd_sin_ok) snd_sin_init();
    memset(snd_mix, 0, sizeof snd_mix);
    for (k = 0; k < TN_VOICES; k++) {            /* each voice through the block, a control tick at a time */
        snd_voice_t *v = &snd_v[k];
        uint32_t c0;
        for (c0 = 0; c0 < n && v->on; c0 += TN_CTL) {
            const snd_inst_t *in = &SND_INST[v->inst];
            uint32_t np = in->np, c1 = c0 + TN_CTL > n ? n : c0 + TN_CTL;
            int32_t gl = v->gl, gr = v->gr, rv = in->rev;
            int32_t *mx = &snd_mix[0][0];
            snd_control(v);
            {
                uint32_t ph0 = v->ph[0], ph1 = v->ph[1], ph2 = v->ph[2], ph3 = v->ph[3];
                uint32_t in0 = v->inc[0], in1 = v->inc[1], in2 = v->inc[2], in3 = v->inc[3];
                int32_t a0 = v->a[0], a1 = v->a[1], a2 = v->a[2], a3 = v->a[3];
                int32_t d0 = v->da[0], d1 = v->da[1], d2 = v->da[2], d3 = v->da[3];
                uint32_t mph = v->mph, minc = v->minc, midx = v->midx >> 16;
                int32_t gain = v->gain, dgain = v->dgain, click = v->click;
                for (i = c0; i < c1; i++) {
                    int32_t acc, s;
                    uint32_t ph = ph0;
                    if (minc) { mph += minc; ph += (uint32_t)(SND_SIN(mph) * (int32_t)midx) << 4; }   /* FM bends partial 0 */
                    a0 += d0; acc = (SND_SIN(ph) * (a0 >> 8)) >> 15; ph0 += in0;
                    if (np > 1u) { a1 += d1; acc += (SND_SIN(ph1) * (a1 >> 8)) >> 15; ph1 += in1; }
                    if (np > 2u) { a2 += d2; acc += (SND_SIN(ph2) * (a2 >> 8)) >> 15; ph2 += in2; }
                    if (np > 3u) { a3 += d3; acc += (SND_SIN(ph3) * (a3 >> 8)) >> 15; ph3 += in3; }
                    if (click > 0) {                 /* the strike: noise, its highs (a difference), falling fast */
                        int32_t nz = (int32_t)(snd_rand() & 0xFFFFu) - 32768, hp = nz - v->cl_last;
                        v->cl_last = nz;
                        acc += (hp * (click >> 3)) >> 15;
                        click -= (click >> 5) + 1;
                    }
                    if (dgain) { gain += dgain; s = (acc * (gain >> 8)) >> 15; } else s = acc;
                    mx[3u * i] += s * gl >> 6;
                    mx[3u * i + 1u] += s * gr >> 6;
                    mx[3u * i + 2u] += s * rv >> 8;
                }
                v->ph[0] = ph0; v->ph[1] = ph1; v->ph[2] = ph2; v->ph[3] = ph3;
                v->a[0] = a0; v->a[1] = a1; v->a[2] = a2; v->a[3] = a3;
                v->mph = mph; v->gain = gain; v->click = click;
            }
        }
    }
    for (i = 0; i < n; i++) {
        int32_t l = snd_mix[i][0], r = snd_mix[i][1], wet = snd_mix[i][2], s;
        s = sb_render();                         /* sleigh bells: a little to each side, some to the room */
        l += s; r += s; wet += s >> 2;
        {                                        /* the cathedral */
            int32_t d[4], h0, h1, h2, h3, in_ = (wet * (int32_t)snd_rev) >> 9;
            rv_hp += ((in_ << 8) - rv_hp) >> 6;  /* keep its lows out (the speaker wastes them); Q8, so it settles at 0 */
            in_ -= rv_hp >> 8;
            for (k = 0; k < 4u; k++) {
                d[k] = rv_buf[RV_OFF[k] + rv_pos[k]];
                rv_lp[k] += ((d[k] << 8) - rv_lp[k]) >> 2;   /* damping (Q8) */
                d[k] = rv_lp[k] >> 8;
            }
            h0 = (d[0] + d[1] + d[2] + d[3]) >> 1; h1 = (d[0] - d[1] + d[2] - d[3]) >> 1;
            h2 = (d[0] + d[1] - d[2] - d[3]) >> 1; h3 = (d[0] - d[1] - d[2] + d[3]) >> 1;
            {
                int32_t w[4];
                w[0] = ((h0 * RV_G[0] + 16384) >> 15) + in_; w[1] = ((h1 * RV_G[1] + 16384) >> 15) + in_;
                w[2] = ((h2 * RV_G[2] + 16384) >> 15) - in_; w[3] = ((h3 * RV_G[3] + 16384) >> 15) - in_;
                for (k = 0; k < 4u; k++) {
                    int32_t x = w[k] > 32767 ? 32767 : w[k] < -32767 ? -32767 : w[k];
                    if (x >= -2 && x <= 2) x = 0;    /* no limit cycle: the tail ends in silence, not a hum */
                    rv_buf[RV_OFF[k] + rv_pos[k]] = (int16_t)x;
                    if (++rv_pos[k] >= RV_LEN[k]) rv_pos[k] = 0;
                }
            }
            l += (d[0] + d[2]) >> 1;
            r += (d[1] + d[3]) >> 1;
        }
        out[2u * i] += l;                        /* (the limiter goes over the block below) */
        out[2u * i + 1u] += r;
        if (l < 0) l = -l;
        if (r < 0) r = -r;
        if (l > peak) peak = l;
        if (r > peak) peak = r;
    }
    snd_frames += n;
    /* the ceiling: the block's gain, so the loudest sample sits at the ceiling; recovering slowly (about 0.3 s) */
    g = snd_lim;
    if (peak > (1 << 20)) peak = 1 << 20;
    if (peak > 0 && (((peak >> 4) * (g >> 4)) >> 7) > ceil) g = (int32_t)((uint32_t)ceil * 2048u / (uint32_t)peak) << 4;
    else if (g < 32768) g += (32768 - g) >> 4 ? (32768 - g) >> 4 : 1;
    if (g > 32768) g = 32768;
    snd_lim = g;
    peak = 0;
    for (i = 0; i < 2u * n; i++) {               /* the gain glides from the last block's to this one's */
        int32_t gi = g0 + (g - g0) * (int32_t)(i >> 1) / (int32_t)n, x = out[i];
        if (x > (1 << 20)) x = 1 << 20;
        if (x < -(1 << 20)) x = -(1 << 20);
        if (x > -24 && x < 24) x = 0;            /* the tail ends in true silence (shifts round negatives down: a hum of -1s) */
        else if (x < 0) x = -((-x >> 4) * (gi >> 4) >> 7);
        else x = (x >> 4) * (gi >> 4) >> 7;      /* x g / 32768, in 32 bits, the same both ways */
        if (x > ceil) x = ceil;
        if (x < -ceil) x = -ceil;
        out[i] = x;
        if (x > peak) peak = x;
        if (-x > peak) peak = -x;
    }
    snd_peak = (uint32_t)peak;
}

static uint32_t snd_active(void)                /* voices sounding (tests, the idle logic) */
{
    uint32_t k, n = 0;
    for (k = 0; k < TN_VOICES; k++) n += snd_v[k].on;
    return n + (snd_sb.energy > 64);
}
#endif
