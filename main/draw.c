#include "draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cjk.h"
#include "gfx.h"
#include "tower.h"
#include "ui.h"

#define S (&g_tw)
#define U (&g_ui)
#define WORLD_W 288                    // x 0..287: gutter + tower; 288..319: minimap

static uint16_t C(uint32_t rgb) { return gfx_rgb((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb); }
static bool s_nt;                      // night: rooms dim, lights glow
static int s_dusk;                     // 0..256 how dark the sky is
static const char *s_ghost_why;
static int s_blink;
static uint16_t s_shade;               // night tint colour

/* ------------------------------------------------------------ text */
static int cjk_find(uint32_t cp)
{
    int lo = 0, hi = g_cjk_n - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        if (g_cjk_cp[m] == cp) return m;
        if (g_cjk_cp[m] < cp) lo = m + 1; else hi = m - 1;
    }
    return -1;
}
static uint32_t utf8_next(const char **p)
{
    const uint8_t *s = (const uint8_t *)*p;
    uint32_t c = *s++;
    if (c >= 0xe0 && s[0] && s[1]) { c = (c & 15) << 12 | (s[0] & 63) << 6 | (s[1] & 63); s += 2; }
    else if (c >= 0xc0 && s[0]) { c = (c & 31) << 6 | (s[0] & 63); s += 1; }
    *p = (const char *)s;
    return c;
}
int cjk_text(const char *t, int x, int y, int size, uint16_t col)
{
    int x0 = x;
    int by0 = gfx_band_y0(), by1 = by0 + gfx_band_rows();
    while (*t) {
        uint32_t cp = utf8_next(&t);
        if (cp < 128) {
            char b[2] = {(char)cp, 0};
            if (cp != ' ') gfx_text(b, x, y + (size - 7) / 2, 1, col);
            x += size == 12 ? 6 : 5;
            continue;
        }
        int gi = cjk_find(cp);
        if (gi >= 0 && y + size > by0 && y < by1) {
            const uint16_t *bits = g_cjk_bits[gi];
            for (int r = 0; r < size; r++) {
                uint16_t row = bits[r];
                int run = -1;
                for (int c = 0; c <= size; c++) {
                    bool on = c < size && (row >> (15 - c)) & 1;
                    if (on && run < 0) run = c;
                    if (!on && run >= 0) { gfx_fill(x + run, y + r, c - run, 1, col); run = -1; }
                }
            }
        }
        x += size + (size == 12 ? 1 : 0);
    }
    return x - x0;
}
int cjk_text_w(const char *t, int size)
{
    int w = 0;
    while (*t) { uint32_t cp = utf8_next(&t); w += cp < 128 ? (size == 12 ? 6 : 5) : size + (size == 12 ? 1 : 0); }
    return w;
}
static void textc(const char *t, int cx, int y, uint16_t col) { gfx_text(t, cx - gfx_text_w(t, 1) / 2, y, 1, col); }

/* ------------------------------------------------------------ geometry */
static int yrow(int f) { return TW_TY + (U->cam + TW_VIS - 1 - f) * TW_FH; }
static int xseg(int x) { return TW_TX + x * TW_SEG; }
static uint16_t dim(uint32_t rgb) { uint16_t c = C(rgb); return s_nt ? gfx_mix(c, s_shade, 150) : c; }
static uint16_t dim565(uint16_t natural) { uint16_t c = gfx_swap(natural); return s_nt ? gfx_mix(c, s_shade, 150) : c; }
static void rect(int x, int y, int w, int h, uint16_t c) { gfx_fill(x, y, w, h, c); }
static void person(int x, int y, uint16_t body)
{
    rect(x, y, 2, 2, C(0xf0c8a0));
    rect(x - 1, y + 2, 4, 3, body);
    rect(x, y + 5, 1, 2, C(0x303040)); rect(x + 2, y + 5, 1, 2, C(0x303040));
}
static void outline(int x, int y, int w, int h, uint16_t c)
{
    rect(x, y, w, 1, c); rect(x, y + h - 1, w, 1, c); rect(x, y, 1, h, c); rect(x + w - 1, y, 1, h, c);
}

/* ------------------------------------------------------------ shops: six HK shop types */
typedef struct { const char *word; uint32_t sign, band, neon, inside, goods[3]; } shopv_t;
/* six HK shop types, told apart by their storefront art (no Chinese on signs) */
static const shopv_t SHOPS[6] = {
    {"24H", 0x1060c0, 0xff9020, 0xffb040, 0xf4f8ff, {0xe04040, 0x40a0e0, 0xf0c020}},    // convenience: fridges
    {"PHARM", 0x10a050, 0xffffff, 0x60ff90, 0xeefaf0, {0xffffff, 0xf06080, 0x60a0f0}},  // pharmacy: green cross
    {"GOLD", 0xa01010, 0xffd700, 0xffe060, 0x5a1010, {0xffd700, 0xffcc33, 0xfff0a0}},   // goldsmith: gold window
    {"BREAD", 0x8a4a20, 0xfff0d0, 0xffa050, 0xfff0d8, {0xd8902c, 0xe8b060, 0xc07020}}, // bakery: bread + cake
    {"TV", 0x202838, 0x30e0ff, 0x30e0ff, 0x283040, {0x2050ff, 0x30e0ff, 0xff60a0}},     // electronics: TV wall
    {"STYLE", 0xd02080, 0xffffff, 0xff60c0, 0xfff0f8, {0xff80c0, 0x80c0ff, 0xffe060}},  // fashion: mannequins
};
static void shutter(int x, int y, int w, int h, const char *sign)
{
    rect(x, y, w, h, dim(0x8a8a90));
    for (int i = 1; i < h; i += 2) rect(x, y + i, w, 1, dim(0x6a6a70));
    if (sign && h >= 7) { int tw = gfx_text_w(sign, 1); rect(x + w / 2 - tw / 2 - 2, y, tw + 4, h > 9 ? 9 : h, C(0xf0f0f0)); gfx_text(sign, x + w / 2 - tw / 2, y + 1, 1, C(0xc02020)); }
}
static void glow(int x, int y, int w, int h, uint16_t c)
{
    gfx_blend(x - 1, y - 1, w + 2, h + 2, c, 70);
}
static void sign_icon(int k, int x, int y)
{
    switch (k) {
    case 0: rect(x, y + 1, 5, 6, C(0xffffff)); rect(x + 1, y + 2, 3, 2, C(0x80d0ff)); break;                    // fridge
    case 1: rect(x + 2, y, 2, 7, C(0xffffff)); rect(x, y + 2, 6, 2, C(0xffffff)); break;                        // cross
    case 2: rect(x, y + 3, 6, 3, C(0xffd700)); rect(x + 1, y + 1, 4, 2, C(0xffd700)); rect(x + 2, y + 2, 1, 1, C(0xffffff)); break; // ingot
    case 3: rect(x, y + 3, 6, 3, C(0xe0a050)); rect(x + 1, y + 2, 4, 1, C(0xe0a050)); rect(x + 1, y + 4, 1, 1, C(0x8a4a20)); rect(x + 3, y + 4, 1, 1, C(0x8a4a20)); break; // bun
    case 4: rect(x, y + 1, 6, 5, C(0x30e0ff)); rect(x + 1, y + 2, 4, 3, C(0x2050ff)); rect(x + 2, y + 6, 2, 1, C(0xc0c0c0)); break; // TV
    case 5: rect(x + 1, y, 4, 2, C(0xffffff)); rect(x, y + 2, 6, 2, C(0xffffff)); rect(x + 1, y + 4, 4, 3, C(0xffffff)); break;   // dress
    }
}
static void draw_shop(const unit_t *u, int x, int y, int w)
{
    int k = u->v % 6;
    const shopv_t *v = &SHOPS[k];
    bool open = u->occ && tw_is_open(u) && !u->build;
    if (s_nt && u->occ) glow(x + 1, y + 1, w - 2, 8, C(v->neon));
    rect(x + 1, y + 1, w - 2, 8, C(v->sign));                                   // signboard: icon + tiny word
    rect(x + 1, y + 8, w - 2, 1, C(v->band));
    sign_icon(k, x + 3, y + 1);
    gfx_text(v->word, x + 11, y + 1, 1, s_nt && u->occ ? C(v->neon) : C(v->band));
    int lx = x + w - 6;                                                        // vertical lightbox
    rect(lx, y + 1, 4, 19, s_nt && u->occ ? C(v->neon) : C(v->sign));
    for (int i = 0; i < 4; i++) rect(lx + 1, y + 3 + i * 4, 2, 2, u->occ ? C(v->band) : C(0xbbbbbb));
    if (s_nt && u->occ) rect(lx + 1, y + 2 + ((s_blink >> 2) % 17), 2, 1, C(0xffffff));
    if (!open) { shutter(x + 1, y + 9, w - 8, 14, u->occ ? NULL : "TO LET"); return; }
    int iw = w - 8, ix = x + 1, iy = y + 9;
    for (int i = 0; i < iw; i += 2) rect(ix + i, iy, 2, 2, (i / 2) & 1 ? C(0xffffff) : C(v->sign));   // awning
    iy += 2;
    rect(ix, iy, iw, 13, s_nt && k != 2 && k != 4 ? C(0xfff8d8) : C(v->inside));
    for (int i = 0; i < iw; i += 4) rect(ix + i, iy + 11, 2, 2, C(0xd0c8b8));          // floor tiles
    iy += 2;
    int gx = ix + 10, gw = iw - 11;                                             // the window art right of the counter
    switch (k) {
    case 0:                                                                    // convenience: a row of glowing drinks fridges
        for (int f = 0; f + 9 <= gw; f += 10) {
            rect(gx + f, iy, 9, 8, C(0xc0c8d0)); rect(gx + f + 1, iy + 1, 7, 6, C(0xd8f4ff));
            for (int r = 0; r < 3; r++) for (int b = 0; b < 3; b++) rect(gx + f + 2 + b * 2, iy + 1 + r * 2, 1, 2, C(v->goods[(r + b + f) % 3]));
            rect(gx + f + 4, iy, 1, 8, C(0xa0a8b0));
        }
        break;
    case 1:                                                                    // pharmacy: big green cross + white shelves
        rect(gx + 1, iy + 1, 9, 7, C(0xffffff)); rect(gx + 4, iy + 1, 3, 7, C(0x10b050)); rect(gx + 2, iy + 3, 7, 3, C(0x10b050));
        for (int r = 0; r < 2; r++) { rect(gx + 12, iy + 3 + r * 4, gw - 12, 1, C(0xc0c0c0)); for (int i = 0; i < gw - 13; i += 3) rect(gx + 12 + i, iy + 1 + r * 4, 2, 2, C(v->goods[(i / 3 + r) % 3])); }
        break;
    case 2:                                                                    // goldsmith: glass case of glittering gold
        rect(gx, iy + 1, gw, 7, C(0x301008)); rect(gx, iy + 1, gw, 1, C(0xffd700));
        for (int i = 1; i < gw - 2; i += 3) rect(gx + i, iy + 4 + (i & 1), 2, 2, C(v->goods[i % 3]));
        for (int i = 0; i < 3; i++) { int sx = gx + 2 + (int)((i * 11 + (s_blink >> 1) * 7) % (gw - 4)), sy = iy + 2 + (i * 2 + (s_blink >> 2)) % 5;
            rect(sx, sy, 1, 1, C(0xffffff)); if ((s_blink >> 1) & 1) { rect(sx - 1, sy, 3, 1, C(0xfff0a0)); rect(sx, sy - 1, 1, 3, C(0xfff0a0)); } }
        break;
    case 3:                                                                    // bakery: loaves + a pink layer cake
        rect(gx, iy + 5, gw, 1, C(0xb08050));
        for (int i = 0; i + 4 <= gw - 9; i += 5) { rect(gx + i, iy + 3, 4, 2, C(v->goods[(i / 5) % 3])); rect(gx + i + 1, iy + 2, 2, 1, C(v->goods[(i / 5) % 3])); }
        for (int i = 0; i + 3 <= gw - 9; i += 4) rect(gx + i, iy + 6, 3, 2, C(0xe8b060));
        { int cx = gx + gw - 8; rect(cx, iy + 4, 7, 4, C(0xffc0d0)); rect(cx + 1, iy + 2, 5, 2, C(0xffffff)); rect(cx + 2, iy + 1, 3, 1, C(0xff4060)); rect(cx, iy + 5, 7, 1, C(0xff80a0)); }
        break;
    case 4:                                                                    // electronics: a wall of TVs
        for (int r = 0; r < 2; r++) for (int i = 0; i + 6 <= gw; i += 7) {
            rect(gx + i, iy + r * 4, 6, 4, C(0x101010));
            rect(gx + i + 1, iy + r * 4 + 1, 4, 2, C(v->goods[(i / 7 + r + (s_blink >> 3)) % 3]));
        }
        break;
    case 5:                                                                    // fashion: mannequins in dresses
        for (int i = 1; i + 5 <= gw; i += 7) {
            rect(gx + i + 1, iy, 2, 2, C(0xe8d8c8));
            rect(gx + i, iy + 2, 4, 4, C(v->goods[(i / 7) % 3])); rect(gx + i - 1, iy + 5, 6, 2, C(v->goods[(i / 7) % 3]));
            rect(gx + i + 1, iy + 7, 1, 1, C(0x808080)); rect(gx + i + 2, iy + 7, 1, 1, C(0x808080));
        }
        break;
    }
    rect(ix + 1, iy + 4, 7, 4, C(0x7a5a3a));                                    // counter + shopkeeper
    person(ix + 3, iy, C(v->sign));
    int nc = u->cust_y > 30 ? 2 : u->cust_y / 15;
    for (int i = 0; i < nc; i++) person(gx + 2 + ((i * 13 + (int)(U->frame >> 4)) % (gw - 4)), iy + 1, C((uint32_t[]){0x3050a0, 0xa04040}[i]));
}
static void draw_food(const unit_t *u, int x, int y, int w)
{
    bool ds = u->t == T_DIMSUM, open = u->occ && tw_is_open(u) && !u->build;
    uint16_t sign = C(ds ? 0xc01818 : 0xf0e0b0), band = C(ds ? 0xf0c020 : 0x208040), neon = C(ds ? 0xff4040 : 0x40ff80);
    if (s_nt && u->occ) glow(x + 1, y + 1, w - 2, 11, neon);
    rect(x + 1, y + 1, w - 2, 11, sign); rect(x + 1, y + 1, w - 2, 1, band); rect(x + 1, y + 11, w - 2, 1, band);
    { const char *wd = ds ? "DIM SUM" : "CAFE"; int tw = gfx_text_w(wd, 1); gfx_text(wd, x + w / 2 - tw / 2, y + 3, 1, ds ? C(0xffe060) : C(0xc02020));
      rect(x + w / 2 - tw / 2 - 9, y + 4, 6, 4, ds ? C(0xe0c080) : C(0xffffff)); rect(x + w / 2 - tw / 2 - 9, y + 3, 6, 1, ds ? C(0xa08040) : C(0xc08040)); }   // steamer / cup
    int lx = x + w - 6;
    rect(lx, y + 1, 4, 15, s_nt && u->occ ? neon : sign);
    if (!open) { shutter(x + 1, y + 12, w - 8, 11, u->occ ? NULL : "TO LET"); return; }
    y += 2;
    rect(x + 1, y + 10, w - 8, 11, s_nt ? C(0xfff0c8) : C(ds ? 0xfff0e0 : 0xf8f0d8));
    for (int i = 4; i < w - 14; i += ds ? 14 : 10) {
        if (ds) rect(x + i + 3, y + 12, 3, 3, C(0xff4020));                      // lanterns
        rect(x + i, y + 16, ds ? 9 : 6, 1, ds ? C(0xc02020) : C(0x8a5a2a));
        rect(x + i + (ds ? 4 : 2), y + 17, 1, 2, C(0x555555));
        rect(x + i + 1, y + 15, 2, 1, C(0xc88040)); rect(x + i + 4, y + 15, 2, 1, C(0xffffff));
        if (u->cust_y > (i / 10) * 6) person(x + i + (ds ? 10 : 7), y + 12, C((uint32_t[]){0x3050a0, 0xa04040, 0x40a060}[(i / 10) % 3]));
    }
}
static void draw_cinema(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH;
    y -= TW_FH;                         // y given for the bottom floor
    rect(x + 1, y + 1, w - 2, h - 2, dim(0x302040));
    rect(x + 3, y + 3, w - 6, 10, C(0x101018));
    for (int i = 0; i < (w - 6) / 3; i++) rect(x + 4 + i * 3, y + 3, 1, 1, ((i + (s_blink >> 3)) & 1) ? C(0xffe060) : C(0x806020));
    gfx_text("CINEMA", x + w / 2 - 17, y + 5, 1, C(0xff4060));
    rect(x + 4, y + 16, 22, 14, C(0xe0d0b0));   // poster
    rect(x + 6, y + 18, 18, 10, C(u->v & 1 ? 0x4060c0 : 0xc04040));
    rect(x + w - 50, y + 16, 44, 16, u->occ && tw_is_open(u) ? C(0xe0e8ff) : C(0x404050));   // screen
    if (u->occ && tw_is_open(u)) rect(x + w - 50 + ((s_blink >> 2) % 40), y + 20, 4, 6, C(0x8090c0));
    for (int i = 0; i < u->cust_y / 8 && i < 8; i++) person(x + 30 + i * 6, y + 31, C(0x404060));
}


