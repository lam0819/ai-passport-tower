// The ~60 Chinese characters the game shows, as 12x12 bitmaps in flash.
// Menus, notices and the title only; signboards are English + pictures.
#pragma once
#include <stdint.h>

extern const int g_cjk_n;
extern const uint16_t g_cjk_cp[];          // sorted code points
extern const uint16_t g_cjk_bits[][12];    // row bits, MSB = left pixel

// Draws UTF-8 text: CJK from the table, ASCII with the 5x7 font. size 12 or 10.
int cjk_text(const char *utf8, int x, int y, int size, uint16_t color);
int cjk_text_w(const char *utf8, int size);
