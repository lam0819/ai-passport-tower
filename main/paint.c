#include "paint.h"

#include "config.h"
#include "draw.h"
#include "gfx.h"
#include "tower.h"
#include "ui.h"

static uint32_t s_row[TW_VIS], s_sh[MAX_SHAFTS][TW_VIS], s_hud, s_map, s_ov;
static bool s_valid;

static int paint(paint_send_t send, int x0, int w, int y0, int y1)
{
    int rows_max = SCREEN_W * BAND_ROWS / w, px = 0;
    for (int y = y0; y < y1; y += rows_max) {
        int rows = y1 - y < rows_max ? y1 - y : rows_max;
        gfx_band_begin_win(x0, w, y, rows, 0);
        draw_screen();
        send(x0, y, w, gfx_band_buf(), rows);
        px += w * rows;
    }
    return px;
}
int paint_frame(paint_send_t send, bool full)
{
    draw_prepare();
    uint32_t ov = draw_sig_overlay();
    bool title = g_ui.mode == M_TITLE;
    if (title || ov) {   // title or a menu / panel over a paused world: repaint all on change
        bool ch = full || !s_valid || ov != s_ov;
        s_ov = ov; s_valid = false;   // the world underneath will be repainted after
        return ch ? paint(send, 0, SCREEN_W, 0, SCREEN_H) : 0;
    }
    if (!s_valid || s_ov) full = true;
    s_ov = 0;
    int px = 0;
    uint32_t h = draw_sig_hud();
    if (full || h != s_hud) { s_hud = h; px += paint(send, 0, SCREEN_W, 0, TW_TY) + paint(send, 0, SCREEN_W, HINT_Y, SCREEN_H); }
    h = draw_sig_map();
    if (full || h != s_map) { s_map = h; px += paint(send, 288, SCREEN_W - 288, TW_TY, TW_TY + TW_VIS * TW_FH);   /* 32 x 192: one band */ }
    for (int r = 0; r < TW_VIS; r++) {
        int y = TW_TY + r * TW_FH;
        h = draw_sig_row(r);
        if (full || h != s_row[r]) {
            s_row[r] = h;
            px += paint(send, 0, 288, y, y + TW_FH);
            for (int i = 0; i < g_tw.n_shafts; i++) s_sh[i][r] = draw_sig_shaft(i, r);
            continue;
        }
        for (int i = 0; i < g_tw.n_shafts; i++) {
            h = draw_sig_shaft(i, r);
            if (h == s_sh[i][r]) continue;
            s_sh[i][r] = h;
            int x0, w = draw_shaft_win(i, &x0);
            px += paint(send, x0, w, y, y + TW_FH);
        }
    }
    s_valid = true;
    return px;
}