/* ------------------------------------------------------------ refuge floor, mega cinema, coaster */
static void draw_refuge(const unit_t *u, int x, int y, int w)
{
    bool first = u->x == 0 || tw_cell(u->f, u->x - 1) < 0 ? true : false;
    rect(x, y + 1, w, TW_FH - 2, dim(0x6a706c));                                     // bare concrete
    for (int i = 0; i < w; i += 8) rect(x + i, y + TW_FH - 4, 4, 2, dim(0xd8c040));   // hatched assembly area
    rect(x, y + 2, w, 2, C(0xc03030));                                                // sprinkler main (red)
    rect(x, y + 5, w, 1, dim(0x9098a0));                                              // duct
    for (int i = 6; i < w - 4; i += 16) rect(x + i, y + 4, 1, 2, C(0xc03030));        // sprinkler drops
    if (u->x == 0) { rect(x, y + 6, 3, TW_FH - 10, s_nt ? C(0x203050) : C(0x88c0f0)); for (int j = 0; j < 3; j++) rect(x, y + 9 + j * 4, 4, 1, C(0xd0d0d0)); }   // open side, rails
    if (u->x + u->w == W) { rect(x + w - 3, y + 6, 3, TW_FH - 10, s_nt ? C(0x203050) : C(0x88c0f0)); for (int j = 0; j < 3; j++) rect(x + w - 4, y + 9 + j * 4, 4, 1, C(0xd0d0d0)); }
    if (first && w >= 30) {                                                           // plant room: pumps + pipes
        rect(x + 5, y + 7, 22, TW_FH - 10, dim(0x50565c));
        gfx_ellipse(x + 11, y + 14, 4, 4, dim(0x2a70b0)); gfx_ellipse(x + 21, y + 14, 4, 4, dim(0x2a70b0));
        rect(x + 11, y + 7, 1, 4, C(0xc03030)); rect(x + 21, y + 7, 1, 4, C(0xc03030));
    }
    for (int i = (first && w >= 30) ? 46 : 6; i + 8 < w; i += 40) {                    // hose reel cabinet
        rect(x + i, y + 7, 7, 7, C(0xc02020)); gfx_ellipse(x + i + 3, y + 10, 2, 2, C(0xf0d0d0));
    }
    for (int i = 18; i + 26 < w; i += 40) {                                          // green EXIT signs (always lit)
        rect(x + i, y + 6, 26, 8, C(0x10a040)); gfx_text("EXIT", x + i + 1, y + 6, 1, C(0xe0ffe0));
        if (s_nt) gfx_blend(x + i - 2, y + 5, 30, 10, C(0x40ff80), 60);
    }
    if (w >= 50) textc("REFUGE", x + w / 2 + 8, y + 15, C(0xf0e060));
    if (s_nt) gfx_blend(x, y + 1, w, TW_FH - 2, C(0x60ff90), 24);                    // emergency lighting
}
static void draw_mega(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH;
    y -= 2 * TW_FH;                                                                    // y given for the bottom floor
    bool on = u->occ && tw_is_open(u);
    rect(x + 1, y + 1, w - 2, h - 2, C(0x180c28));
    rect(x + 2, y + 2, w - 4, 14, C(0x301040));                                        // marquee with chasing bulbs
    for (int i = 0; i < (w - 4) / 4; i++) {
        uint16_t b = ((i + (s_blink >> 2)) % 3) ? C(0xffe070) : C(0x705020);
        rect(x + 3 + i * 4, y + 2, 2, 2, b); rect(x + 3 + i * 4, y + 14, 2, 2, b);
    }
    gfx_text("MEGA", x + w / 2 - 23, y + 4, 2, C(0xff3070));
    if (s_nt) gfx_blend(x + 2, y + 2, w - 4, 14, C(0xff40a0), 40);
    int sx = x + w / 2 - 6, sw = w / 2 - 2, sy = y + 19, sh = h - 26;                  // tall screen on the right
    rect(sx - 2, sy - 2, sw + 4, sh + 4, C(0x000000));
    if (on) {
        uint32_t r = (uint32_t)(s_blink >> 1) * 2654435761u;
        uint16_t base = C((r >> 8) & 1 ? 0xb0c8f0 : 0xa0b8e0);
        rect(sx, sy, sw, sh, base);                                                    // flickering film
        int px = sx + (int)((s_blink >> 1) % (uint32_t)(sw - 14));
        rect(px, sy + sh - 22, 12, 16, C(0x405080)); rect(px + 3, sy + sh - 28, 6, 6, C(0x405080));   // hero silhouette
        rect(sx + sw - 20, sy + 6, 14, 8, C(0xf0f8ff));                                // sky / sun
        if ((r >> 12) % 5 == 0) gfx_blend(sx, sy, sw, sh, C(0xffffff), 70);            // flash cut
    } else rect(sx, sy, sw, sh, C(0x303040));
    for (int t = 0; t < 4; t++) {                                                      // tiered seats, audience
        int ty = y + 26 + t * 11, tx = x + 6 + t * 4, tw = sx - 8 - tx;
        rect(tx, ty + 6, tw, 4, C(0x701830));
        int n = on ? (u->cust_y / 6 + t * 2) % (tw / 5 + 1) + (u->cust_y ? 2 : 0) : 0;
        for (int i = 0; i < n && i * 5 + 4 < tw; i++) { rect(tx + 2 + i * 5, ty + 2, 3, 4, C(0x302838)); rect(tx + 2 + i * 5, ty, 3, 2, C(0xd0a080)); }
    }
    int by = y + 2 * TW_FH;                                                            // bottom floor: ticket booth + entrance
    rect(x + 3, by + 4, 22, TW_FH - 6, C(0xd0b060)); rect(x + 6, by + 7, 16, 6, C(0x302010));
    gfx_text("TIX", x + 7, by + 7, 1, C(0xffe080));
    person(x + 12, by + 20, C(0x803030));
    for (int i = 0; i < u->cust_y / 10 && i < 4; i++) person(x + 30 + i * 5, by + 20, C(0x404070));
}
static const int8_t SIN64[64] = {0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12};
static uint8_t s_trk_x[128], s_trk_y[128];   // coaster track, room-relative (192 x 120 box)
static void coaster_track(int W0, int H0)
{
    if (s_trk_x[127]) return;
    for (int i = 0; i < 128; i++) {
        int px, py;
        if (i < 16) { px = 10 + i * 4; py = H0 - 14 - (i >= 6 ? (i - 6) * (H0 - 26) / 9 : 0); }                // station, lift hill
        else if (i < 32) { int k = i - 16; px = 74 + k * 2; py = 12 + (H0 - 26) * (127 - SIN64[(16 + k * 2) & 63]) / 254; }   // first drop
        else if (i < 64) { int k = i - 32, r = 26; px = 104 + k / 2 + r * SIN64[(k * 2) & 63] / 127; py = H0 - 14 - r + r * SIN64[(16 + k * 2) & 63] / 127; }   // loop
        else if (i < 96) { int k = i - 64; px = 122 + k * 2; py = H0 - 34 - 22 * SIN64[(k * 4) & 63] / 127; }    // camel humps
        else { int k = i - 96; px = 186 - k * 176 / 31; py = H0 - 8; }                                       // return to the station
        s_trk_x[i] = (uint8_t)(px * W0 / 192); s_trk_y[i] = (uint8_t)py;
    }
}
static void draw_coaster(const unit_t *u, int x, int y, int w)
{
    int h = 5 * TW_FH;
    y -= 4 * TW_FH;
    coaster_track(w, h);
    bool on = u->occ && tw_is_open(u);
    rect(x + 1, y + 1, w - 2, h - 2, s_nt || on ? C(0x101830) : C(0x283048));           // dark hall
    for (int i = 0; i < 24; i++) rect(x + 4 + (i * 37) % (w - 8), y + 4 + (i * 53) % (h - 30), 1, 1, C(0x8080c0));   // ceiling stars
    for (int i = 0; i < 128; i += 6) rect(x + s_trk_x[i], y + s_trk_y[i], 1, h - 2 - s_trk_y[i], C(0x404860));   // supports
    for (int i = 0; i < 128; i++) {                                                    // rails
        int a = i, b = (i + 1) & 127, x0 = s_trk_x[a], y0 = s_trk_y[a], x1 = s_trk_x[b], y1 = s_trk_y[b];
        int n = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
        for (int k = 0; k <= n; k++) rect(x + x0 + (x1 - x0) * k / (n ? n : 1), y + y0 + (y1 - y0) * k / (n ? n : 1), 2, 2, C(0xe04050));
    }
    rect(x + 4, y + h - 20, 60, 2, C(0x808898));                                      // station platform
    int head = on ? (int)((s_blink * (3 + ((s_blink >> 5) & 1))) & 127) : 8;           // train: 4 cars
    for (int c = 0; c < 4; c++) {
        int p = (head - c * 3) & 127;
        int cx = x + s_trk_x[p] - 2, cy = y + s_trk_y[p] - 4;
        rect(cx, cy, 6, 4, c ? C(0xf0c020) : C(0x20b0f0));
        if (on) { rect(cx + 1, cy - 2, 2, 2, C(0xf0c0a0)); rect(cx + 4, cy - 2, 1, 2, C(0xf0c0a0)); }
    }
    rect(x + 4, y + 4, 52, 13, C(0x200828)); outline(x + 4, y + 4, 52, 13, C(0x40f0ff)); // neon sign
    gfx_text("COASTER", x + 8, y + 7, 1, ((s_blink >> 4) & 7) ? C(0x40f0ff) : C(0x108090));
    if (s_nt || on) gfx_blend(x + 2, y + 2, 56, 17, C(0x40f0ff), 30);
    int by = y + 4 * TW_FH;                                                            // entrance + queue on the bottom floor
    rect(x + w - 30, by + 4, 26, TW_FH - 6, C(0x303850)); gfx_text("RIDE", x + w - 27, by + 6, 1, C(0xffd040));
    for (int i = 0; i < u->cust_y / 8 && i < 6; i++) person(x + w - 36 - i * 5, by + 20, C(0x5050a0));
}


