#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "tower.h"

ui_t g_ui;
#define U (&g_ui)
#define S (&g_tw)

enum { A_NONE, A_AGAIN, A_BUILDMENU, A_INSPECT, A_SPEED, A_SHOP, A_RUSH, A_STATUS, A_NEWS, A_GO_G, A_GO_TOP, A_SAVE_TITLE,
       A_PICK, A_BUY, A_BACK, A_CAR, A_FINISH, A_RENT, A_DEMOLISH, A_DEMO_YES, A_CLOSE, A_SOUND, A_DIG, A_SAVE1, A_SAVE2, A_LOAD1, A_LOAD2, A_NEW, A_TITLE, A_REPAIR_PWR, A_REPAIR_LIFT };
static const char *const SHOP_EN[6] = {"CONVENIENCE", "PHARMACY", "GOLDSMITH", "BAKERY", "ELECTRONICS", "FASHION"};
static const int TPF[4] = {0, 4, 8, 16};

static void sfx(int w) { if (U->sfx_cb) U->sfx_cb(w); }
static void kfmt(char *b, int n, int64_t v)
{
    if (v >= 1000000 || v <= -1000000) snprintf(b, n, "%lld.%lldM", (long long)(v / 1000000), (long long)((v < 0 ? -v : v) % 1000000 / 100000));
    else if (v >= 1000 || v <= -1000) snprintf(b, n, "%lldK", (long long)(v / 1000));
    else snprintf(b, n, "%lld", (long long)v);
}
static int top_floor(void)
{
    for (int f = NF - 1; f >= 0; f--) if (S->slab[f]) return f > BASE ? f : BASE;
    return BASE;
}
void ui_follow(void)
{
    if (U->cur < U->cam) U->cam = U->cur;
    if (U->cur > U->cam + TW_VIS - 1) U->cam = U->cur - TW_VIS + 1;
    if (U->cam < 0) U->cam = 0;
    if (U->cam > NF - TW_VIS) U->cam = NF - TW_VIS;
}
static item_t *add(const char *label, const char *sub, int act, int arg, int arg2, bool dis)
{
    if (U->menu.n >= MAX_ITEMS) return NULL;
    item_t *it = &U->menu.it[U->menu.n++];
    snprintf(it->label, sizeof it->label, "%s", label);
    snprintf(it->sub, sizeof it->sub, "%s", sub ? sub : "");
    it->act = (uint8_t)act; it->arg = (int16_t)arg; it->arg2 = (int16_t)arg2; it->dis = dis;
    return it;
}
static void open_menu(const char *title, int back)
{
    snprintf(U->menu.title, sizeof U->menu.title, "%s", title);
    U->menu.n = 0; U->menu.sel = 0; U->menu.back = (uint8_t)back;
    U->mode = M_MENU;
}
static const char *tname(int t, int v)
{
    static const char *PK[3] = {"PARKING 2F", "PARKING 3F", "PARKING 4F"};
    if (t == T_CARPARK && v >= 2 && v <= 4) return PK[v - 2];
    return t == T_SHOP && v >= 0 && v < 6 ? SHOP_EN[v] : g_T[t].en;
}
int ui_place_top(void) { return U->place.t == T_CARPARK ? U->cur + (U->place.v >= 2 && U->place.v <= 4 ? U->place.v : 2) - 1 : -1; }
static void title_menu(void)
{
    char sub1[24], sub2[24], lab[34];
    if (U->slot_info_cb) { U->slot_info_cb(0, sub1, sizeof sub1); U->slot_info_cb(1, sub2, sizeof sub2); }
    else { snprintf(sub1, sizeof sub1, "EMPTY"); snprintf(sub2, sizeof sub2, "EMPTY"); }
    open_menu("SKYRISE", M_TITLE);
    snprintf(lab, sizeof lab, "LOAD 1  %s", sub1); add(lab, NULL, A_LOAD1, 0, 0, strcmp(sub1, "EMPTY") == 0);
    snprintf(lab, sizeof lab, "LOAD 2  %s", sub2); add(lab, NULL, A_LOAD2, 0, 0, strcmp(sub2, "EMPTY") == 0);
    add("NEW TOWER", NULL, A_NEW, 0, 0, false);
}
static void main_menu(void)
{
    char t[40], fn[8];
    snprintf(t, sizeof t, "%s/F  %s Q%d Y%d", tw_floor_name(U->cur, fn), (const char *[]){"WD1", "WD2", "WE"}[S->day % 3],
             (int)(S->day / 3 % 4) + 1, (int)(S->day / 12) + 1);
    open_menu(t, M_PLAY);
    char b[34];
    snprintf(b, sizeof b, "BUILD AGAIN: %s", tname(S->last_t, S->last_v == 0xff ? -1 : S->last_v));
    add(b, NULL, A_AGAIN, 0, 0, false);
    add("BUILD...", NULL, A_BUILDMENU, 0, 0, false);
    snprintf(b, sizeof b, "INSPECT %s/F", tw_floor_name(U->cur, fn));
    add(b, NULL, A_INSPECT, 0, 0, false);
    snprintf(b, sizeof b, "SPEED: %s", (const char *[]){"PAUSE", "1X", "2X", "4X"}[S->speed]);
    add(b, NULL, A_SPEED, 0, 0, false);
    add("SPEED SHOP...", NULL, A_SHOP, 0, 0, false);
    snprintf(b, sizeof b, "RUSH BUILD: %s", S->rush ? "ON +25%" : "OFF");
    for (char *p = b; *p; p++) if (*p == '%') *p = ' ';   // the font has no percent sign
    if (S->rush) strcat(b, "PCT");
    add(b, NULL, A_RUSH, 0, 0, false);
    add((const char *[]){"SOUND: MUSIC + FX", "SOUND: FX ONLY", "SOUND: OFF"}[U->sound % 3], NULL, A_SOUND, 0, 0, false);
    if (S->deep && S->dug > 0) {
        char fn2[8], c[10];
        tw_floor_name(S->dug - 1, fn2);
        if (S->dig_h) { kfmt(c + 1, 9, tw_dig_cost() * RUSH_PCT / 100); c[0] = '$'; snprintf(b, sizeof b, "RUSH DIG %s (%dH LEFT)", fn2, S->dig_h); }
        else { kfmt(c + 1, 9, tw_dig_cost()); c[0] = '$'; snprintf(b, sizeof b, "DIG %s (%dH)", fn2, tw_dig_hours()); }
        add(b, c, A_DIG, 0, 0, false);
    }
    add("TOWER STATUS", NULL, A_STATUS, 0, 0, false);
    add("NEWS", NULL, A_NEWS, 0, 0, false);
    add("GO TO G/F", NULL, A_GO_G, 0, 0, false);
    add("GO TO TOP", NULL, A_GO_TOP, 0, 0, false);
    if (S->power_h) { char c[10]; kfmt(c + 1, 9, tw_repair_power_cost()); c[0] = '$'; add("REPAIR POWER", c, A_REPAIR_PWR, 0, 0, false); }
    if (S->broken_si != 0xff) { char c[10]; kfmt(c + 1, 9, tw_repair_lift_cost()); c[0] = '$'; add("REPAIR LIFT", c, A_REPAIR_LIFT, 0, 0, false); }
    {
        char sub1[24], sub2[24], lab[34];
        if (U->slot_info_cb) { U->slot_info_cb(0, sub1, sizeof sub1); U->slot_info_cb(1, sub2, sizeof sub2); }
        else { snprintf(sub1, sizeof sub1, "-"); snprintf(sub2, sizeof sub2, "-"); }
        snprintf(lab, sizeof lab, "SAVE 1  %s", sub1); add(lab, NULL, A_SAVE1, 0, 0, false);
        snprintf(lab, sizeof lab, "SAVE 2  %s", sub2); add(lab, NULL, A_SAVE2, 0, 0, false);
        snprintf(lab, sizeof lab, "LOAD 1  %s", sub1); add(lab, NULL, A_LOAD1, 0, 0, strcmp(sub1, "EMPTY") == 0);
        snprintf(lab, sizeof lab, "LOAD 2  %s", sub2); add(lab, NULL, A_LOAD2, 0, 0, strcmp(sub2, "EMPTY") == 0);
    }
    add("TITLE", NULL, A_TITLE, 0, 0, false);
}
static void build_menu(void)
{
    char t[40], fn[8], b[34], c[10];
    snprintf(t, sizeof t, "BUILD ON %s/F", tw_floor_name(U->cur, fn));
    open_menu(t, M_PLAY);
    int sel = 0;
    for (int k = 0; k < T_COUNT; k++) {
        int ty = g_build_order[k];
        const tdef_t *d = &g_T[ty];
        if (d->dmin && !S->deep) continue;            // underground sights appear with the challenge
        bool lock = d->star > S->star || (d->dmin && tw_depth(S->dug) < d->dmin);   // greyed until dug that deep
        kfmt(c + 1, 9, (d->flags & F_SHAFT) ? d->cost + d->car : d->cost); c[0] = '$';
        for (int v = 0; v < (ty == T_SHOP ? 6 : ty == T_CARPARK ? 3 : 1); v++) {
            int vv = ty == T_SHOP ? v : ty == T_CARPARK ? v + 2 : -1;
            char pre[8] = "";
            if (lock) { if (d->dmin) snprintf(pre, sizeof pre, "B%d ", d->dmin); else snprintf(pre, sizeof pre, "*%d ", d->star); }
            snprintf(b, sizeof b, "%s%s %s", pre,
                     ty == T_SHOP ? "SHOP:" : d->zh, tname(ty, vv));
            if (ty == S->last_t && (vv < 0 || vv == S->last_v)) sel = U->menu.n;
            add(b, c, A_PICK, ty, vv, lock);
        }
    }
    U->menu.sel = (uint8_t)sel;
}
static void speed_shop(void)
{
    open_menu("SPEED SHOP: IN-GAME $ ONLY", M_PLAY);
    for (int i = 2; i <= 3; i++) {
        char b[34], c[10];
        bool lock = i == 3 && S->star < 3;
        snprintf(b, sizeof b, "%s%s CLOCK", S->owned[i] ? "OWNED: " : lock ? "*3 " : "", i == 2 ? "2X" : "4X");
        snprintf(c, sizeof c, "%s", S->owned[i] ? "" : i == 2 ? "$300K" : "$1.0M");
        add(b, c, A_BUY, i, 0, S->owned[i] || lock);
    }
    add("BACK", NULL, A_CLOSE, 0, 0, false);
}
static void start_place(int t, int v)
{
    S->last_t = (uint8_t)t; S->last_v = (uint8_t)(v < 0 ? 0xff : v);
    const tdef_t *d = &g_T[t];
    int f = U->cur;
    if (d->where == WH_TOP) f = NF - d->h;
    if (d->where == WH_METRO) f = 0;
    if (d->where == WH_LOBBY && !tw_is_lobby_floor(f)) f = BASE;
    U->cur = f; ui_follow();
    U->place.t = t; U->place.v = v; U->place.stage = 0; U->place.top = f; U->place.f0 = f;
    U->place.x = tw_find_spot(t, f, U->x);
    U->mode = M_PLACE;
}
static void fill_insp(void)
{
    int n = 0;
    int16_t tmp[24]; int xs[24];
    for (int ff = U->cur; ff >= 0 && ff > U->cur - 4; ff--) {
        for (uint16_t i = S->floor_head[ff]; i != NO_ID && n < 24; i = S->u[i].next) {
            if (ff + g_T[S->u[i].t].h - 1 >= U->cur) { tmp[n] = (int16_t)i; xs[n++] = S->u[i].x; }
        }
    }
    for (int i = 0; i < S->n_shafts && n < 24; i++) if (U->cur >= S->s[i].f0 && U->cur <= S->s[i].f1) { tmp[n] = (int16_t)-(i + 1); xs[n++] = S->s[i].x; }
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && xs[j] < xs[j - 1]; j--) {   // sort by x
        int tx = xs[j]; xs[j] = xs[j - 1]; xs[j - 1] = tx;
        int16_t tl = tmp[j]; tmp[j] = tmp[j - 1]; tmp[j - 1] = tl;
    }
    memcpy(U->insp.list, tmp, sizeof tmp);
    U->insp.n = n;
    if (U->insp.i >= n) U->insp.i = 0;
}
static void unit_menu(void)
{
    int it = U->insp.list[U->insp.i];
    char c[10], b[34];
    if (it < 0) {
        int si = -it - 1;
        const shaft_t *s = &S->s[si];
        char f0[8], f1[8];
        snprintf(b, sizeof b, "%s %s-%s CARS %d/4", g_T[s->t].en, tw_floor_name(s->f0, f0), tw_floor_name(s->f1, f1), s->ncars);
        open_menu(b, M_INSPECT);
        U->menu.ctx = (int16_t)it;
        if (s->build) { kfmt(c + 1, 9, tw_finish_price(-1, si)); c[0] = '$'; add("FINISH NOW", c, A_FINISH, -1, si, false); }
        kfmt(c + 1, 9, g_T[s->t].car); c[0] = '$';
        add("ADD CAR", c, A_CAR, si, 0, false);
        add("BACK", NULL, A_CLOSE, 0, 0, false);
        return;
    }
    const unit_t *u = &S->u[it];
    snprintf(b, sizeof b, "%s %s", u->t == T_SHOP ? "SHOP:" : g_T[u->t].zh, tname(u->t, u->t == T_SHOP ? u->v : -1));
    open_menu(b, M_INSPECT);
    U->menu.ctx = (int16_t)it;
    if (u->build) { kfmt(c + 1, 9, tw_finish_price(it, -1)); c[0] = '$'; add("FINISH NOW", c, A_FINISH, it, -1, false); }
    if (g_T[u->t].flags & F_TENANT) { snprintf(b, sizeof b, "RENT: %s", (const char *[]){"LOW", "NORMAL", "HIGH"}[u->rent]); add(b, NULL, A_RENT, it, 0, false); }
    kfmt(c + 1, 9, g_T[u->t].cost / 10); c[0] = '$';
    add("DEMOLISH", c, A_DEMOLISH, it, 0, false);
    add("BACK", NULL, A_CLOSE, 0, 0, false);
}
static void result(const char *why, const char *ok_msg, int good_sfx)
{
    if (why) { tw_note(why); sfx(2); } else { if (ok_msg) tw_note(ok_msg); sfx(good_sfx); }
}
static void do_item(const item_t *it)
{
    char b[48];
    switch (it->act) {
    case A_AGAIN: start_place(S->last_t, S->last_v == 0xff ? -1 : S->last_v); break;
    case A_BUILDMENU: build_menu(); break;
    case A_INSPECT: fill_insp(); if (U->insp.n) { U->insp.i = 0; U->mode = M_INSPECT; } else { tw_note("NOTHING ON THIS FLOOR"); U->mode = M_PLAY; } break;
    case A_SPEED: { int sel = U->menu.sel; do S->speed = (uint8_t)((S->speed + 1) % 4); while (S->speed && !S->owned[S->speed]); main_menu(); U->menu.sel = (uint8_t)sel; break; }
    case A_SHOP: speed_shop(); break;
    case A_RUSH: { int sel = U->menu.sel; S->rush = !S->rush; main_menu(); U->menu.sel = (uint8_t)sel; break; }
    case A_SOUND: { int sel = U->menu.sel; U->sound = (uint8_t)((U->sound + 1) % 3); if (U->sound_cb) U->sound_cb(U->sound); main_menu(); U->menu.sel = (uint8_t)sel; break; }
    case A_DIG: { const char *w = tw_dig(); result(w, S->dig_h ? "DIGGING..." : "EXCAVATED", 1); if (!w) { U->cur = S->dig_h ? S->dug - 1 : S->dug; ui_follow(); U->save_in = 900; } U->mode = M_PLAY; break; }
    case A_STATUS: U->mode = M_STATUS; break;
    case A_NEWS: U->mode = M_NEWS; break;
    case A_GO_G: U->cur = BASE; ui_follow(); U->mode = M_PLAY; break;
    case A_GO_TOP: U->cur = top_floor(); ui_follow(); U->mode = M_PLAY; break;
    case A_SAVE1: if (U->save_slot_cb) U->save_slot_cb(0); else if (U->save_cb) U->save_cb(); U->mode = M_PLAY; break;
    case A_SAVE2: if (U->save_slot_cb) U->save_slot_cb(1); else if (U->save_cb) U->save_cb(); U->mode = M_PLAY; break;
    case A_LOAD1: case A_LOAD2: {
        int sl = it->act == A_LOAD2 ? 1 : 0;
        bool ok = U->load_slot_cb ? U->load_slot_cb(sl) : false;
        if (ok) { U->mode = M_PLAY; U->cur = BASE; ui_follow(); tw_note(sl ? "LOADED SLOT 2" : "LOADED SLOT 1"); sfx(0); }
        else { tw_note("EMPTY SLOT"); sfx(2); U->mode = U->menu.back == M_TITLE ? M_TITLE : M_PLAY; if (U->mode == M_TITLE) title_menu(); }
        break;
    }
    case A_NEW: tw_new(S->rng ^ U->frame ^ 0x51ed27u); tw_note("NEW TOWER"); U->mode = M_PLAY; U->cur = BASE; U->cam = BASE - 2; sfx(1); break;
    case A_TITLE: U->mode = M_TITLE; title_menu(); break;
    case A_REPAIR_PWR: result(tw_repair_power(), "POWER ON", 1); U->mode = M_PLAY; U->save_in = 900; break;
    case A_REPAIR_LIFT: result(tw_repair_lift(), "LIFT FIXED", 1); U->mode = M_PLAY; U->save_in = 900; break;
    case A_SAVE_TITLE: if (U->save_cb) U->save_cb(); U->mode = M_TITLE; title_menu(); break;
    case A_PICK: start_place(it->arg, it->arg2); break;
    case A_BUY: { const char *w = tw_buy_speed(it->arg); if (!w) { S->speed = (uint8_t)it->arg; snprintf(b, sizeof b, "%s CLOCK BOUGHT", it->arg == 2 ? "2X" : "4X"); } result(w, b, 3); U->mode = M_PLAY; break; }
    case A_CLOSE: U->mode = U->menu.back; break;
    case A_CAR: result(tw_add_car(it->arg), "CAR ADDED", 1); U->mode = M_INSPECT; U->save_in = 900; break;
    case A_FINISH: result(tw_finish_now(it->arg, it->arg2), "FINISHED", 1); U->mode = M_INSPECT; U->save_in = 900; break;
    case A_RENT: { unit_t *u = &S->u[it->arg]; u->rent = (uint8_t)((u->rent + 1) % 3); unit_menu(); U->menu.sel = (uint8_t)(it - U->menu.it); break; }
    case A_DEMOLISH: { int id = it->arg; open_menu("DEMOLISH?", M_INSPECT); add("NO", NULL, A_CLOSE, 0, 0, false); add("YES, DEMOLISH", NULL, A_DEMO_YES, id, 0, false); break; }
    case A_DEMO_YES: result(tw_demolish(it->arg), "DEMOLISHED", 0); fill_insp(); U->mode = U->insp.n ? M_INSPECT : M_PLAY; U->save_in = 900; break;
    }
}
void ui_act(int k)
{
    switch (U->mode) {
    case M_TITLE:
        if (U->menu.n == 0) title_menu();
        if (k == KEY_UP && U->menu.n) { U->menu.sel = (uint8_t)((U->menu.sel + U->menu.n - 1) % U->menu.n); sfx(0); }
        else if (k == KEY_DOWN && U->menu.n) { U->menu.sel = (uint8_t)((U->menu.sel + 1) % U->menu.n); sfx(0); }
        else if (k == KEY_OK && U->menu.n) { const item_t *it = &U->menu.it[U->menu.sel]; if (it->dis) { sfx(2); tw_note("EMPTY"); } else do_item(it); }
        return;
    case M_PLAY:
        if (k == KEY_UP && U->cur < NF - 1) { U->cur++; ui_follow(); }
        else if (k == KEY_DOWN && U->cur > 0 && U->cur >= S->dug) { U->cur--; ui_follow(); }   // one floor of rock below the dug floor
        else if (k == KEY_OK) { main_menu(); sfx(0); }
        else if (k == KEY_BACK) { S->speed = S->speed ? 0 : 1; tw_note(S->speed ? "RUNNING" : "PAUSED"); }
        return;
    case M_MENU: {
        if (k == KEY_UP) U->menu.sel = (uint8_t)((U->menu.sel + U->menu.n - 1) % U->menu.n);
        else if (k == KEY_DOWN) U->menu.sel = (uint8_t)((U->menu.sel + 1) % U->menu.n);
        else if (k == KEY_OK) { const item_t *it = &U->menu.it[U->menu.sel]; if (it->dis) { sfx(2); tw_note("LOCKED"); } else do_item(it); }
        else if (k == KEY_BACK) {
            if (U->menu.back == M_TITLE) title_menu();   // stay on the title slot menu
            else U->mode = U->menu.back;
        }
        return;
    }
    case M_PLACE: {
        const tdef_t *d = &g_T[U->place.t];
        if (U->place.stage == 0) {
            if (k == KEY_UP && U->place.x > 0) U->place.x--;
            else if (k == KEY_DOWN && U->place.x < W - d->w) U->place.x++;
            else if (k == KEY_OK) {
                if (d->flags & F_SHAFT) {
                    const char *w = tw_can_place(U->place.t, U->cur, U->place.x, ui_place_top());
                    if (w) result(w, NULL, 0); else { U->place.stage = 1; U->place.top = U->place.f0 = U->cur; }
                } else {
                    const char *w = tw_place(U->place.t, U->cur, U->place.x, ui_place_top(), U->place.v);
                    result(w, NULL, 1);
                    if (!w) { U->x = U->place.x; U->place.x = tw_find_spot(U->place.t, U->cur, U->place.x + d->w); U->save_in = 900; }
                }
            } else if (k == KEY_BACK) { U->x = U->place.x; U->mode = M_PLAY; }
        } else {
            if (k == KEY_UP && U->place.top < NF - 1) { U->place.top++; U->cur = U->place.top; ui_follow(); }
            else if (k == KEY_DOWN && U->place.top > 0) { U->place.top--; U->cur = U->place.top; ui_follow(); }
            else if (k == KEY_OK) {
                int f0 = U->place.f0 < U->place.top ? U->place.f0 : U->place.top, f1 = U->place.f0 < U->place.top ? U->place.top : U->place.f0;
                const char *w = tw_place(U->place.t, f0, U->place.x, f1, -1);
                result(w, "LIFT BUILT", 1);
                if (!w) { U->mode = M_PLAY; U->save_in = 900; }
            } else if (k == KEY_BACK) U->place.stage = 0;
        }
        return;
    }
    case M_INSPECT:
        if (!U->insp.n) { U->mode = M_PLAY; return; }
        if (k == KEY_UP) U->insp.i = (U->insp.i + U->insp.n - 1) % U->insp.n;
        else if (k == KEY_DOWN) U->insp.i = (U->insp.i + 1) % U->insp.n;
        else if (k == KEY_OK) unit_menu();
        else if (k == KEY_BACK) U->mode = M_PLAY;
        return;
    case M_STATUS: case M_NEWS:
        if (k == KEY_OK || k == KEY_BACK) U->mode = M_PLAY;
        return;
    case M_END:
        if (U->end_kind == 1 && !S->deep && (k == KEY_UP || k == KEY_DOWN)) { U->end_sel ^= 1; sfx(0); }
        else if (k == KEY_OK) {
            U->mode = M_PLAY; sfx(0);
            if (U->end_kind == 1 && !S->deep && U->end_sel == 0) {   // YES: the Underground challenge
                S->deep = 1; U->cur = BASE - 4; ui_follow();
                tw_note("UNDERGROUND CHALLENGE: DIG TO B100!");
                if (U->save_cb) U->save_cb();
            } else tw_note(U->end_kind == 2 ? "CORE RATING - THE DEEPEST TOWER ON EARTH" : "KEEP BUILDING - THE TOWER IS YOURS");
        }
        return;
    }
}
void ui_keys(bool up, bool down, bool ok)
{
    // UP/DOWN: act on press, then repeat after 12 frames (every 4th, then every frame after 40)
    uint8_t h = up ? KEY_UP : down ? KEY_DOWN : ok ? KEY_OK : 0;
    if (h != U->held) {
        if (U->held == KEY_OK && !U->ok_long) ui_act(KEY_OK);   // OK acts on release (a tap)
        U->held = h; U->hold_n = 0; U->ok_long = false;
        if (h == KEY_UP || h == KEY_DOWN) ui_act(h);
        return;
    }
    if (!h) return;
    U->hold_n++;
    if (h == KEY_OK) { if (U->hold_n == 15) { U->ok_long = true; ui_act(KEY_BACK); } return; }
    if (U->hold_n >= 12 && (U->hold_n >= 40 || U->hold_n % 4 == 0)) ui_act(h);
}
void ui_init(void)
{
    uint8_t snd = U->sound;
    memset(U, 0, sizeof *U - sizeof(void *) * 4);
    U->sound = snd;
    U->mode = M_TITLE; U->cur = BASE; U->cam = BASE - 2; U->x = 28; title_menu();
}
int ui_music(void)
{
    if (U->sound != 0) return 0;
    if (U->mode == M_TITLE) return 2;
    return tw_night() ? 2 : 1;
}
int ui_frame(void)
{
    U->frame++;
    if (U->mode != M_PLAY && U->mode != M_PLACE && U->mode != M_INSPECT) return 0;
    if (!S->speed) return 0;
    int n = S->tick < 6 * T_HOUR ? NIGHT_TPF : TPF[S->speed];
    uint32_t d0 = S->day;
    for (int i = 0; i < n; i++) tw_tick();
    if (S->celebrate) { U->end_kind = S->celebrate; U->end_sel = 0; S->celebrate = 0; U->mode = M_END; sfx(1); if (U->save_cb) U->save_cb(); }   // ending: fireworks
    if (S->day != d0 && S->day % 3 == 0 && U->save_cb) U->save_cb();   // autosave each quarter
    if (U->save_in && --U->save_in == 0 && U->save_cb) U->save_cb();   // and 30 s after building
    return n;
}
