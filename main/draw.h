// Procedural art for SKYRISE. Every function draws into the current band
// (gfx clips), so the same code paints a full screen, one floor row, or a
// narrow window around one lift shaft.
#pragma once
#include <stdint.h>

void draw_prepare(void);              // once per frame, before any band
void draw_screen(void);               // everything that overlaps the band
// dirty-region signatures (game.c repaints only what changed)
uint32_t draw_sig_row(int r);         // world row r (0 = top), shafts excluded
uint32_t draw_sig_shaft(int si, int r);
uint32_t draw_sig_hud(void);
uint32_t draw_sig_map(void);
uint32_t draw_sig_overlay(void);      // 0 = no overlay (menus, panels, title)
int draw_shaft_win(int si, int *x0);  // the window around a shaft: returns width