/* ------------------------------------------------------------ leisure / multi-storey facilities (y = bottom floor) */
static void draw_carpark(const unit_t *u, int x, int y, int w)
{
    int n = tw_uh(u), h = n * TW_FH, t0 = y - (n - 1) * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0x4a4e56));
    for (int k = 0; k < n; k++) {                                                      // decks, bays, ramps
        int dy = t0 + k * TW_FH;
        rect(x + 1, dy + TW_FH - 3, w - 2, 2, dim(0x7a7e86));
        for (int i = 14; i < w - 22; i += 10) rect(x + i, dy + TW_FH - 9, 1, 6, C(0xf0f0f0));
        int rx = (k & 1) ? x + 3 : x + w - 15;                                         // zig-zag ramp
        for (int j = 0; j < 12; j++) rect(rx + j, dy + TW_FH - 3 - ((k & 1) ? j : 11 - j) * (TW_FH - 4) / 12, 1, 2, C(0xd0c040));
        for (int i = 0; i < 4; i++) {                                                  // parked cars
            int cx = x + 16 + i * 10 + (k * 7 % 3);
            if (cx + 8 > x + w - 18 || ((i + k + u->x) % 3) == 2) continue;
            static const uint32_t CC[4] = {0xd03030, 0x3060d0, 0xe0e0e0, 0x303030};
            rect(cx, dy + TW_FH - 8, 8, 4, C(CC[(i + k) & 3])); rect(cx + 2, dy + TW_FH - 10, 4, 2, C(0x90b0d0));
        }
        int p = (int)((s_blink >> 1) + k * 23) % (w - 8);                              // a car circling the deck
        rect(x + 4 + p, dy + TW_FH - 8, 8, 4, C(0xf0a020)); rect(x + 6 + p, dy + TW_FH - 10, 4, 2, C(0xb0d0f0));
    }
    int gy = y + TW_FH - 13;                                                           // barrier gate on the bottom deck
    rect(x + w - 6, gy, 3, 10, C(0xf0d040));
    bool up = ((s_blink >> 5) & 3) == 0;
    if (up) rect(x + w - 6, gy - 10, 2, 10, C(0xf04040)); else rect(x + w - 22, gy, 16, 2, C(0xf04040));
    rect(x + 2, t0 + 2, 12, 9, C(0x2050c0)); gfx_text("P", x + 5, t0 + 3, 1, C(0xffffff));
}
static void draw_pool(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0xd8e8f0));
    for (int i = 0; i < w - 4; i += 8) rect(x + 2 + i, t0 + 2, 6, 4, C(0x9fd8f8));       // skylights
    int py = t0 + 22, ph = h - 25;
    rect(x + 6, py, w - 12, ph, C(0x2080d0));
    for (int i = 0; i < (w - 12) / 6; i++) {                                           // shimmer
        int sx = x + 7 + i * 6 + (int)((s_blink >> 2) + i * 3) % 5;
        rect(sx, py + 2 + (i * 5 + (s_blink >> 3)) % (ph - 4), 3, 1, C(0xb0e8ff));
    }
    for (int i = 0; i < 4; i++) rect(x + 10 + i * (w - 20) / 4, py, 1, ph, C(0xf0f0f0));   // lane ropes
    rect(x + w - 26, py - 3, 18, 2, C(0xf0f0f0)); rect(x + w - 10, py - 3, 2, ph + 3, C(0x909090));   // diving board
    if (u->occ && tw_is_open(u)) {
        int jy = (int)(s_blink >> 1) % 16;
        person(x + w - 22, py - 8 + (jy < 8 ? -jy / 2 : -4 + (jy - 8)), C(0xe05050));    // diver
        for (int i = 0; i < u->cust_y / 10 && i < 5; i++) rect(x + 14 + i * 9 + (int)(s_blink >> 3) % 3, py + 4 + (i & 1) * 6, 3, 2, C(0xf0c0a0));
    }
}
static void draw_ice(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0x7090b0));
    for (int i = 0; i < w - 4; i += 10) rect(x + 4 + i, t0 + 3, 4, 2, C(0xfff0c0));      // lights
    rect(x + 4, t0 + 20, w - 8, h - 23, C(0xe8f6ff));                                  // ice
    rect(x + 4, t0 + 18, w - 8, 3, C(0xf0f0f0)); rect(x + 4, t0 + 18, w - 8, 1, C(0x3050a0));   // boards
    rect(x + w / 2, t0 + 21, 1, h - 25, C(0xd04040));
    gfx_ellipse(x + w / 2, t0 + 21 + (h - 25) / 2, 5, 5, C(0x80a0e0));
    int n = u->occ && tw_is_open(u) ? 2 + u->cust_y / 8 : 0;
    for (int i = 0; i < n && i < 8; i++) {                                             // skaters gliding in ellipses
        int a = (int)((s_blink >> 1) + i * 8) & 63, rx = (w - 24) / 2, ry = 6;
        static const int8_t Q[16] = {0, 12, 24, 35, 45, 53, 59, 63, 64, 63, 59, 53, 45, 35, 24, 12};
        int sx = (a < 32 ? Q[a & 15] : -Q[a & 15]) * (a & 16 ? -1 : 1);
        int cy = (a < 16 || a >= 48) ? Q[(a + 16) & 15] : -Q[(a + 16) & 15];
        person(x + w / 2 + sx * rx / 64, t0 + 30 + cy * ry / 64 + (i & 1) * 4, C((i & 1) ? 0xd03060 : 0x3050c0));
    }
}
static void draw_atrium(const unit_t *u, int x, int y, int w)
{
    int h = 4 * TW_FH, t0 = y - 3 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0xf0e8d8));
    rect(x + w / 2 - 14, t0 + 1, 28, 6, C(0xa0d8f0));                                  // glass roof
    for (int k = 0; k < 4; k++) {                                                      // galleries with shopfronts
        int dy = t0 + k * TW_FH;
        rect(x + 1, dy + TW_FH - 3, 14, 3, dim(0xb0a080)); rect(x + w - 15, dy + TW_FH - 3, 14, 3, dim(0xb0a080));
        rect(x + 2, dy + 8, 10, TW_FH - 12, C((k & 1) ? 0xf080a0 : 0x80c0f0)); rect(x + w - 12, dy + 8, 10, TW_FH - 12, C((k & 1) ? 0x90d080 : 0xf0c060));
        rect(x + 15, dy + TW_FH - 6, w - 30, 1, C(0xc0c8d0));                           // glass balustrade
    }
    for (int k = 0; k < 3; k++) {                                                      // criss-cross escalators
        int dy = t0 + (k + 1) * TW_FH, x0 = x + 17, len = w - 34;
        for (int j = 0; j < len; j += 2) {
            int yy = (k & 1) ? dy - 3 - j * (TW_FH) / len : dy - 3 - (len - j) * (TW_FH) / len;
            rect(x0 + j, yy, 2, 2, C(0x707880));
        }
        int p = (int)((s_blink >> 1) + k * 9) % len;
        int yy = (k & 1) ? dy - 3 - p * TW_FH / len : dy - 3 - (len - p) * TW_FH / len;
        person(x0 + p, yy - 1, C(0x4060a0));
    }
    rect(x + w / 2 - 6, y + TW_FH - 9, 12, 6, C(0x40a060)); rect(x + w / 2 - 1, y + TW_FH - 15, 2, 6, C(0x80e0ff));   // fountain
}
static void draw_banquet(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    bool on = u->occ && tw_is_open(u) && tw_hour_x10() / 10 * 60 >= 1080;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(on ? 0x9a1c24 : 0x6a1820));
    rect(x + w / 2 - 22, t0 + 12, 44, 22, C(0xd8a830));                                // dragon-phoenix screen
    rect(x + w / 2 - 20, t0 + 14, 40, 18, C(0xb01818));
    for (int i = 0; i < 9; i++) rect(x + w / 2 - 17 + i * 2, t0 + 22 + ((i * 3) % 5) - 2, 2, 2, C(0x30a040));   // dragon (green)
    for (int i = 0; i < 7; i++) rect(x + w / 2 + 3 + i * 2, t0 + 18 + ((i * 2) % 4), 2, 2, C(0xf0d040));       // phoenix (gold)
    for (int i = 0; i < 3; i++) {                                                       // chandeliers
        int cx = x + 14 + i * (w - 28) / 2;
        rect(cx, t0 + 1, 1, 4, C(0xd0b060)); gfx_ellipse(cx, t0 + 7, 4, 2, C(0xf8e080));
        if (on && ((s_blink >> 3) + i) & 1) gfx_blend(cx - 6, t0 + 3, 13, 9, C(0xfff0a0), 80);
    }
    for (int i = 0; i < 5; i++) {                                                       // round tables with guests
        int tx = x + 6 + i * (w - 12) / 5;
        if (tx > x + w / 2 - 26 && tx < x + w / 2 + 22) continue;
        rect(tx, y + TW_FH - 8, 10, 3, C(0xf0f0e8));
        if (on) { person(tx - 2, y + TW_FH - 4, C(0x302030)); person(tx + 9, y + TW_FH - 4, C(0x302030)); }
    }
}
static void draw_climb(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH, t0 = y - 2 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0xe0d8c8));
    rect(x + 4, t0 + 4, w - 8, h - 8, C(0x8a6a4a));                                      // wall
    static const uint32_t HC[4] = {0xf04040, 0x40c040, 0xf0d020, 0x4080f0};
    for (int i = 0; i < 22; i++) rect(x + 6 + (i * 13) % (w - 13), t0 + 6 + (i * 29) % (h - 14), 3, 2, C(HC[i & 3]));   // holds
    if (u->occ && tw_is_open(u)) {
        int n = 1 + (u->cust_y > 20);
        for (int i = 0; i < n; i++) {
            int up = (int)((s_blink >> 3) + i * 17) % (h - 18);
            int cx = x + 10 + i * (w - 22) / 2, cy = t0 + h - 10 - up;
            rect(cx + 2, t0 + 3, 1, cy - t0 - 3, C(0xf0f0f0));                         // rope
            person(cx, cy, C(i ? 0x40a0f0 : 0xf06040));
        }
    }
    rect(x + 2, y + TW_FH - 4, w - 4, 2, C(0x3050a0));                                  // crash mat
}
static void draw_garden(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, s_nt ? C(0x18283a) : C(0xa8d8f0));               // open sky
    rect(x + 1, y + TW_FH - 6, w - 2, 5, dim(0x5a9a40));                                // lawn
    for (int i = 0; i < 4; i++) {                                                       // trees
        int tx = x + 8 + i * (w - 30) / 3;
        rect(tx + 3, t0 + 24, 2, h - 30, C(0x6a4a2a));
        gfx_ellipse(tx + 4, t0 + 20, 6, 7, dim(i & 1 ? 0x3a8a3a : 0x4aa04a));
        if (!s_nt) rect(tx + 1, t0 + 15, 2, 2, C(0x7ad07a));
    }
    int wx = x + w - 16;                                                                // waterfall + pond
    rect(wx, t0 + 4, 10, 6, C(0x808890));
    for (int j = 0; j < h - 16; j += 3) rect(wx + 2 + ((j + (s_blink >> 1)) % 3), t0 + 10 + j, 6, 2, C(((j / 3 + (s_blink >> 1)) & 1) ? 0xa0e0ff : 0x60b0f0));
    rect(wx - 6, y + TW_FH - 8, 18, 4, C(0x3080c0));
    if (s_nt) for (int i = 0; i < 6; i++) if (((s_blink >> 3) + i) % 3) rect(x + 6 + i * 7, t0 + 8 + (i * 5) % 14, 1, 1, C(0xf0f080));   // fireflies / lanterns
    if (u->occ) for (int i = 0; i < 2; i++) person(x + 14 + i * 18, y + TW_FH - 6, C(0x705090));
}
static void draw_studio(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH, t0 = y - 2 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(0x18181e));
    for (int i = 0; i < w - 8; i += 3) rect(x + 4 + i, t0 + 3, 2, 3, C(0x404048));      // lighting grid
    for (int i = 0; i < 4; i++) {                                                       // spotlights with beams
        int lx = x + 12 + i * (w - 24) / 3;
        rect(lx - 2, t0 + 6, 5, 4, C(0x707078));
        if (((s_blink >> 3) + i) % 4) gfx_blend(lx - 6 - (i - 2) * 4, t0 + 10, 12, h - 24, C(0xfff0b0), 40);
    }
    rect(x + w / 2 - 20, t0 + 20, 40, 26, C(0x2a40a0));                                 // set / backdrop
    for (int i = 0; i < 5; i++) rect(x + w / 2 - 18 + i * 8, t0 + 24 + (i & 1) * 6, 6, 4, C(0x6080f0));
    rect(x + w / 2 - 14, y + TW_FH - 10, 28, 6, C(0xc0a060));                           // desk
    person(x + w / 2 - 6, y + TW_FH - 8, C(0xe04040)); person(x + w / 2 + 4, y + TW_FH - 8, C(0x303080));
    for (int i = 0; i < 2; i++) {                                                       // cameras
        int cx = i ? x + w - 22 : x + 8;
        rect(cx, y + TW_FH - 18, 12, 7, C(0x404048)); rect(cx + (i ? -3 : 12), y + TW_FH - 16, 3, 3, C(0x101010));
        rect(cx + 5, y + TW_FH - 11, 2, 9, C(0x606068));
        if (!i && (s_blink >> 4) & 1) rect(cx + 1, y + TW_FH - 17, 2, 2, C(0xff2020));      // tally light
    }
    rect(x + 3, t0 + 10, 22, 9, C(0xe02020)); gfx_text("ON AIR", x + 4, t0 + 11, 1, (s_blink >> 4) & 1 ? C(0xffffff) : C(0xffa0a0));
    if (S->celeb) { gfx_ellipse(x + w / 2, t0 + 16, 3, 3, C(0xffe040)); textc("STAR!", x + w / 2, t0 + 11, C(0xffe040)); }
}


