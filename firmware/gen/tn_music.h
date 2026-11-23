/* SPDX-License-Identifier: GPL-3.0-only */
/* Tinsel's carols. Made by tools/gen_music.py from the files in music/: change those, not this. */
#ifndef TN_MUSIC_H
#define TN_MUSIC_H
typedef struct { const char *title; const uint8_t *pitch, *len, *chord; uint16_t n, nchord, tempo;
                 uint8_t bar, pickup, picture, six8, jolly; } tn_carol_t;
enum { CP_BELLS, CP_PUDDING, CP_HOLLY, CP_STAR, CP_CROWN, CP_ANGEL, CP_CANDLE, CP_BELL };
/* Jingle Bells: James Lord Pierpont, "One Horse Open Sleigh" (Boston: Oliver Ditson, 1857); the chorus. Transcribed by ear for Tinsel; to check against the 1857 print. */
static const uint8_t TNM_P0[] = {71, 71, 71, 71, 71, 71, 71, 74, 67, 69, 71, 72, 72, 72, 72, 72, 71, 71, 71, 71, 71, 69, 69, 71, 69, 74, 71, 71, 71, 71, 71, 71, 71, 74, 67, 69, 71, 72, 72, 72, 72, 72, 71, 71, 71, 71, 74, 74, 72, 69, 67};
static const uint8_t TNM_L0[] = {4, 4, 8, 4, 4, 8, 4, 4, 6, 2, 16, 4, 4, 6, 2, 4, 4, 4, 2, 2, 4, 4, 4, 4, 8, 8, 4, 4, 8, 4, 4, 8, 4, 4, 6, 2, 16, 4, 4, 6, 2, 4, 4, 4, 2, 2, 4, 4, 4, 4, 16};
static const uint8_t TNM_C0[] = {7, 16, 7, 16, 7, 16, 7, 16, 0, 16, 7, 16, 41, 16, 34, 16, 7, 16, 7, 16, 7, 16, 7, 16, 0, 16, 7, 16, 34, 16, 7, 16};
/* We Wish You a Merry Christmas: West Country folk carol, traditional. Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P1[] = {62, 67, 67, 69, 67, 66, 64, 64, 64, 69, 69, 71, 69, 67, 66, 62, 62, 71, 71, 72, 71, 69, 67, 64, 62, 62, 64, 69, 66, 67};
static const uint8_t TNM_L1[] = {4, 4, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2, 4, 4, 2, 2, 4, 4, 4, 8};
static const uint8_t TNM_C1[] = {2, 4, 7, 12, 0, 12, 41, 12, 2, 12, 43, 12, 20, 12, 0, 4, 34, 8, 7, 8};
/* Deck the Halls: Welsh tune "Nos Galan", traditional (printed by John Parry, 1794). Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P2[] = {74, 72, 71, 69, 67, 69, 71, 67, 69, 71, 72, 69, 71, 69, 67, 66, 67, 74, 72, 71, 69, 67, 69, 71, 67, 69, 71, 72, 69, 71, 69, 67, 66, 67, 69, 71, 72, 69, 71, 72, 74, 69, 71, 73, 74, 76, 78, 79, 78, 76, 74, 74, 72, 71, 69, 67, 69, 71, 67, 76, 76, 76, 76, 74, 72, 71, 69, 67};
static const uint8_t TNM_L2[] = {6, 2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 6, 2, 4, 4, 8, 6, 2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 6, 2, 4, 4, 8, 6, 2, 4, 4, 6, 2, 4, 4, 2, 2, 4, 2, 2, 4, 4, 4, 8, 6, 2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 6, 2, 4, 4, 8};
static const uint8_t TNM_C2[] = {7, 16, 7, 16, 34, 8, 7, 8, 34, 8, 7, 8, 7, 16, 7, 16, 34, 8, 7, 8, 34, 8, 7, 8, 2, 16, 7, 8, 2, 8, 41, 16, 2, 16, 7, 16, 7, 16, 0, 8, 7, 8, 34, 8, 7, 8};
/* Silent Night: Franz Xaver Gruber, "Stille Nacht" (1818). Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P3[] = {67, 69, 67, 64, 67, 69, 67, 64, 74, 74, 71, 72, 72, 67, 69, 69, 72, 71, 69, 67, 69, 67, 64, 69, 69, 72, 71, 69, 67, 69, 67, 64, 74, 74, 77, 74, 71, 72, 76, 72, 67, 64, 67, 65, 62, 60};
static const uint8_t TNM_L3[] = {3, 1, 2, 6, 3, 1, 2, 6, 4, 2, 6, 4, 2, 6, 4, 2, 3, 1, 2, 3, 1, 2, 6, 4, 2, 3, 1, 2, 3, 1, 2, 6, 4, 2, 3, 1, 2, 6, 6, 3, 1, 2, 3, 1, 2, 12};
static const uint8_t TNM_C3[] = {0, 12, 0, 12, 39, 12, 0, 12, 5, 12, 0, 12, 5, 12, 0, 12, 39, 12, 0, 12, 0, 6, 39, 6, 0, 12};
/* Good King Wenceslas: "Tempus adest floridum", Piae Cantiones (1582); words J. M. Neale (1853). Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P4[] = {67, 67, 67, 69, 67, 67, 62, 64, 62, 64, 66, 67, 67, 67, 67, 67, 69, 67, 67, 62, 64, 62, 64, 66, 67, 67, 74, 72, 71, 69, 71, 69, 67, 64, 62, 64, 66, 67, 67, 62, 62, 64, 66, 67, 67, 69, 74, 72, 71, 69, 67, 72, 67};
static const uint8_t TNM_L4[] = {4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 8, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 8, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 8, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 8, 8, 16};
static const uint8_t TNM_C4[] = {7, 16, 7, 8, 2, 8, 0, 8, 2, 8, 7, 16, 7, 16, 7, 8, 2, 8, 0, 8, 2, 8, 7, 16, 7, 8, 2, 8, 7, 16, 20, 8, 2, 8, 7, 16, 7, 8, 2, 8, 20, 8, 2, 8, 7, 8, 2, 8, 7, 8, 0, 8, 7, 16};
/* Joy to the World: "Antioch", arranged by Lowell Mason (1839); words Isaac Watts (1719). Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P5[] = {74, 73, 71, 69, 67, 66, 64, 62, 69, 71, 71, 73, 73, 74, 74, 74, 73, 71, 69, 69, 67, 66, 74, 74, 73, 71, 69, 69, 67, 66, 66, 66, 66, 66, 66, 67, 69, 67, 66, 64, 64, 64, 64, 66, 67, 66, 64, 62, 74, 71, 69, 67, 66, 67, 66, 64, 62};
static const uint8_t TNM_L5[] = {4, 3, 1, 6, 2, 4, 4, 6, 2, 6, 2, 6, 2, 6, 2, 2, 2, 2, 2, 3, 1, 2, 2, 2, 2, 2, 2, 3, 1, 2, 2, 2, 2, 2, 1, 1, 6, 1, 1, 2, 2, 2, 1, 1, 6, 1, 1, 2, 4, 2, 3, 1, 2, 2, 4, 4, 8};
static const uint8_t TNM_C5[] = {2, 8, 7, 4, 41, 4, 2, 4, 41, 4, 2, 8, 7, 8, 9, 8, 2, 8, 2, 8, 41, 4, 2, 4, 2, 8, 41, 4, 2, 4, 2, 8, 2, 8, 41, 8, 41, 8, 2, 4, 7, 4, 2, 4, 41, 4, 2, 4, 41, 4, 2, 8};
/* God Rest Ye Merry, Gentlemen: English traditional carol (printed by William Sandys, 1833). Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P6[] = {64, 64, 71, 71, 69, 67, 66, 64, 62, 64, 66, 67, 69, 71, 64, 64, 71, 71, 69, 67, 66, 64, 62, 64, 66, 67, 69, 71, 71, 72, 69, 71, 72, 74, 76, 71, 69, 67, 64, 66, 67, 69, 67, 69, 71, 72, 71, 71, 69, 67, 66, 64, 67, 66, 64, 69, 67, 69, 71, 72, 74, 76, 71, 69, 67, 66, 64};
static const uint8_t TNM_L6[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, 4, 8, 4, 4, 4, 4, 4, 4, 8, 2, 2, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12};
static const uint8_t TNM_C6[] = {20, 4, 20, 16, 20, 8, 43, 8, 20, 8, 2, 8, 20, 16, 20, 16, 20, 8, 43, 8, 20, 8, 2, 8, 20, 16, 25, 8, 20, 8, 7, 8, 43, 8, 20, 8, 2, 8, 2, 16, 20, 8, 25, 8, 20, 8, 43, 8, 20, 16, 2, 16, 7, 8, 0, 8, 20, 8, 43, 8, 20, 12};
/* Shchedryk (Carol of the Bells): Mykola Leontovych, "Shchedryk" (1916), the ostinato; not Wilhousky's 1936 English version. Transcribed by ear for Tinsel; to check against a pre-1930 print. */
static const uint8_t TNM_P7[] = {74, 72, 74, 70, 74, 72, 74, 70, 74, 72, 74, 70, 74, 72, 74, 70, 77, 75, 77, 74, 77, 75, 77, 74, 77, 75, 77, 74, 77, 75, 77, 74, 74, 72, 74, 70, 74, 72, 74, 70, 74, 72, 74, 70, 74, 72, 74, 70};
static const uint8_t TNM_L7[] = {4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4, 4, 2, 2, 4};
static const uint8_t TNM_C7[] = {23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12, 23, 12};
static const tn_carol_t TN_CAROLS[] = {
    {"Jingle Bells", TNM_P0, TNM_L0, TNM_C0, 51, 16, 112, 16, 0, 0, 0, 1},
    {"We Wish You a Merry Christmas", TNM_P1, TNM_L1, TNM_C1, 30, 10, 150, 12, 4, 1, 0, 1},
    {"Deck the Halls", TNM_P2, TNM_L2, TNM_C2, 68, 23, 116, 16, 0, 2, 0, 1},
    {"Silent Night", TNM_P3, TNM_L3, TNM_C3, 46, 13, 50, 12, 0, 3, 1, 0},
    {"Good King Wenceslas", TNM_P4, TNM_L4, TNM_C4, 53, 27, 104, 16, 0, 4, 0, 0},
    {"Joy to the World", TNM_P5, TNM_L5, TNM_C5, 57, 26, 92, 8, 0, 5, 0, 1},
    {"God Rest Ye Merry, Gentlemen", TNM_P6, TNM_L6, TNM_C6, 67, 31, 112, 16, 4, 6, 0, 0},
    {"Shchedryk (Carol of the Bells)", TNM_P7, TNM_L7, TNM_C7, 48, 12, 120, 12, 0, 7, 0, 0},
};
#define TN_NCAROLS 8u
#endif