/* ------------------------------------------------------------ underground challenge (y = bottom floor) */
static void draw_pump(const unit_t *u, int x, int y, int w)
{
    rect(x + 1, y + 3, w - 2, TW_FH - 4, dim(0x3c5a82));
    gfx_ellipse(x + 8, y + 13, 5, 5, C(0x2a70b0)); rect(x + 8, y + 3, 2, 6, C(0x80c0f0));
    rect(x + 13, y + 11, w - 15, 3, C(0x80c0f0));
    int s = (int)(s_blink >> 1) & 3; rect(x + 6 + s, y + 12, 3, 2, C(0xe0f0ff));     // impeller
    gfx_text("PUMP", x + 2, y + 16, 1, C(0xffffff));
}
static void draw_vent(const unit_t *u, int x, int y, int w)
{
    rect(x + 1, y, w - 2, TW_FH, dim(0x6e7878));
    for (int j = 2; j < TW_FH - 2; j += 3) rect(x + 2, y + j, w - 4, 1, C(0x404848));
    int a = (int)(s_blink >> 1) & 3;
    rect(x + w / 2 - 1, y + 8, 2, 8, C(0xd0d8d8)); rect(x + w / 2 - 4 + a * 2, y + 11, 2, 2, C(0xffffff));
}
static void draw_mhub(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH, t0 = y - 2 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0xd8d0c0));
    rect(x + 2, t0 + 2, w - 4, 10, C(0xc83c32)); gfx_text("METRO HUB", x + w / 2 - 26, t0 + 3, 1, C(0xffffff));
    for (int k = 0; k < 2; k++) {                                                      // two platforms, trains gliding
        int py = t0 + 16 + k * 26;
        rect(x + 2, py + 16, w - 4, 3, C(0x808080));
        int tx = (int)((s_blink * (k ? 2 : 3)) % (uint32_t)(w + 80)) - 60;
        if (k) tx = w - tx - 60;
        for (int c = 0; c < 3; c++) {
            int cx = x + tx + c * 20;
            if (cx + 18 < x || cx > x + w) continue;
            int l = cx < x + 2 ? x + 2 : cx, r = cx + 18 > x + w - 2 ? x + w - 2 : cx + 18;
            if (r > l) { rect(l, py + 4, r - l, 12, C(k ? 0x40a040 : 0xe0e0e0)); rect(l, py + 7, r - l, 3, C(0x203040)); }
        }
    }
    for (int i = 0; i < u->cust_y / 8 && i < 8; i++) person(x + 6 + i * 5, y + TW_FH - 8, C(0x405080));
}
static void draw_spa(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0x7a5040));
    for (int i = 0; i < 6; i++) rect(x + 4 + i * (w - 8) / 6, t0 + 3, 6, 3, C(0x9a6a50));   // rock ceiling
    rect(x + 6, y + 6, w - 12, TW_FH - 8, C(0x60b8c8));                              // hot pool
    for (int i = 0; i < 5; i++) {                                                     // steam wisps
        int sx = x + 10 + i * (w - 20) / 5, sy = t0 + 20 - (int)((s_blink >> 2) + i * 5) % 14;
        rect(sx + ((s_blink >> 3) + i) % 3, sy, 2, 3, C(0xf0f0f0));
    }
    if (u->occ) for (int i = 0; i < 3; i++) rect(x + 14 + i * 14, y + 6, 4, 3, C(0xf0c0a0));   // bathers
    gfx_text("SPA", x + 3, t0 + 8, 1, C(0xffe0c0));
}
static void draw_fossil(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH, t0 = y - 2 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0xc8b890));
    int bx = x + w / 2 - 30, by = t0 + 22;                                            // T-rex skeleton
    uint16_t bone = C(0xf8f0e0);
    rect(bx, by, 14, 8, bone); rect(bx + 2, by + 8, 2, 3, bone); rect(bx + 8, by + 8, 2, 3, bone);   // skull + teeth
    rect(bx + 12, by + 6, 24, 3, bone);                                               // spine
    for (int i = 0; i < 6; i++) rect(bx + 14 + i * 4, by + 9, 1, 8, bone);           // ribs
    rect(bx + 36, by + 7, 18, 2, bone);                                               // tail
    rect(bx + 16, by + 17, 2, 18, bone); rect(bx + 28, by + 17, 2, 18, bone);        // legs
    rect(bx - 4, y + TW_FH - 6, 64, 3, C(0x806040));                                  // plinth
    for (int i = 0; i < u->cust_y / 10 && i < 4; i++) person(x + 6 + i * 6, y + TW_FH - 8, C(0x506030));
    gfx_text("FOSSILS", x + 3, t0 + 4, 1, C(0x604020));
}
static void draw_gem(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, dim(0x3a2a4a));
    for (int i = 0; i < 2; i++) rect(x + 4 + i * (w - 12), t0 + 2, 4, h - 4, C(0x8a6a3a));   // timber props
    static const uint32_t G[4] = {0x40f0ff, 0xff40a0, 0x80ff60, 0xffe040};
    for (int i = 0; i < 12; i++) {                                                    // glittering gems in the rock
        int gx = x + 10 + (i * 23) % (w - 20), gy = t0 + 4 + (i * 13) % (h - 12);
        uint16_t c = C(G[i & 3]);
        rect(gx, gy, 2, 2, ((s_blink >> 2) + i) % 5 ? c : C(0xffffff));
    }
    rect(x + w / 2 - 8, y + TW_FH - 9, 16, 5, C(0x707070));                            // mine cart
    rect(x + w / 2 - 6, y + TW_FH - 11, 4, 2, C(0x40f0ff)); rect(x + w / 2, y + TW_FH - 11, 4, 2, C(0xff40a0));
}
static void draw_nmarket(const unit_t *u, int x, int y, int w)
{
    bool on = u->occ && tw_is_open(u);
    rect(x, y + 1, w, TW_FH - 2, dim(0x281820));
    static const uint32_t AW[4] = {0xe03030, 0xf0c020, 0x30a0e0, 0x40c060};
    for (int i = 0; i < w / 20; i++) {                                                // stalls with awnings + bulb strings
        int sx = x + 2 + i * 20;
        rect(sx, y + 3, 18, 4, C(AW[i & 3])); rect(sx, y + 7, 1, TW_FH - 9, C(0x8a6a3a)); rect(sx + 17, y + 7, 1, TW_FH - 9, C(0x8a6a3a));
        rect(sx + 2, y + 14, 14, 3, C(0xc08040));
        if (on) for (int j = 0; j < 4; j++) rect(sx + 2 + j * 4, y + 2, 1, 1, ((s_blink >> 3) + i + j) & 1 ? C(0xffe080) : C(0x806020));
    }
    if (on) { for (int i = 0; i < u->cust_y / 6 && i < 10; i++) person(x + 6 + i * 8, y + TW_FH - 7, C(0x605080)); gfx_blend(x, y + 1, w, 8, C(0xffa040), 40); }
}
static void draw_aqua(const unit_t *u, int x, int y, int w)
{
    int h = 3 * TW_FH, t0 = y - 2 * TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(0x0c2850));
    rect(x + 4, t0 + 4, w - 8, h - 14, C(0x1c5ab0));                                  // big tank
    for (int i = 0; i < 6; i++) rect(x + 8 + i * (w - 16) / 6, t0 + h - 14 - (i * 7) % 10, 2, 8 + (i * 7) % 10, C(0x30a050));   // kelp
    for (int i = 0; i < 7; i++) {                                                     // fish swimming
        int fx = x + 6 + (int)((s_blink >> 1) * (1 + (i & 1)) + i * 37) % (w - 18), fy = t0 + 8 + (i * 11) % (h - 26);
        uint16_t c = C(i % 3 == 0 ? 0xffa020 : i % 3 == 1 ? 0xf0f040 : 0xf06090);
        rect(fx, fy, 5, 3, c); rect(fx - 2, fy, 2, 3, c);
    }
    int sx = x + (int)((s_blink >> 2) % (uint32_t)(w - 30)) + 4;                     // the whale shark
    rect(sx, t0 + 30, 22, 7, C(0x8090a8)); rect(sx + 22, t0 + 31, 4, 5, C(0x8090a8));
    for (int i = 0; i < 4; i++) rect(sx + 3 + i * 5, t0 + 32, 1, 1, C(0xffffff));
    for (int i = 0; i < u->cust_y / 10 && i < 5; i++) person(x + 8 + i * 8, y + TW_FH - 8, C(0x204060));
}
static void draw_lab(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(0x1e2a2a));
    for (int i = 0; i < 3; i++) {                                                     // glowing tubes
        int tx = x + 8 + i * 22;
        rect(tx, t0 + 8, 10, h - 14, C(0x204040)); rect(tx + 2, t0 + 10, 6, h - 18, C((((s_blink >> 3) + i) & 1) ? 0x40ff90 : 0x30c070));
        rect(tx + 4, t0 + 14 + (int)((s_blink >> 1) + i * 5) % (h - 26), 2, 2, C(0xe0ffe0));   // bubbles
    }
    rect(x + w - 30, y + 4, 26, 10, C(0x101818));                                     // console
    for (int i = 0; i < 6; i++) rect(x + w - 28 + i * 4, y + 6, 2, 2, ((s_blink >> 2) + i) % 3 ? C(0x40ff40) : C(0xff4040));
    rect(x + 3, t0 + 3, 36, 8, C(0xf0d020)); gfx_text("LAB 51", x + 4, t0 + 3, 1, C(0x101010));
}
static void draw_geo(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(0x3a1a12));
    for (int i = 0; i < 3; i++) {                                                     // turbines
        int cx = x + 16 + i * (w - 32) / 2;
        gfx_ellipse(cx, t0 + 18, 9, 9, C(0x808890));
        int a = ((int)(s_blink >> 1) + i * 5) & 63;
        rect(cx + 7 * SIN64[a] / 127 - 1, t0 + 18 + 7 * SIN64[(a + 16) & 63] / 127 - 1, 3, 3, C(0x202020));
    }
    for (int i = 0; i < w - 4; i += 5) rect(x + 2 + i, y + TW_FH - 6, 4, 4, ((s_blink >> 2) + i / 5) & 1 ? C(0xff8020) : C(0xd03010));   // magma channel
    for (int i = 0; i < 4; i++) rect(x + 6 + i * 20, t0 + 3, 2, 10, C(0xf0c040));     // power cables
}
static void draw_vault(const unit_t *u, int x, int y, int w)
{
    int h = 2 * TW_FH, t0 = y - TW_FH;
    rect(x + 1, t0 + 1, w - 2, h - 2, C(0x2a2010));
    int cx = x + w / 2, cy = t0 + h / 2;
    gfx_ellipse(cx, cy, 20, 20, C(0xd8b040)); gfx_ellipse(cx, cy, 16, 16, C(0xa08020));   // the great round door
    for (int i = 0; i < 8; i++) { int a = (i * 8 + (int)(s_blink >> 3)) & 63; rect(cx + 12 * SIN64[a] / 127 - 1, cy + 12 * SIN64[(a + 16) & 63] / 127 - 1, 3, 3, C(0xfff0a0)); }
    gfx_ellipse(cx, cy, 5, 5, C(0xfff0a0));
    for (int i = 0; i < 6; i++) rect(x + 6 + i * 4, y + TW_FH - 8 - (i & 1) * 3, 3, 5, C(0xffd040));   // gold bars
    for (int i = 0; i < 6; i++) rect(x + w - 30 + i * 4, y + TW_FH - 8 - (i & 1) * 3, 3, 5, C(0xffd040));
    gfx_text("CORE VAULT", x + 4, t0 + 3, 1, C(0xffe080));
    gfx_blend(x + 1, t0 + 1, w - 2, h - 2, C(0xff8020), 20 + (int)((s_blink >> 2) & 7) * 4);   // magma heat glow
}

/* ------------------------------------------------------------ rooms */
static void scaffold(int x, int y, int w, int h, int left, int tot)
{
    rect(x, y, w, h, dim(0x5a5a50));
    for (int i = 0; i < w; i += 6) rect(x + i, y, 1, h, C(0xc8a050));          // bamboo poles
    for (int j = 3; j < h; j += 6) rect(x, y + j, w, 1, C(0xc8a050));
    gfx_blend(x, y, w, h, C(0x30a050), 90);                                   // green safety net
    int pct = tot ? (tot - left) * 100 / tot : 100;
    rect(x + 2, y + h - 4, w - 4, 2, C(0x202020));
    rect(x + 2, y + h - 4, (w - 4) * pct / 100, 2, C(0xf0d040));
}
static void draw_unit(int id)
{
    const unit_t *u = &S->u[id];
    const tdef_t *d = &g_T[u->t];
    int x = xseg(u->x), w = u->w * TW_SEG, y = yrow(u->f), h = tw_uh(u) * TW_FH, top = y - h + TW_FH;
    if (u->t == T_FLOOR && u->f < BASE && !u->build) {                         // dug, empty basement: the rock band shows through
        for (int i = 0; i < w; i += 16) rect(x + i, top + 2, 1, h - 4, gfx_mix(dim565(d->col), C(0x000000), 60));
        rect(x, top, w, 1, dim(0x707070)); rect(x, top + h - 1, w, 1, dim(0x404048));
        return;
    }
    rect(x, top, w, h, dim565(d->col));
    rect(x, top + h - 1, w, 1, dim(0x404048));                                // slab line
    if (u->build) { scaffold(x, top, w, h, u->build, d->build_min); return; }
    bool lit = s_nt && u->occ;
    if (tw_uh(u) == 1 && u->t != T_SHOP && u->t != T_CCT && u->t != T_DIMSUM && u->t != T_LOBBY && u->t != T_FLOOR && u->t != T_REFUGE) {
        rect(x, y, w, 2, dim(0x505060)); y += 2;                                  // ceiling beam; content sits 2 px lower
    }
    switch (u->t) {
    case T_LOBBY:
        rect(x, y + 1, w, TW_FH - 5, dim(0xe0d0b0));
        for (int i = 0; i < w; i += 16) rect(x + i + 1, y + 1, 3, TW_FH - 2, dim(0xb0a080));   // columns
        if (u->f == BASE && u->x <= 32 && u->x + u->w > 28) { rect(xseg(30) - 6, y + 4, 12, TW_FH - 5, C(0x80c0e0)); }
        rect(x, y + TW_FH - 2, w, 2, dim(0x806040));
        break;
    case T_FLOOR:
        rect(x, y, w, TW_FH, dim(0x5a5a66));
        for (int i = 0; i < w; i += 8) rect(x + i, y + 2, 1, TW_FH - 4, dim(0x484852));
        break;
    case T_STAIRS:
        for (int i = 0; i < 8; i++) rect(x + 2 + i * 3, y + 20 - i * 2, 6, 2, dim(0x8a7a5a));
        break;
    case T_OFFICE:
        for (int i = 0; i < 4; i++) {
            rect(x + 3 + i * 8, y + 4, 5, 10, lit && u->inside ? C(0xffe080) : u->occ ? dim(0x4a6a9a) : dim(0x505050));
            if (!s_nt && u->occ && u->inside > i) person(x + 4 + i * 8, y + 11, C(0x203050));
        }
        if (!u->occ) { rect(x + 3, y + 5, 40, 9, C(0xf0f0f0)); gfx_text("TO LET", x + 5, y + 6, 1, C(0xc02020)); }
        break;
    case T_FLAT:
        for (int i = 0; i < 4; i++) rect(x + 4 + i * 10, y + 4, 5, 9, lit && u->inside ? C(0xffd070) : u->occ ? dim(0x7a5a30) : dim(0x555555));
        rect(x + w - 6, y + 4, 4, 15, dim(0x8a4a2a));
        for (int i = 0; i < 3; i++) rect(x + 6 + i * 18, y + 15, 6, 1, dim(0xe0e0e0));
        if (!u->occ) { rect(x + 28, y + 4, 28, 9, C(0xf0f0f0)); gfx_text("SALE", x + 30, y + 5, 1, C(0xc02020)); }
        break;
    case T_SINGLE: case T_TWIN: case T_SUITE:
        rect(x + 2, y + 12, w - 4, 5, u->roach ? C(0x4a2a10) : u->dirty ? C(0xb0a070) : dim(0xffffff));
        rect(x + 2, y + 10, 3, 2, dim(0xdddddd));
        if (lit && (u->inside || u->stay)) rect(x + w - 5, y + 4, 3, 3, C(0xffe080));
        if (u->roach) for (int i = 0; i < 3; i++) rect(x + 3 + ((i * 7 + (s_blink >> 1)) % (w - 6)), y + 16, 2, 1, C(0x201000));
        break;
    case T_HOUSEK: gfx_text("HOUSEKEEP", x + 4, y + 6, 1, C(0x202040)); rect(x + 34, y + 10, 20, 8, dim(0xffffff)); break;
    case T_SECURITY: gfx_text("GUARD", x + 4, y + 6, 1, C(0xffffff)); for (int i = 0; i < 3; i++) rect(x + 34 + i * 8, y + 5, 6, 5, C((s_blink >> 3) & 1 ? 0x40ff40 : 0x208020)); break;
    case T_CLINIC: rect(x + 6, y + 5, 10, 3, C(0xe02020)); rect(x + 9, y + 2, 4, 9, C(0xe02020)); gfx_text("CLINIC", x + 22, y + 5, 1, C(0x202060)); break;
    case T_RECYCLE: gfx_text("RECYCLE", x + 6, top + 4, 1, C(0xffffff)); for (int i = 0; i < 4; i++) rect(x + 10 + i * 20, top + 22, 12, 14, dim((uint32_t[]){0x2060c0, 0xe0a020, 0x808080, 0x40a040}[i])); break;
    case T_PARKING: for (int i = 0; i < 3; i++) { rect(x + 4 + i * 20, y + 10, 14, 6, dim((uint32_t[]){0xc03030, 0x3060c0, 0xe0e0e0}[i])); rect(x + 6 + i * 20, y + 7, 9, 3, dim(0x404850)); } break;
    case T_METRO:
        rect(x, top + 30, w, 30, dim(0x707070));
        rect(x + 8, top + 34, w - 16, 14, C(0xd0d0d8)); for (int i = 0; i < 8; i++) rect(x + 12 + i * 13, top + 37, 9, 6, C(0x203050));
        rect(x + 8, top + 44, w - 16, 2, C(0xc02020));
        gfx_text("METRO", x + 6, top + 6, 2, C(0xffffff));
        break;
    case T_CHAPEL:
        rect(x + w / 2 - 2, top + 2, 4, 14, C(0xc0a040)); rect(x + w / 2 - 6, top + 6, 12, 3, C(0xc0a040));
        for (int i = 0; i < 5; i++) rect(x + 8 + i * 20, top + 26, 8, 40, s_nt ? C(0xffd080) : C(0x80a0e0));
        gfx_text("CHAPEL", x + w / 2 - 17, top + 70, 1, C(0x806020));
        if (S->tower_done) for (int i = 0; i < 6; i++) person(x + 10 + i * 16, top + 70, C(i & 1 ? 0xffffff : 0x202020));
        break;
    case T_SHOP: draw_shop(u, x, y, w); break;
    case T_CCT: case T_DIMSUM: draw_food(u, x, y, w); break;
    case T_CINEMA: draw_cinema(u, x, y, w); break;
    case T_MEGA: draw_mega(u, x, y, w); break;
    case T_COASTER: draw_coaster(u, x, y, w); break;
    case T_REFUGE: draw_refuge(u, x, y, w); break;
    case T_CARPARK: draw_carpark(u, x, y, w); break;
    case T_POOL: draw_pool(u, x, y, w); break;
    case T_ICE: draw_ice(u, x, y, w); break;
    case T_ATRIUM: draw_atrium(u, x, y, w); break;
    case T_BANQUET: draw_banquet(u, x, y, w); break;
    case T_CLIMB: draw_climb(u, x, y, w); break;
    case T_GARDEN: draw_garden(u, x, y, w); break;
    case T_STUDIO: draw_studio(u, x, y, w); break;
    case T_PUMP: draw_pump(u, x, y, w); break;
    case T_VENT: draw_vent(u, x, y, w); break;
    case T_MHUB: draw_mhub(u, x, y, w); break;
    case T_SPA: draw_spa(u, x, y, w); break;
    case T_FOSSIL: draw_fossil(u, x, y, w); break;
    case T_GEM: draw_gem(u, x, y, w); break;
    case T_NMARKET: draw_nmarket(u, x, y, w); break;
    case T_AQUA: draw_aqua(u, x, y, w); break;
    case T_LAB: draw_lab(u, x, y, w); break;
    case T_GEO: draw_geo(u, x, y, w); break;
    case T_VAULT: draw_vault(u, x, y, w); break;
    }
    if (u->fire_h) {                                                         // fire
        for (int i = 0; i < w; i += 5) {
            int fh = 6 + ((i * 7 + s_blink) % 9);
            rect(x + i, top + h - fh - 1, 4, fh, C((i / 5 + (s_blink >> 1)) & 1 ? 0xff6010 : 0xffd020));
        }
    }
}

/* ------------------------------------------------------------ lifts */
static void draw_shaft(int si)
{
    const shaft_t *s = &S->s[si];
    int x = xseg(s->x), w = s->w * TW_SEG;
    bool ex = s->t == T_EXP;
    int lo = U->cam, hi = U->cam + TW_VIS - 1;
    int f0 = s->f0 > lo ? s->f0 : lo, f1 = s->f1 < hi ? s->f1 : hi;
    if (f0 > f1) return;
    int ytop = yrow(f1), ybot = yrow(f0) + TW_FH;
    if (s->build) { scaffold(x, ytop, w, ybot - ytop, s->build, g_T[s->t].build_min); return; }
    rect(x, ytop, w, ybot - ytop, C(ex ? 0x24283a : 0x2c3040));
    rect(x + w / 2, ytop, 1, ybot - ytop, C(0x50586a));                      // cable
    for (int f = f0; f <= f1; f++) {
        int y = yrow(f);
        if (!tw_serves(s, f)) continue;
        rect(x - 1, y + 5, 1, 18, C(ex ? 0xd0a040 : 0x888888));                 // door frame
        rect(x + w, y + 5, 1, 18, C(ex ? 0xd0a040 : 0x888888));
        int mw; bool up, dn;
        int n = tw_waiting(si, f, &mw, &up, &dn);
        rect(x - 4, y + 8, 2, 2, up ? C(0x40ff40) : C(0x304030));               // hall lanterns
        rect(x - 4, y + 12, 2, 2, dn ? C(0xff4040) : C(0x403030));
        uint16_t qc = mw > 120 ? C(0xe03030) : mw > 48 ? C(0xe0c030) : C(0x40a050);
        for (int i = 0; i < (n + 2) / 3 && i < 6; i++) person(x - 10 - i * 4, y + 17, qc);
    }
    int cys[MAX_CARS];
    for (int k = 0; k < s->ncars; k++) cys[k] = TW_TY + (int)(((int32_t)(U->cam + TW_VIS - 1) * 256 - s->car[k].disp) * TW_FH / 256);
    for (int k = 0; k < s->ncars; k++) {
        const car_t *c = &s->car[k];
        int cy = cys[k];
        /* cars at the same height share the shaft in lanes; a lone car uses the full width */
        int n = 0, lane = 0, lead = k;
        for (int j = 0; j < s->ncars; j++) if (abs(cys[j] - cy) < TW_FH) { if (j < k) { lane++; if (j < lead) lead = j; } n++; }
        int cw = (w - 2) / n, cx = x + 1 + lane * cw;
        if (lane == n - 1) cw = w - 2 - lane * cw;
        rect(cx, cy + 2, cw, TW_FH - 3, C(ex ? 0xd8b048 : 0xc8ccd8));
        rect(cx + 1, cy + 4, cw - 2, TW_FH - 7, C(0xfff4d0));                  // lit cabin
        for (int i = 0; i < c->load && i < (cw - 2) / 3; i++) person(cx + 2 + i * 3, cy + 14, C(0x304060));
        int half = (cw - 2) / 2, open = half * c->door_a / 256;                 // sliding doors
        rect(cx + 1, cy + 4, half - open, TW_FH - 7, C(ex ? 0xb08830 : 0x9098a8));
        rect(cx + 1 + half + open, cy + 4, cw - 2 - half - open, TW_FH - 7, C(ex ? 0xb08830 : 0x9098a8));
        if (lead == k) {                                                         // one floor indicator per group
            char b[8];
            tw_floor_name((c->disp + 128) >> 8, b);
            int tw = gfx_text_w(b, 1) + 2, ix = x + w / 2 - tw / 2;
            rect(ix, cy - 4, tw, 8, C(0x101010));
            gfx_text(b, ix + 1, cy - 3, 1, C(0xff8020));
            if (n > 1) for (int j = 0; j < n && j < 4; j++) rect(ix + tw + 1, cy - 4 + j * 2, 1, 1, C(0xff8020));   // 1 dot per car here
        }
        for (int i = 0; i < c->n_anim; i++) {                                    // boarding / alighting
            int t = c->anim_t[i];
            if (t < 0) continue;
            int py = yrow(c->hold_f) + 16, px = c->anim_out[i] ? cx + 2 - t * 2 : cx - 26 + t * 2;
            person(px, py, C(c->anim_out[i] ? 0x6080c0 : 0x304060));
        }
    }
}

/* ------------------------------------------------------------ the world */
static void draw_backdrop(void)
{
    int day = 256 - s_dusk;
    uint16_t sky = gfx_mix(C(0x101838), C(0x78b8e8), day);
    if (s_dusk > 40 && s_dusk < 200) sky = gfx_mix(sky, C(0xf09060), 60);
    if (S->typhoon || S->typhoon_sig >= 3 || S->rain_h) sky = gfx_mix(sky, C(0x505868), S->typhoon || S->rain_h ? 180 : 100);
    int gy = yrow(BASE) + TW_FH;
    rect(0, TW_TY, WORLD_W, gy - TW_TY, sky);
    if (s_nt) for (int i = 0; i < 24; i++) { int sx = (i * 97 + 13) % WORLD_W, sy = TW_TY + (i * 53 + U->cam * 7) % 200; if (sy < gy) rect(sx, sy, 1, 1, C(0xc0c0ff)); }
    if (gy < TW_TY + TW_VIS * TW_FH) {                                        // underground: depth bands
        for (int r = 0; r < TW_VIS; r++) {
            int f = U->cam + TW_VIS - 1 - r;
            if (f >= BASE) continue;
            int y = yrow(f), d = tw_depth(f);
            static const uint32_t BAND[4] = {0x5a4030, 0x56545a, 0x342f3c, 0x3a1410};   // soil, rock, granite, magma
            int b = d <= 30 ? 0 : d <= 60 ? 1 : d <= 85 ? 2 : 3;
            bool dug = f >= S->dug;
            uint16_t base = C(BAND[b]);
            rect(0, y, WORLD_W, TW_FH, dug ? gfx_mix(base, C(0x8a8478), 90) : base);
            for (int i = 0; i < 14; i++) {                                          // pebbles / strata
                int px = (i * 71 + f * 37) % WORLD_W, py = y + 3 + (i * 7 + f * 5) % (TW_FH - 6);
                rect(px, py, 3 + (i & 1) * 2, 2, gfx_mix(base, C(b == 3 ? 0xff6020 : 0xd0c0a0), dug ? 50 : 70));
            }
            if (!dug) rect(0, y + (f * 7) % (TW_FH - 4) + 2, WORLD_W, 1, gfx_mix(base, C(0x000000), 80));   // strata line
            if (b == 3) for (int i = 0; i < 5; i++) {                              // magma glow cracks
                int cx = (i * 61 + f * 13) % (WORLD_W - 20);
                uint16_t g = ((s_blink >> 3) + i + f) & 1 ? C(0xff7020) : C(0xc03010);
                rect(cx, y + TW_FH - 4, 14, 1, g); rect(cx + 5, y + TW_FH - 7, 1, 3, g);
            }
            if (!dug && f == S->dug - 1 && S->dig_h) {                              // the drill
                int tot = tw_dig_hours(), pct = tot ? (tot - S->dig_h) * 100 / tot : 0;
                int dx = TW_TX + 8 + (WORLD_W - TW_TX - 40) * pct / 100;
                rect(TW_TX, y + 4, dx - TW_TX, TW_FH - 8, gfx_mix(base, C(0x8a8478), 90));
                rect(dx, y + 6, 12, TW_FH - 12, C(0xf0c020)); rect(dx + 12, y + 9, 6, 6, C(0x909090));
                rect(dx + 18 + ((s_blink >> 1) & 1), y + 10, 3, 4, C(0xd0d0d0));
                for (int i = 0; i < 4; i++) rect(dx + 20 + ((s_blink + i * 3) % 7), y + 6 + i * 4, 2, 2, C(0xc0a080));   // debris
                gfx_text("DIGGING", dx - 46 < TW_TX ? TW_TX + 2 : dx - 46, y + 8, 1, C(0xffe080));
            }
        }
        rect(0, gy, WORLD_W, 2, C(0x3a8a3a));
    }
    if (S->typhoon || S->rain_h || S->typhoon_sig >= 3)
        for (int i = 0; i < (S->rain_h ? 50 : 30); i++)
            rect((i * 41 + (int)U->frame * (S->rain_h ? 5 : 3)) % WORLD_W, TW_TY + (i * 29 + (int)U->frame * 7) % 200, 1, S->rain_h ? 6 : 4, S->rain_h ? C(0x6080a0) : C(0xa0b0d0));
    if (S->quake_h && ((U->frame >> 1) & 1)) {                                          // screen shake
        /* drawn as a jitter on the tower edge */
        rect(TW_TX - 2, TW_TY + ((int)U->frame * 11) % (TW_VIS * TW_FH), 2, 8, C(0xffd040));
    }
    if (S->power_h) {                                                                   // dim the tower during a blackout
        gfx_blend(TW_TX, TW_TY, WORLD_W - TW_TX, TW_VIS * TW_FH, C(0x000010), 90);
        gfx_text("POWER CUT", TW_TX + 4, TW_TY + 4, 1, C(0xff8040));
    }
    if (S->fest) {                                                                      // Christmas / Mid-Autumn decorations
        for (int f = U->cam; f < U->cam + TW_VIS && f < NF; f++) {
            int y = yrow(f);
            if (S->fest == 1) {                                                         // Christmas lights
                for (int i = 0; i < 8; i++) rect(TW_TX + 4 + i * 28, y + 2, 2, 2, ((s_blink >> 2) + i + f) & 1 ? C(0xff4040) : C(0x40ff40));
            } else {                                                                    // lanterns
                for (int i = 0; i < 4; i++) { int lx = TW_TX + 20 + i * 48; rect(lx, y + 2, 6, 8, C(0xff6020)); rect(lx + 2, y, 2, 2, C(0xffd040)); }
            }
        }
    }
    if (S->santa && !S->power_h) {                                                      // Santa riding a lift car: red hat on a random floor
        int sf = U->cam + ((int)(U->frame >> 3) % TW_VIS);
        if (sf >= 0 && sf < NF) { int y = yrow(sf); rect(TW_TX + WORLD_W / 2 - 4, y + 4, 8, 10, C(0xe02020)); rect(TW_TX + WORLD_W / 2 - 4, y + 2, 8, 3, C(0xffffff)); }
    }
    rect(0, TW_TY, TW_TX - 2, TW_VIS * TW_FH, C(0x181820));                    // floor gutter
}
static void draw_world(void)
{
    draw_backdrop();
    int lo = U->cam, hi = U->cam + TW_VIS - 1;
    int by0 = gfx_band_y0(), by1 = by0 + gfx_band_rows();
    for (int f = lo - (MAX_H - 1) < 0 ? 0 : lo - (MAX_H - 1); f <= hi; f++) {
        int y = yrow(f);
        if (y >= by1 + (MAX_H - 1) * TW_FH || y + TW_FH <= by0) continue;
        for (uint16_t i = S->floor_head[f]; i != NO_ID; i = S->u[i].next) {
            const unit_t *u = &S->u[i];
            int top = y - (tw_uh(u) - 1) * TW_FH;
            if (top >= by1 || y + TW_FH <= by0) continue;
            int x = xseg(u->x);
            if (x >= gfx_band_x0() + gfx_band_w() || x + u->w * TW_SEG <= gfx_band_x0()) continue;
            draw_unit(i);
        }
    }
    for (int i = 0; i < S->n_shafts; i++) draw_shaft(i);
    if (S->flood_h && S->flood_f >= lo && S->flood_f <= hi) {                  // flood: water across the floor
        int y = yrow(S->flood_f), lvl = 6 + (6 - S->flood_h) * 2;
        gfx_blend(TW_TX, y + TW_FH - lvl, WORLD_W - TW_TX, lvl, C(0x2060c0), 150);
        for (int i = 0; i < 12; i++) rect(TW_TX + ((i * 23 + (int)(s_blink >> 1)) % (WORLD_W - TW_TX - 4)), y + TW_FH - lvl, 4, 1, C(0xa0d0ff));
        gfx_text("FLOOD", TW_TX + 4, y + 2, 1, C(0x80c0ff));
    }
    for (int f = lo; f <= hi; f++) {                                            // floor numbers
        int y = yrow(f);
        if (y >= by1 || y + TW_FH <= by0) continue;
        char b[8];
        tw_floor_name(f, b);
        bool c = f == U->cur;
        if (c) rect(0, y, TW_TX - 2, TW_FH, C(0xf0c020));
        int tx = TW_TX - 3 - gfx_text_w(b, 1);
        gfx_text(b, tx < 0 ? 0 : tx, y + 7, 1, c ? C(0x000000) : tw_is_lobby_floor(f) ? C(0xf0c020) : C(0x9090a0));
    }
    if (U->mode == M_PLACE) {                                                  // ghost
        const tdef_t *d = &g_T[U->place.t];
        int f0 = U->cur, f1 = U->cur;
        if (U->place.stage) { f0 = U->place.f0 < U->place.top ? U->place.f0 : U->place.top; f1 = U->place.f0 < U->place.top ? U->place.top : U->place.f0; }
        else f1 = U->place.t == T_CARPARK ? ui_place_top() : U->cur + d->h - 1;
        int x = xseg(U->place.x), w = d->w * TW_SEG, y0 = yrow(f1), y1 = yrow(f0) + TW_FH;
        uint16_t c = s_ghost_why ? C(0xff4040) : C(0x40ff60);
        gfx_blend(x, y0, w, y1 - y0, c, 90);
        outline(x, y0, w, y1 - y0, c);
    } else {
        int y = yrow(U->cur);
        outline(TW_TX - 1, y, WORLD_W - TW_TX, TW_FH, gfx_mix(C(0xf0c020), C(0x000000), U->mode == M_PLAY ? 0 : 128));
    }
    if (U->mode == M_INSPECT && U->insp.n && (s_blink >> 3) & 1) {
        int it = U->insp.list[U->insp.i];
        int x, w, y0, y1;
        if (it >= 0) { const unit_t *u = &S->u[it]; x = xseg(u->x); w = u->w * TW_SEG; y1 = yrow(u->f) + TW_FH; y0 = y1 - tw_uh(u) * TW_FH; }
        else { const shaft_t *s = &S->s[-it - 1]; x = xseg(s->x); w = s->w * TW_SEG; y0 = yrow(U->cur); y1 = y0 + TW_FH; }
        outline(x - 1, y0, w + 2, y1 - y0, C(0xffffff));
        outline(x, y0 + 1, w, y1 - y0 - 2, C(0x000000));
    }
}

/* ------------------------------------------------------------ HUD, hint bar, minimap */
static void money(char *b, int n, int64_t v)
{
    int64_t a = v < 0 ? -v : v;
    if (a >= 10000000) snprintf(b, n, "$%s%lldM", v < 0 ? "-" : "", (long long)(a / 1000000));
    else if (a >= 10000) snprintf(b, n, "$%s%lldK", v < 0 ? "-" : "", (long long)(a / 1000));
    else snprintf(b, n, "$%lld", (long long)v);
}
static void draw_hud(void)
{
    rect(0, 0, SCREEN_W, TW_TY, C(0x101020));
    char b[32];
    money(b, sizeof b, S->funds);
    gfx_text(b, 3, 4, 1, S->funds < 0 ? C(0xff6060) : C(0x80ff80));
    for (int i = 0; i < 5; i++) {                                              // stars
        uint16_t c = i < S->star ? C(0xffd020) : C(0x404050);
        int sx = 70 + i * 9;
        rect(sx + 2, 3, 3, 7, c); rect(sx, 5, 7, 3, c);
    }
    if (S->core_done) gfx_text("CORE", 116, 4, 1, C(0xff8040));
    else if (S->tower_done) gfx_text("TOWER", 116, 4, 1, C(0xffd020));
    int m = (int)(S->tick / T_MIN);
    snprintf(b, sizeof b, "%s %02d:%02d", (const char *[]){"WD1", "WD2", "WE"}[S->day % 3], m / 60, m % 60);
    gfx_text(b, 152, 4, 1, C(0xe0e0f0));
    snprintf(b, sizeof b, "POP %ld", (long)S->pop);
    gfx_text(b, 222, 4, 1, C(0xa0c0ff));
    const char *sp = (const char *[]){"II", ">", ">>", ">>>"}[S->speed];
    gfx_text(sp, SCREEN_W - 3 - gfx_text_w(sp, 1), 4, 1, S->speed ? C(0xffffff) : C(0xff8040));
}
static void draw_hint(void)
{
    rect(0, HINT_Y, SCREEN_W, SCREEN_H - HINT_Y, C(0x101020));
    rect(0, HINT_Y, SCREEN_W, 1, C(0x303050));
    char b[64];
    const char *l2 = NULL;
    switch (U->mode) {
    case M_PLACE: {
        const tdef_t *d = &g_T[U->place.t];
        int64_t c = tw_cost(U->place.t, U->cur, U->place.stage ? U->place.top : U->cur);
        char m[16]; money(m, sizeof m, c);
        snprintf(b, sizeof b, "%s %s %s", U->place.stage ? "TOP:" : "PLACE", U->place.t == T_SHOP ? "SHOP" : d->zh, m);
        cjk_text(b, 4, HINT_Y + 2, 12, C(0xffffff));
        l2 = s_ghost_why ? s_ghost_why : U->place.stage ? "UP/DN: TOP FLOOR  OK: BUILD" : "UP/DN: MOVE  OK: BUILD  HOLD: DONE";
        gfx_text(l2, 4, HINT_Y + 15, 1, s_ghost_why ? C(0xff8080) : C(0x9090a0));
        return;
    }
    case M_INSPECT: {
        int it = U->insp.list[U->insp.i];
        if (it >= 0) {
            const unit_t *u = &S->u[it];
            if (u->build) snprintf(b, sizeof b, "%s BUILDING %dH %dM LEFT", g_T[u->t].en, u->build / 60, u->build % 60);
            else if (g_T[u->t].flags & F_COMMERCE) snprintf(b, sizeof b, "%s %s EVAL %d CUST/DAY %d", g_T[u->t].en, u->occ ? "OPEN" : "VACANT", u->eval, u->cust_y);
            else if (g_T[u->t].flags & F_TENANT) snprintf(b, sizeof b, "%s %s EVAL %d WAIT %d.%dM", g_T[u->t].en, u->occ ? "LET" : "VACANT", u->eval, u->last_wait / 10, u->last_wait % 10);
            else snprintf(b, sizeof b, "%s", g_T[u->t].en);
        } else {
            const shaft_t *s = &S->s[-it - 1];
            int mw; int n = tw_waiting(-it - 1, U->cur, &mw, NULL, NULL);
            snprintf(b, sizeof b, "%s %d CARS  WAITING %d", g_T[s->t].en, s->ncars, n);
        }
        gfx_text(b, 4, HINT_Y + 4, 1, C(0xffffff));
        gfx_text("UP/DN: NEXT  OK: OPTIONS  HOLD: BACK", 4, HINT_Y + 15, 1, C(0x9090a0));
        return;
    }
    default: break;
    }
    if (S->msg_t) { cjk_text(S->msg, 4, HINT_Y + 2, 12, C(0xffe080)); }
    else gfx_text("UP/DN: FLOOR  OK: MENU  HOLD OK: PAUSE", 4, HINT_Y + 4, 1, C(0x9090a0));
    snprintf(b, sizeof b, "AVG WAIT %d.%dM", (int)(S->st.wait_n ? S->st.wait_sum * 10 / S->st.wait_n / T_MIN / 10 : 0),
             (int)(S->st.wait_n ? S->st.wait_sum * 10 / S->st.wait_n / T_MIN % 10 : 0));
    gfx_text(b, 4, HINT_Y + 15, 1, C(0x707088));
}
static void draw_map(void)
{
    rect(MAP_X - 2, TW_TY, SCREEN_W - MAP_X + 2, TW_VIS * TW_FH, C(0x0c0c18));
    for (int f = 0; f < NF; f++) {
        int y = TW_TY + (NF - 1 - f) * (TW_VIS * TW_FH) / NF;
        if (y < TW_TY || y + 2 > TW_TY + TW_VIS * TW_FH) continue;
        uint64_t s = S->slab[f];
        if (!s) continue;
        int n = __builtin_popcountll(s);
        rect(MAP_X, y, 1 + n * 26 / W, 2, tw_is_lobby_floor(f) ? C(0xc0a040) : f < BASE ? C(0x806040) : C(0x7080a0));
    }
    int y0 = TW_TY + (NF - 1 - (U->cam + TW_VIS - 1)) * (TW_VIS * TW_FH) / NF, y1 = TW_TY + (NF - U->cam) * (TW_VIS * TW_FH) / NF;
    outline(MAP_X - 1, y0 < TW_TY ? TW_TY : y0, 30, y1 - y0, C(0xf0c020));
}

/* ------------------------------------------------------------ overlays */
static void panel(int x, int y, int w, int h, const char *title)
{
    rect(x, y, w, h, C(0x181830));
    outline(x, y, w, h, C(0xf0c020));
    rect(x + 1, y + 1, w - 2, 15, C(0x2a2a50));
    cjk_text(title, x + 5, y + 2, 12, C(0xffe080));
}
static void draw_menu(void)
{
    int n = U->menu.n, vis = n < 11 ? n : 11, first = U->menu.sel - vis / 2;
    if (first > n - vis) first = n - vis;
    if (first < 0) first = 0;
    int h = 20 + vis * 16, x = 28, y = (TW_TY + HINT_Y - h) / 2, w = 264;
    panel(x, y, w, h, U->menu.title);
    for (int i = 0; i < vis; i++) {
        const item_t *it = &U->menu.it[first + i];
        int iy = y + 18 + i * 16;
        bool sel = first + i == U->menu.sel;
        if (sel) rect(x + 2, iy, w - 4, 15, C(0xf0c020));
        uint16_t c = sel ? C(0x000000) : it->dis ? C(0x707080) : C(0xffffff);
        cjk_text(it->label, x + 6, iy + 1, 12, c);
        if (it->sub[0]) gfx_text(it->sub, x + w - 6 - gfx_text_w(it->sub, 1), iy + 4, 1, sel ? C(0x000000) : C(0x80ff80));
    }
    for (int i = 0; i < 4; i++) {                                           // scroll arrows
        if (first > 0) rect(x + w / 2 - i, y + 13 + i, 2 * i + 1, 1, C(0xf0c020));
        if (first + vis < n) rect(x + w / 2 - i, y + h - 1 - i, 2 * i + 1, 1, C(0xf0c020));
    }
}
static void draw_status(void)
{
    panel(24, 24, 272, 184, "TOWER STATUS");
    char b[64], m[16];
    int y = 46;
    money(m, sizeof m, S->funds);
    snprintf(b, sizeof b, "FUNDS %s   POP %ld   DAY %ld", m, (long)S->pop, (long)S->day + 1);
    gfx_text(b, 32, y, 1, C(0xffffff)); y += 12;
    money(m, sizeof m, S->st.rent_q);
    snprintf(b, sizeof b, "LAST QUARTER RENT %s", m);
    gfx_text(b, 32, y, 1, C(0xc0c0d0)); y += 12;
    snprintf(b, sizeof b, "ARRIVED %lld  NO ROUTE %lld", (long long)S->st.arrived, (long long)S->st.failed);
    gfx_text(b, 32, y, 1, C(0xc0c0d0)); y += 16;
    if (S->star < 6) {
        snprintf(b, sizeof b, "FOR %s:", S->star >= 5 ? "TOWER" : (const char *[]){"", "2 STARS", "3 STARS", "4 STARS", "5 STARS"}[S->star]);
        gfx_text(b, 32, y, 1, C(0xffe080)); y += 12;
        const char *nm[8]; bool ok[8];
        int k = tw_star_reqs(S->star + 1, nm, ok);
        for (int i = 0; i < k; i++) {
            rect(36, y + 1, 5, 5, ok[i] ? C(0x40ff60) : C(0xff4040));
            gfx_text(nm[i], 46, y, 1, ok[i] ? C(0x80ff80) : C(0xffffff)); y += 11;
        }
    } else gfx_text("TOWER RATING! THANK YOU.", 32, y, 1, C(0xffe080));
    gfx_text("OK: CLOSE", 32, 194, 1, C(0x9090a0));
}
static void draw_news(void)
{
    panel(16, 24, 288, 184, "NEWS");
    for (int i = 0; i < S->n_log && i < 12; i++) cjk_text(S->log[i], 22, 42 + i * 13, 12, i ? C(0xc0c0d0) : C(0xffffff));
}
static void draw_end(void)
{
    static const uint32_t FW[5] = {0xff4060, 0xffd040, 0x40e0ff, 0x80ff60, 0xff80ff};
    for (int k = 0; k < 5; k++) {                                                      // fireworks over the tower
        int ph = (int)((U->frame >> 1) + k * 13) % 40, gen = (int)((U->frame >> 1) + k * 13) / 40;
        int cx = 40 + (k * 61 + gen * 37) % 240, cy = 30 + (k * 29 + gen * 17) % 80;
        if (ph < 8) { rect(cx, cy + 60 - ph * 7, 2, 4, C(0xfff0c0)); continue; }      // rocket going up
        int r = (ph - 8) * 3 / 2;
        if (r > 34) continue;
        uint16_t c = C(FW[k]);
        for (int i = 0; i < 16; i++) {
            int a = i * 4;
            int px = cx + r * SIN64[a & 63] / 127, py = cy + r * SIN64[(a + 16) & 63] / 127 + r * r / 120;   // a little gravity
            rect(px, py, 2, 2, r > 26 ? gfx_mix(c, C(0x101020), 140) : c);
        }
    }
    bool core = U->end_kind == 2, ask = U->end_kind == 1 && !S->deep;
    panel(40, ask ? 108 : 128, 240, ask ? 100 : 80, core ? "CORE RATING!" : "TOWER RATING!");
    int py = ask ? 128 : 148;
    cjk_text(core ? "地心寶庫" : "摩天大廈", 52, py, 12, C(0xffd040));
    gfx_text(core ? "B100 CORE VAULT OPEN" : "SKYRISE COMPLETE", 112, py + 3, 1, C(0xffffff));
    if (ask) {
        char b2[48];
        snprintf(b2, sizeof b2, "DAY %d  POP %d", (int)S->day + 1, (int)S->pop);
        gfx_text(b2, 52, 146, 1, C(0xc0c0d0));
        gfx_text("CONTINUE CHALLENGE?", 52, 160, 1, C(0xffffff));
        gfx_text("DIG 100 FLOORS UNDERGROUND", 52, 171, 1, C(0xff9050));
        static const char *O[2] = {"YES, DIG!", "KEEP BUILDING"};
        for (int i = 0; i < 2; i++) {
            bool sel = U->end_sel == i;
            if (sel) rect(50, 183 + i * 11, 220, 10, C(0x3a3010));
            gfx_text(O[i], 64, 184 + i * 11, 1, sel ? C(0xffd040) : C(0x9090a0));
            if (sel) gfx_text(">", 54, 184 + i * 11, 1, C(0xffd040));
        }
        return;
    }
    char b[48];
    if (S->funds >= 10000000) snprintf(b, sizeof b, "DAY %d  POP %d  $%dM", (int)S->day + 1, (int)S->pop, (int)(S->funds / 1000000));
    else snprintf(b, sizeof b, "DAY %d  POP %d  $%dK", (int)S->day + 1, (int)S->pop, (int)(S->funds / 1000));
    gfx_text(b, 52, 168, 1, C(0xc0c0d0));
    gfx_text(core ? "OK: KEEP PLAYING" : "OK: KEEP BUILDING", 52, 186, 1, (U->frame >> 4) & 1 ? C(0xffd040) : C(0xa08020));
}
static void draw_title(void)
{
    rect(0, 0, SCREEN_W, SCREEN_H, C(0x101838));
    for (int i = 0; i < 40; i++) rect((i * 97) % 320, (i * 61) % 140, 1, 1, C(0xc0c0ff));
    for (int i = 0; i < 14; i++) {                                           // skyline
        int bw = 14 + (i * 7) % 12, bh = 40 + (i * 37) % 90, bx = i * 24 - 6;
        rect(bx, 240 - bh, bw, bh, C(0x202848));
        for (int wy = 240 - bh + 4; wy < 236; wy += 6) for (int wx = bx + 2; wx < bx + bw - 2; wx += 4) if (((wx * 7 + wy * 3) >> 2) % 3 == 0) rect(wx, wy, 2, 3, C(0xffd070));
    }
    rect(138, 40, 44, 200, C(0x2a3458));                                      // the hero tower
    for (int wy = 46; wy < 236; wy += 8) for (int wx = 142; wx < 178; wx += 6) rect(wx, wy, 3, 4, ((wx + wy) / 6) % 4 ? C(0xffe090) : C(0x405080));
    rect(158, 22, 4, 18, C(0xc0c0c0));
    if ((U->frame >> 4) & 1) rect(158, 20, 4, 3, C(0xff3030));
    // 摩天大廈 at 24 px: each glyph pixel drawn 2x2
    const char *t = "摩天大廈";
    int x = 160 - 4 * 25 / 2 - 2;
    while (*t) {
        uint32_t cp = utf8_next(&t);
        int gi = cjk_find(cp);
        if (gi < 0) continue;
        for (int r = 0; r < 12; r++) for (int c = 0; c < 12; c++) if ((g_cjk_bits[gi][r] >> (15 - c)) & 1) { rect(x + c * 2 + 1, 70 + r * 2 + 1, 2, 2, C(0x000000)); rect(x + c * 2, 70 + r * 2, 2, 2, C(0xffd040)); }
        x += 26;
    }
    gfx_text("SKYRISE", 160 - gfx_text_w("SKYRISE", 2) / 2, 100, 2, C(0xffffff));
    if (U->mode == M_TITLE) {   // no menu open yet: hint
        rect(60, 160, 200, 34, C(0x101020));
        outline(60, 160, 200, 34, C(0xf0c020));
        textc("OK: SAVE / LOAD", 160, 172, C(0xffffff));
    }
}
static void draw_overlay(void)
{
    switch (U->mode) {
    case M_MENU: draw_menu(); break;
    case M_STATUS: draw_status(); break;
    case M_NEWS: draw_news(); break;
    case M_END: draw_end(); break;
    default: break;
    }
}

/* ------------------------------------------------------------ entry points */
void draw_prepare(void)
{
    int h10 = tw_hour_x10();
    s_nt = tw_night();
    s_dusk = h10 < 50 ? 256 : h10 < 70 ? 256 - (h10 - 50) * 256 / 20 : h10 < 175 ? 0 : h10 < 195 ? (h10 - 175) * 256 / 20 : 256;
    s_shade = C(0x101830);
    s_blink = (int)(U->frame >> 1);
    s_ghost_why = NULL;
    if (U->mode == M_PLACE) {
        if (U->place.stage) {
            int f0 = U->place.f0 < U->place.top ? U->place.f0 : U->place.top, f1 = U->place.f0 < U->place.top ? U->place.top : U->place.f0;
            s_ghost_why = tw_can_place(U->place.t, f0, U->place.x, f1);
        } else s_ghost_why = tw_can_place(U->place.t, U->cur, U->place.x, ui_place_top());
    }
}
void draw_screen(void)
{
    if (U->mode == M_TITLE || (U->mode == M_MENU && U->menu.back == M_TITLE)) {
        draw_title();
        if (U->mode == M_MENU) draw_menu();
        return;
    }
    int by0 = gfx_band_y0(), by1 = by0 + gfx_band_rows(), bx1 = gfx_band_x0() + gfx_band_w();
    if (by0 < TW_TY) draw_hud();
    if (by1 > TW_TY && by0 < TW_TY + TW_VIS * TW_FH) {
        if (gfx_band_x0() < WORLD_W) draw_world();
        if (bx1 > WORLD_W) draw_map();
    }
    if (by1 > HINT_Y) draw_hint();
    draw_overlay();
}

/* ------------------------------------------------------------ signatures */
#define LIVE_U(u) ((u)->build == 0)
static uint32_t mix(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t base_sig(void)
{
    uint32_t h = 2166136261u;
    h = mix(h, (uint32_t)U->cam); h = mix(h, (uint32_t)s_dusk); h = mix(h, s_nt);
    h = mix(h, S->typhoon || S->rain_h || S->quake_h || S->power_h || S->fest ? U->frame : 0);
    h = mix(h, U->mode); h = mix(h, S->tower_done);
    return h;
}
uint32_t draw_sig_row(int r)
{
    int f = U->cam + TW_VIS - 1 - r;
    uint32_t h = mix(base_sig(), (uint32_t)f);
    h = mix(h, (uint32_t)(f == U->cur));
    h = mix(h, (uint32_t)(S->slab[f] ^ S->slab[f] >> 32));
    bool anim = false, fast = false;
    for (int ff = f; ff >= 0 && ff > f - MAX_H; ff--) {
        for (uint16_t i = S->floor_head[ff]; i != NO_ID; i = S->u[i].next) {
            const unit_t *u = &S->u[i];
            if (ff + tw_uh(u) - 1 < f) continue;
            h = mix(h, i);
            h = mix(h, (uint32_t)(u->occ | u->dirty << 1 | u->roach << 2 | (u->inside > 0) << 3 | (u->inside > 3) << 4 | (u->stay > 0) << 5));
            h = mix(h, (uint32_t)(u->build ? 1 + u->build * 20 / (g_T[u->t].build_min ? g_T[u->t].build_min : 1) : 0));
            h = mix(h, (uint32_t)(u->cust_y / 10) ^ (uint32_t)u->inside << 8);
            if (g_T[u->t].flags & F_COMMERCE) { h = mix(h, tw_is_open(u)); anim |= u->occ && (s_nt || u->t == T_CINEMA || u->t == T_SHOP); if ((u->t == T_MEGA || u->t == T_COASTER) && u->occ && tw_is_open(u)) fast = true; }
            if (u->t == T_REFUGE && s_nt) anim = true;
            if (u->t >= T_CARPARK && u->t <= T_STUDIO && LIVE_U(u)) anim = true;   // slow loops (every 8 frames)
            if (u->t >= T_PUMP && u->t <= T_VAULT && LIVE_U(u)) fast = true;          // pumps, trains, fish: every 2 frames
            if (u->fire_h || u->roach || u->t == T_SECURITY) anim = true;
        }
    }
    if (f < BASE) {                                                         // underground row: dug state, drill, magma, flood
        h = mix(h, (uint32_t)(f < S->dug)); h = mix(h, (uint32_t)S->dug);
        if (f == S->dug - 1 && S->dig_h) { h = mix(h, S->dig_h); fast = true; }
        if (tw_depth(f) > 85) anim = true;
        if (f == S->flood_f && S->flood_h) { h = mix(h, S->flood_h); anim = true; }
    }
    if (S->rain_h || S->typhoon_sig || S->quake_h || S->power_h || S->fest || S->santa) anim = true;
    if (anim) h = mix(h, (uint32_t)(s_blink >> 2));
    if (fast) h = mix(h, (uint32_t)s_blink);          // coaster train / film: only these bands every frame   // neon, marquee, flames: every 8 frames
    for (int i = 0; i < S->n_shafts; i++) { h = mix(h, (uint32_t)(S->s[i].build * 20 / g_T[S->s[i].t].build_min)); h = mix(h, S->s[i].ncars); }
    if (U->mode == M_PLACE) { h = mix(h, (uint32_t)U->place.x); h = mix(h, (uint32_t)U->place.top); h = mix(h, (uint32_t)U->place.t); h = mix(h, s_ghost_why != NULL); }
    if (U->mode == M_INSPECT) { h = mix(h, (uint32_t)U->insp.i); h = mix(h, (uint32_t)(s_blink >> 3)); }
    return h;
}
uint32_t draw_sig_shaft(int si, int r)
{
    const shaft_t *s = &S->s[si];
    int f = U->cam + TW_VIS - 1 - r;
    uint32_t h = mix(base_sig(), (uint32_t)(si * 131 + f));
    int ytop = TW_TY + r * TW_FH;
    int mw; bool up, dn;
    int n = tw_waiting(si, f, &mw, &up, &dn);
    h = mix(h, (uint32_t)((n + 2) / 3)); h = mix(h, (uint32_t)(up | dn << 1 | (mw > 48) << 2 | (mw > 120) << 3));
    for (int k = 0; k < s->ncars; k++) {
        const car_t *c = &s->car[k];
        int cy = TW_TY + (int)(((int32_t)(U->cam + TW_VIS - 1) * 256 - c->disp) * TW_FH / 256);
        if (cy - 6 >= ytop + TW_FH || cy + TW_FH <= ytop) continue;   // the car is not in this row
        h = mix(h, (uint32_t)cy); h = mix(h, c->door_a); h = mix(h, c->load);
        for (int i = 0; i < c->n_anim; i++) h = mix(h, (uint32_t)(c->anim_t[i] + 64 * c->anim_out[i]));
    }
    for (int k = 0; k < s->ncars; k++) {                              // walkers to/from a car at this floor
        const car_t *c = &s->car[k];
        if (c->n_anim && c->hold_f == f) for (int i = 0; i < c->n_anim; i++) h = mix(h, (uint32_t)c->anim_t[i]);
    }
    return h;
}
int draw_shaft_win(int si, int *x0)
{
    const shaft_t *s = &S->s[si];
    int a = xseg(s->x) - 32, b = xseg(s->x) + s->w * TW_SEG + 2;
    if (a < TW_TX - 2) a = TW_TX - 2;
    if (b > WORLD_W) b = WORLD_W;
    *x0 = a;
    return b - a;
}
uint32_t draw_sig_hud(void)
{
    uint32_t h = mix(2166136261u, (uint32_t)(S->funds / 1000));
    h = mix(h, S->star); h = mix(h, (uint32_t)(S->tick / T_MIN)); h = mix(h, (uint32_t)S->pop); h = mix(h, S->speed); h = mix(h, S->tower_done);
    h = mix(h, U->mode);
    h = mix(h, S->msg_t ? (uint32_t)S->msg[0] * 7 + (uint32_t)S->msg[3] * 31 + (uint32_t)strlen(S->msg) + 1000 : 0);
    if (U->mode == M_PLACE) { h = mix(h, (uint32_t)U->place.x); h = mix(h, (uint32_t)U->place.top); h = mix(h, (uint32_t)(uintptr_t)s_ghost_why); h = mix(h, (uint32_t)U->cur); }
    if (U->mode == M_INSPECT && U->insp.n) {
        int it = U->insp.list[U->insp.i];
        h = mix(h, (uint32_t)it);
        if (it >= 0) { const unit_t *u = &S->u[it]; h = mix(h, (uint32_t)(u->eval | u->cust_y << 8 | u->build << 16 | (uint32_t)u->occ << 31)); }
        else h = mix(h, (uint32_t)tw_waiting(-it - 1, U->cur, NULL, NULL, NULL));
    }
    h = mix(h, (uint32_t)(S->st.wait_n ? S->st.wait_sum * 10 / S->st.wait_n / T_MIN : 0));
    return h;
}
uint32_t draw_sig_map(void)
{
    uint32_t h = mix(2166136261u, (uint32_t)U->cam);
    for (int f = 0; f < NF; f++) h = mix(h, (uint32_t)__builtin_popcountll(S->slab[f]));
    return h;
}
uint32_t draw_sig_overlay(void)
{
    if (U->mode == M_TITLE) return mix(1, (uint32_t)(U->frame >> 4));
    if (U->mode == M_END) return mix(mix(2, (uint32_t)(U->frame >> 2)), U->end_sel);   // fireworks: whole screen every 4 frames
    if (U->mode != M_MENU && U->mode != M_STATUS && U->mode != M_NEWS) return 0;
    uint32_t h = mix(2166136261u, U->mode);
    h = mix(h, U->menu.sel); h = mix(h, U->menu.n); h = mix(h, (uint32_t)U->menu.title[0] * 131 + U->menu.title[5]);
    for (int i = 0; i < U->menu.n; i++) h = mix(h, (uint32_t)U->menu.it[i].label[0] + U->menu.it[i].label[8] * 7u + U->menu.it[i].label[13] * 31u);
    h = mix(h, (uint32_t)(S->funds / 1000)); h = mix(h, S->n_log);
    return h;
}
