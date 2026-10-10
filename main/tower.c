// 摩天大廈 SKYRISE simulation: a one-for-one port of prototype/p1..p3.js.
#include "tower.h"

#include <stdio.h>
#include <string.h>

tower_t g_tw;
#define S (&g_tw)

#define RGB(r, g, b) (uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define TEN F_TENANT
#define COM (F_COMMERCE | F_TENANT)
//  zh, en, w, h, star, flags, where, open0, open1, guests, cost, car, maint, night, per_cust, base, build_min, col
const tdef_t g_T[T_COUNT] = {
    [T_LOBBY]   = {"大堂", "LOBBY", 8, 1, 1, 0, WH_LOBBY, 0, 0, 0, 5000, 0, 0, 0, 0, 0, 0, RGB(200, 180, 138)},
    [T_FLOOR]   = {"空層", "EMPTY FLOOR", 1, 1, 1, F_FILL, WH_ANY, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, RGB(90, 90, 102)},
    [T_STAIRS]  = {"樓梯", "STAIRS", 8, 1, 1, 0, WH_ANY, 0, 0, 0, 5000, 0, 0, 0, 0, 0, 30, RGB(140, 122, 90)},
    [T_LIFT]    = {"升降機", "LIFT", 4, 1, 1, F_SHAFT, WH_ANY, 0, 0, 0, 200000, 80000, 10000, 0, 0, 0, 360, RGB(58, 63, 74)},
    [T_OFFICE]  = {"寫字樓", "OFFICE", 9, 1, 1, TEN, WH_ANY, 0, 0, 0, 40000, 0, 0, 0, 0, 0, 180, RGB(127, 168, 216)},
    [T_FLAT]    = {"住宅", "FLAT", 16, 1, 1, TEN | F_QUIET, WH_ANY, 0, 0, 0, 80000, 0, 0, 0, 0, 0, 240, RGB(232, 200, 144)},
    [T_CCT]     = {"茶餐廳", "CHA CHAAN TENG", 16, 1, 1, COM | F_FOOD, WH_ANY, 7, 21, 0, 100000, 0, 0, 0, 100, 35, 240, RGB(232, 88, 74)},
    [T_SINGLE]  = {"單人房", "HOTEL SINGLE", 4, 1, 2, TEN | F_HOTEL | F_QUIET, WH_ANY, 0, 0, 1, 20000, 0, 0, 1500, 0, 0, 120, RGB(184, 224, 192)},
    [T_HOUSEK]  = {"房務部", "HOUSEKEEPING", 15, 1, 2, 0, WH_ANY, 0, 0, 0, 50000, 0, 10000, 0, 0, 0, 180, RGB(160, 160, 184)},
    [T_SECURITY]= {"保安室", "SECURITY", 16, 1, 2, 0, WH_ANY, 0, 0, 0, 100000, 0, 20000, 0, 0, 0, 180, RGB(74, 106, 168)},
    [T_TWIN]    = {"雙人房", "HOTEL TWIN", 6, 1, 3, TEN | F_HOTEL | F_QUIET, WH_ANY, 0, 0, 2, 50000, 0, 0, 3000, 0, 0, 180, RGB(144, 208, 168)},
    [T_SUITE]   = {"套房", "HOTEL SUITE", 10, 1, 3, TEN | F_HOTEL | F_QUIET, WH_ANY, 0, 0, 2, 100000, 0, 0, 6000, 0, 0, 240, RGB(96, 184, 136)},
    [T_SHOP]    = {"SHOP", "SHOP", 12, 1, 3, COM, WH_ANY, 10, 21, 0, 100000, 0, 0, 0, 150, 25, 240, RGB(232, 144, 200)},
    [T_DIMSUM]  = {"酒樓", "DIM SUM HALL", 24, 1, 3, COM | F_FOOD, WH_ANY, 7, 15, 0, 200000, 0, 0, 0, 200, 35, 360, RGB(216, 64, 64)},
    [T_CINEMA]  = {"戲院", "CINEMA", 31, 2, 3, COM, WH_ANY, 0, 0, 0, 500000, 0, 0, 0, 120, 60, 720, RGB(80, 48, 112)},
    [T_CLINIC]  = {"診所", "CLINIC", 26, 1, 3, 0, WH_ANY, 0, 0, 0, 500000, 0, 0, 0, 0, 0, 720, RGB(240, 240, 240)},
    [T_RECYCLE] = {"回收站", "RECYCLING", 25, 2, 3, 0, WH_ANY, 0, 0, 0, 500000, 0, 50000, 0, 0, 0, 720, RGB(90, 138, 74)},
    [T_PARKING] = {"停車場", "CAR PARK", 16, 1, 3, 0, WH_BASE, 0, 0, 0, 50000, 0, 10000, 0, 0, 0, 120, RGB(112, 112, 112)},
    [T_EXP]     = {"高速升降機", "EXPRESS LIFT", 6, 1, 3, F_SHAFT, WH_ANY, 0, 0, 0, 400000, 150000, 20000, 0, 0, 0, 720, RGB(42, 47, 58)},
    [T_METRO]   = {"地鐵站", "METRO STATION", 30, 3, 4, 0, WH_METRO, 0, 0, 0, 1000000, 0, 100000, 0, 0, 0, 1440, RGB(192, 48, 48)},
    [T_CHAPEL]  = {"天空禮堂", "SKY CHAPEL", 28, 4, 5, 0, WH_TOP, 0, 0, 0, 3000000, 0, 0, 0, 0, 0, 2880, RGB(248, 240, 216)},
    [T_CARPARK] = {"停車場", "PARKING", 20, 2, 2, 0, WH_PARK, 0, 0, 0, 300000, 0, 20000, 0, 0, 0, 360, RGB(100, 104, 112)},
    [T_POOL]    = {"泳池", "POOL", 24, 2, 3, COM, WH_ANY, 7, 22, 0, 600000, 0, 20000, 0, 60, 70, 720, RGB(60, 140, 200)},
    [T_ICE]     = {"溜冰場", "ICE RINK", 28, 2, 3, COM, WH_ANY, 10, 22, 0, 800000, 0, 30000, 0, 90, 70, 720, RGB(200, 230, 245)},
    [T_ATRIUM]  = {"中庭", "ATRIUM", 24, 4, 3, 0, WH_ANY, 0, 0, 0, 1000000, 0, 30000, 0, 0, 0, 1080, RGB(230, 220, 200)},
    [T_BANQUET] = {"宴會廳", "BANQUET", 32, 2, 4, COM, WH_ANY, 11, 23, 0, 1200000, 0, 20000, 0, 300, 30, 1080, RGB(170, 30, 40)},
    [T_CLIMB]   = {"攀石", "CLIMB", 12, 3, 3, COM, WH_ANY, 10, 22, 0, 400000, 0, 10000, 0, 80, 40, 480, RGB(150, 120, 90)},
    [T_GARDEN]  = {"空中花園", "SKY GARDEN", 24, 2, 4, 0, WH_HIGH, 0, 0, 0, 800000, 0, 20000, 0, 0, 0, 720, RGB(70, 140, 70)},
    [T_STUDIO]  = {"電視台", "TV STUDIO", 28, 3, 4, 0, WH_ANY, 0, 0, 0, 1500000, 0, 0, 0, 0, 0, 1440, RGB(30, 30, 40)},
    [T_PUMP]    = {"泵房", "PUMP ROOM", 6, 1, 1, 0, WH_ANY, 0, 0, 0, 150000, 0, 15000, 0, 0, 0, 240, RGB(60, 90, 130), 1},
    [T_VENT]    = {"通風", "VENT SHAFT", 4, 1, 1, 0, WH_ANY, 0, 0, 0, 120000, 0, 10000, 0, 0, 0, 240, RGB(110, 120, 120), 1},
    [T_MHUB]    = {"地鐵", "METRO HUB", 40, 3, 1, COM, WH_ANY, 6, 24, 0, 2000000, 0, 50000, 0, 40, 400, 1440, RGB(200, 60, 50), 10},
    [T_SPA]     = {"溫泉", "HOT SPRING", 28, 2, 1, COM, WH_ANY, 10, 24, 0, 1500000, 0, 30000, 0, 220, 80, 1080, RGB(200, 120, 90), 20},
    [T_FOSSIL]  = {"化石館", "FOSSIL MUSEUM", 32, 3, 1, COM, WH_ANY, 10, 19, 0, 1800000, 0, 30000, 0, 150, 90, 1440, RGB(170, 150, 110), 30},
    [T_GEM]     = {"寶石", "GEM MINE", 20, 2, 1, COM, WH_ANY, 10, 21, 0, 1500000, 0, 20000, 0, 400, 40, 1080, RGB(90, 60, 120), 40},
    [T_NMARKET] = {"夜市", "NIGHT MARKET", 40, 1, 1, COM, WH_ANY, 17, 24, 0, 1200000, 0, 20000, 0, 120, 140, 720, RGB(160, 40, 50), 50},
    [T_AQUA]    = {"水族館", "AQUARIUM", 36, 3, 1, COM, WH_ANY, 10, 21, 0, 2500000, 0, 50000, 0, 200, 110, 1440, RGB(20, 70, 140), 60},
    [T_LAB]     = {"實驗室", "SECRET LAB", 24, 2, 1, 0, WH_ANY, 0, 0, 0, 3000000, 0, 0, 0, 0, 0, 1440, RGB(40, 60, 60), 70},
    [T_GEO]     = {"地熱", "GEOTHERMAL", 32, 2, 1, 0, WH_ANY, 0, 0, 0, 4000000, 0, 0, 0, 0, 0, 1440, RGB(120, 40, 20), 85},
    [T_VAULT]   = {"地心寶庫", "CORE VAULT", 40, 2, 1, 0, WH_ANY, 0, 0, 0, 10000000, 0, 0, 0, 0, 0, 2880, RGB(220, 180, 60), 99},
    [T_REFUGE]  = {"防火層", "REFUGE", 64, 1, 1, 0, WH_ANY, 0, 0, 0, 300000, 0, 20000, 0, 0, 0, 360, RGB(96, 104, 98)},
    [T_MEGA]    = {"戲院", "MEGA CINEMA", 40, 3, 4, COM, WH_ANY, 0, 0, 0, 1500000, 0, 0, 0, 160, 150, 1440, RGB(40, 24, 64)},
    [T_COASTER] = {"過山車", "COASTER", 48, 5, 4, COM, WH_ANY, 10, 23, 0, 3000000, 0, 50000, 0, 250, 140, 2880, RGB(24, 28, 52)},
};
const uint8_t g_build_order[T_COUNT] = {T_LOBBY, T_FLOOR, T_STAIRS, T_LIFT, T_OFFICE, T_FLAT, T_CCT, T_SINGLE, T_HOUSEK,
    T_SECURITY, T_REFUGE, T_TWIN, T_SUITE, T_SHOP, T_DIMSUM, T_CINEMA, T_CLINIC, T_RECYCLE, T_PARKING, T_CARPARK, T_POOL, T_ICE, T_CLIMB, T_ATRIUM, T_EXP, T_MEGA, T_COASTER, T_BANQUET, T_GARDEN, T_STUDIO, T_METRO, T_CHAPEL,
    T_PUMP, T_VENT, T_MHUB, T_SPA, T_FOSSIL, T_GEM, T_NMARKET, T_AQUA, T_LAB, T_GEO, T_VAULT};
static const uint8_t SKY[N_SKY] = {BASE - 90, BASE - 75, BASE - 60, BASE - 45, BASE - 30, BASE - 15, BASE,
                                   BASE + 15, BASE + 30, BASE + 45, BASE + 60, BASE + 75, BASE + 90};   // ascending
static const int STAR_POP[7] = {0, 0, 200, 600, 2000, 3500, 5000};
static const int SPEED_PRICE[4] = {0, 0, 300000, 1000000};
static const int SPEED_STAR[4] = {0, 0, 1, 3};
int g_star_pop_override = -1;   // host tests: -1 = normal
int g_build_on = 1;             // host tests can switch construction time off
int g_events_on = 1;            // host tests: 0 = no random day events (VIP/fire still run)

enum { K_IN, K_OUT, K_BACK, K_LUNCH, K_VISIT, K_RESSHOP, K_GUEST, K_VIP, K_VIPOUT, K_WED, K_LEAVE };
enum { ST_FREE, ST_Q, ST_RIDE, ST_WALK };

#define FL(t) (g_T[t].flags)
#define UH(u) tw_uh(u)
#define IS_TENANT(t) (FL(t) & F_TENANT)
#define IS_COM(t) (FL(t) & F_COMMERCE)
#define IS_FOOD(t) (FL(t) & F_FOOD)
#define IS_HOTEL(t) (FL(t) & F_HOTEL)
#define LIVE(u) ((u)->build == 0)
#define MINUTE() ((int)(S->tick / T_MIN))

/* ------------------------------------------------------------ rng, helpers */
uint32_t tw_rand(void)
{
    uint32_t x = S->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return S->rng = x ? x : 1;
}
static int rn(int n) { return n > 0 ? (int)(tw_rand() % (uint32_t)n) : 0; }
static bool chance(int num, int den) { return den > 0 && rn(den) < num; }   // num/den

bool tw_is_lobby_floor(int f)
{
    for (int i = 0; i < N_SKY; i++) if (SKY[i] == f) return true;
    return false;
}
const char *tw_floor_name(int f, char b[8])
{
    if (f < BASE) snprintf(b, 8, "B%d", (BASE - f) % 1000);
    else if (f == BASE) snprintf(b, 8, "G");
    else snprintf(b, 8, "%d", (f - BASE) & 127);
    return b;
}
int tw_hour_x10(void) { return (int)(S->tick * 10 / T_HOUR); }
bool tw_night(void) { int h = tw_hour_x10(); return h < 60 || h >= 190; }
static int day_kind(void) { return (int)(S->day % 3); }
static bool weekend(void) { return day_kind() == 2; }
static int quarter(void) { return (int)(S->day / 3 % 4); }

void tw_note(const char *s)
{
    snprintf(S->msg, sizeof S->msg, "%s", s);
    S->msg_t = 150;
    memmove(S->log[1], S->log[0], sizeof S->log[0] * 11);
    char b[48];
    snprintf(b, sizeof b, "%s %s", (const char *[]){"WD1", "WD2", "WE"}[day_kind()], s);
    memcpy(S->log[0], b, sizeof b);
    if (S->n_log < 12) S->n_log++;
}
static void notes(const char *fmt, const char *a) { char b[64]; snprintf(b, sizeof b, fmt, a); tw_note(b); }
static void notei(const char *fmt, int a) { char b[64]; snprintf(b, sizeof b, fmt, a); tw_note(b); }
static void spend(int64_t n) { S->funds -= n; S->st.spend += n; S->st.q_out += n; }
static void earn(int64_t n) { S->funds += n; S->st.income += n; S->st.q_in += n; }

/* ------------------------------------------------------------ rooms and cells */
static void list_add(int id)
{
    unit_t *u = &S->u[id];
    u->next = S->floor_head[u->f];
    S->floor_head[u->f] = (uint16_t)id;
}
static void list_del(int id)
{
    uint16_t *pp = &S->floor_head[S->u[id].f];
    while (*pp != NO_ID) {
        if (*pp == id) { *pp = S->u[id].next; return; }
        pp = &S->u[*pp].next;
    }
}
static int alloc_unit(void)
{
    for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_NONE) return i;
    if (S->n_units < MAX_UNITS) return S->n_units++;
    return -1;
}
int tw_cell(int f, int x)
{
    if (f < 0 || f >= NF || x < 0 || x >= W) return 0;
    for (int ff = f; ff >= 0 && ff > f - MAX_H; ff--) {
        for (uint16_t i = S->floor_head[ff]; i != NO_ID; i = S->u[i].next) {
            const unit_t *u = &S->u[i];
            if (ff + UH(u) - 1 >= f && x >= u->x && x < u->x + u->w) return i + 1;
        }
    }
    for (int i = 0; i < S->n_shafts; i++) {
        const shaft_t *s = &S->s[i];
        if (f >= s->f0 && f <= s->f1 && x >= s->x && x < s->x + s->w) return -(i + 1);
    }
    return 0;
}
static bool slab(int f, int x) { return (S->slab[f] >> x) & 1; }
static void set_slab(int f, int x) { S->slab[f] |= 1ull << x; }
static bool supported(int f, int x, int w)
{
    if (f == BASE) return true;
    int g = f > BASE ? f - 1 : f + 1;
    for (int xx = x; xx < x + w; xx++) if (!slab(g, xx)) return false;
    return true;
}
static bool has_type(int t, bool live_only)
{
    for (int i = 0; i < S->n_units; i++) if (S->u[i].t == t && (!live_only || LIVE(&S->u[i]))) return true;
    return false;
}
static int count_type(int t)
{
    int n = 0;
    for (int i = 0; i < S->n_units; i++) if (S->u[i].t == t && LIVE(&S->u[i])) n++;
    return n;
}
static bool rush_on(int t) { return S->rush && g_build_on && g_T[t].build_min > 0; }
int32_t tw_cost(int t, int f, int f1)
{
    const tdef_t *d = &g_T[t];
    int32_t base = (d->flags & F_SHAFT) ? d->cost + d->car + 2000 * (f1 - f) : d->cost;
    return rush_on(t) ? base * (100 + RUSH_PCT) / 100 : base;
}

int tw_uh(const unit_t *u) { return u->t == T_CARPARK ? (u->v >= 2 && u->v <= 4 ? u->v : 2) : g_T[u->t].h; }
static bool has_refuge(int f, bool live)
{
    if (f < 0 || f >= NF) return false;
    for (uint16_t i = S->floor_head[f]; i != NO_ID; i = S->u[i].next) if (S->u[i].t == T_REFUGE && (!live || LIVE(&S->u[i]))) return true;
    return false;
}
bool tw_is_refuge_floor(int f) { return has_refuge(f, true); }
int tw_depth(int f) { return f < BASE ? BASE - f : 0; }
int32_t tw_repair_power_cost(void) { return 80000; }
int32_t tw_repair_lift_cost(void) { return 50000; }
const char *tw_repair_power(void)
{
    if (!S->power_h) return "POWER IS ON";
    int32_t c = tw_repair_power_cost();
    if (c > S->funds) return "NOT ENOUGH $";
    spend(c); S->power_h = 0; tw_note("POWER RESTORED! LIFTS RUNNING"); return NULL;
}
const char *tw_repair_lift(void)
{
    if (S->broken_si == 0xff) return "NO BROKEN LIFT";
    int32_t c = tw_repair_lift_cost();
    if (c > S->funds) return "NOT ENOUGH $";
    spend(c); S->broken_si = 0xff; S->broken_h = 0; tw_note("THE LIFT IS FIXED!"); return NULL;
}
bool tw_shaft_live(int si) { return !S->power_h && (S->broken_si == 0xff || S->broken_si != (uint8_t)si); }

static int dig_mult(int d) { return d <= 30 ? 1 : d <= 60 ? 2 : d <= 85 ? 3 : 4; }   // soil, rock, granite, magma
int32_t tw_dig_cost(void) { int d = tw_depth(S->dug - 1); return (100000 + 15000 * d) * dig_mult(d); }
int tw_dig_hours(void) { int d = tw_depth(S->dug - 1); return (2 + d / 10) * (dig_mult(d) + 1) / 2; }
const char *tw_dig(void)
{
    if (!S->deep) return "UNDERGROUND LOCKED";
    if (S->dug == 0) return "B100 REACHED";
    if (S->dig_h) {   // rush the dig in progress: +25 pct of its price
        int32_t c = tw_dig_cost() * RUSH_PCT / 100;
        if (c > S->funds) return "NOT ENOUGH $";
        spend(c); S->dig_h = 1; return NULL;   // finishes on the next hour tick
    }
    int32_t c = tw_dig_cost();
    if (c > S->funds) return "NOT ENOUGH $";
    spend(c);
    if (!g_build_on) { S->dug--; return NULL; }
    S->dig_h = (uint16_t)tw_dig_hours();
    return NULL;
}
static bool deep_near(int t, int f, int range)
{
    for (int i = 0; i < S->n_units; i++) { const unit_t *o = &S->u[i]; if (o->t == t && LIVE(o) && o->f + tw_uh(o) - 1 >= f - range && o->f <= f + range) return true; }
    return false;
}
int tw_deep_uncovered(int kind)
{
    int lim = kind ? 20 : 10, t = kind ? T_VENT : T_PUMP, r = kind ? 15 : 10;
    for (int f = BASE - lim - 1; f >= 0; f--) {
        bool used = false;
        for (int ff = f; ff >= 0 && ff > f - MAX_H && !used; ff--)
            for (uint16_t i = S->floor_head[ff]; i != NO_ID && !used; i = S->u[i].next) used = S->u[i].t != T_FLOOR && ff + tw_uh(&S->u[i]) - 1 >= f;
        if (used && !deep_near(t, f, r)) return f;
    }
    return -1;
}
static bool vent_ok(int f) { return tw_depth(f) <= 20 || deep_near(T_VENT, f, 15); }
static bool refuge_covered(int f)
{
    if (f <= BASE + REFUGE_GAP) return true;
    for (int r = f - 1; r >= f - REFUGE_GAP; r--) if (has_refuge(r, true)) return true;
    return false;
}
int tw_uncovered_floor(void)
{
    for (int f = BASE + REFUGE_GAP + 1; f < NF; f++) {
        bool used = false;
        for (uint16_t i = S->floor_head[f]; i != NO_ID && !used; i = S->u[i].next) used = S->u[i].t != T_FLOOR && S->u[i].t != T_REFUGE;
        if (used && !refuge_covered(f)) return f;
    }
    return -1;
}
static const char *can_refuge(int f)
{
    if (f <= BASE) return "ABOVE G/F ONLY";
    if (tw_is_lobby_floor(f)) return "SKY LOBBY FLOOR";
    if (has_refuge(f, false)) return "ALREADY REFUGE";
    int n = 0;   // unsupported holes are skipped; it needs some floor to stand on
    for (int x = 0; x < W; x++) {
        int c = tw_cell(f, x);
        if (c < 0 || (c > 0 && S->u[c - 1].t == T_STAIRS)) continue;
        if (c > 0 && S->u[c - 1].t == T_FLOOR) { n++; continue; }
        if (c > 0) return "FLOOR NOT EMPTY";
        n += supported(f, x, 1);
    }
    if (!n) return "NO FLOOR BELOW";
    if (tw_cost(T_REFUGE, f, f) > S->funds) return "NOT ENOUGH $";
    return NULL;
}

const char *tw_can_place(int t, int f, int x, int top)
{
    const tdef_t *d = &g_T[t];
    static char why[24];
    if (d->star > S->star) { snprintf(why, sizeof why, "NEEDS %d STARS", d->star); return why; }
    if (t == T_REFUGE) return can_refuge(f);
    if (d->dmin) {
        if (!S->deep) return "UNDERGROUND CHALLENGE";
        if (f >= BASE) return "UNDERGROUND ONLY";
        if (t == T_VAULT && f != 0) return "B100 ONLY";
        if (tw_depth(f) < d->dmin) { snprintf(why, sizeof why, "B%d AND DEEPER", d->dmin); return why; }
    }
    if (f < S->dug) return S->deep ? "DIG FIRST" : "SOLID ROCK";
    if (x < 0 || x + d->w > W) return "OUTSIDE";
    if (d->flags & F_SHAFT) {
        int f1 = top < 0 ? f : top;
        if (f1 < f || f1 >= NF) return "BAD SPAN";
        if (t == T_LIFT && f1 - f + 1 > LIFT_SPAN) return "MAX 30 FLOORS";
        if (S->n_shafts >= MAX_SHAFTS) return "MAX 24 SHAFTS";
        if (t == T_EXP) {   // an express stops at its ends and the lobby floors between: must fit the queue table
            int n = 2;
            for (int i = 0; i < N_SKY; i++) if (SKY[i] > f && SKY[i] < f1) n++;
            if (n > MAX_QF) return "TOO MANY STOPS";
        }
        for (int ff = f; ff <= f1; ff++) {
            for (int xx = x; xx < x + d->w; xx++) {
                int c = tw_cell(ff, xx);
                if (c < 0 || (c > 0 && S->u[c - 1].t != T_LOBBY && S->u[c - 1].t != T_FLOOR && S->u[c - 1].t != T_REFUGE)) return "BLOCKED";   // lifts may pass through refuge floors
            }
            if (ff == f && !supported(ff, x, d->w)) return "NO FLOOR BELOW";
        }
        if (tw_cost(t, f, f1) > S->funds) return "NOT ENOUGH $";
        return NULL;
    }
    int hh = t == T_CARPARK ? (top < 0 ? 2 : top - f + 1) : d->h;
    if (t == T_CARPARK) {
        if (hh < 2 || hh > 4) return "2-4 FLOORS";
        if (f < BASE - 4 || f + hh - 1 > BASE) return "B4-G ONLY";
        for (int ff = f; ff < f + hh; ff++) for (int xx = x; xx < x + d->w; xx++) {
            int c = tw_cell(ff, xx);
            if (c < 0 || (c > 0 && S->u[c - 1].t != T_FLOOR && S->u[c - 1].t != T_LOBBY)) return "BLOCKED";
        }
        if (f + hh - 1 < BASE && !supported(f + hh - 1, x, d->w)) return "DIG FROM ABOVE";
        if (tw_cost(t, f, f) * hh / 2 > S->funds) return "NOT ENOUGH $";
        if (S->n_units >= MAX_UNITS && alloc_unit() < 0) return "MAX ROOMS";
        return NULL;
    }
    if (d->where == WH_HIGH && f < BASE + 60) return "60/F AND ABOVE";
    if (f + d->h - 1 >= NF) return "TOO HIGH";
    if (d->where == WH_LOBBY && !tw_is_lobby_floor(f)) return "LOBBY: G + EVERY 15 FL";
    if (d->where != WH_LOBBY && t != T_STAIRS && t != T_FLOOR && tw_is_lobby_floor(f) && f != BASE) return "SKY LOBBY FLOOR";
    if (d->where == WH_BASE && f >= BASE) return "BASEMENT ONLY";
    if (d->where == WH_METRO && f != BASE - 4) return "B4-B2 ONLY";
    if (d->where == WH_TOP && f + d->h - 1 != NF - 1) return "TOP 4 FLOORS ONLY";
    if ((t == T_CHAPEL || t == T_METRO) && has_type(t, false)) return "ONLY ONE";
    if (f == BASE && t != T_LOBBY && t != T_STAIRS) return "G/F IS THE LOBBY";
    for (int ff = f; ff < f + d->h; ff++) {
        if (has_refuge(ff, false) && t != T_STAIRS) return "REFUGE FLOOR";
        for (int xx = x; xx < x + d->w; xx++) {
            int c = tw_cell(ff, xx);
            if (c == 0) continue;
            if (c > 0 && (S->u[c - 1].t == T_FLOOR || (t == T_STAIRS && S->u[c - 1].t == T_LOBBY))) continue;
            return "BLOCKED";
        }
    }
    if (!supported(f < BASE ? f + d->h - 1 : f, x, d->w)) return f < BASE ? "DIG FROM ABOVE" : "NO FLOOR BELOW";   // basements hang from the floor above their top
    if (t == T_STAIRS && f + 1 >= NF) return "TOP FLOOR";
    if (tw_cost(t, f, f) > S->funds) return "NOT ENOUGH $";
    if (S->n_units >= MAX_UNITS && alloc_unit() < 0) return "MAX ROOMS";
    return NULL;
}

static int add_unit(int t, int f, int x, int w)
{
    int id = alloc_unit();
    if (id < 0) return -1;
    unit_t *u = &S->u[id];
    memset(u, 0, sizeof *u);
    u->t = (uint8_t)t; u->f = (uint8_t)f; u->x = (uint8_t)x; u->w = (uint8_t)w;
    u->eval = 70; u->rent = 1; u->v = (uint8_t)(id % 6);
    u->occ = IS_TENANT(t) ? 0 : 1;
    list_add(id);
    for (int ff = f; ff < f + g_T[t].h; ff++) for (int xx = x; xx < x + w; xx++) set_slab(ff, xx);
    return id;
}
static void remove_unit(int id)
{
    list_del(id);
    S->u[id].t = T_NONE;
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (S->g[i].st == ST_FREE) continue;
        if (S->g[i].u == id) S->g[i].u = NO_ID;
        if (S->g[i].dest == id) S->g[i].dest = NO_ID;
    }
    for (int i = 0; i < S->n_pend; i++) if (S->p[i].u == id) S->p[i].u = NO_ID;
    if (S->fire_u == id) S->fire_u = NO_ID;
}
/* cut [x, x+w) out of empty-floor / lobby pieces on floor f; the rest stays */
static void carve(int f, int x, int w)
{
    for (int xx = x; xx < x + w; xx++) {
        int c = tw_cell(f, xx);
        if (c <= 0) continue;
        unit_t o = S->u[c - 1];
        if (o.t != T_FLOOR && o.t != T_LOBBY && o.t != T_REFUGE) continue;
        remove_unit(c - 1);
        int L = x - o.x, R = o.x + o.w - (x + w);
        if (L > 0) add_unit(o.t, f, o.x, L);
        if (R > 0) add_unit(o.t, f, x + w, R);
    }
}
static void car_init(car_t *c, int f)
{
    memset(c, 0, sizeof *c);
    c->f = (uint8_t)f; c->head = NO_ID; c->disp = (int32_t)f << 8; c->stop_f = -1;
}
static int qi(const shaft_t *s, int f);
const char *tw_place(int t, int f, int x, int top, int v)
{
    const char *why = tw_can_place(t, f, x, top);
    if (why) return why;
    const tdef_t *d = &g_T[t];
    bool rushed = rush_on(t);
    if (d->flags & F_SHAFT) {
        int f1 = top < 0 ? f : top;
        for (int ff = f; ff <= f1; ff++) carve(ff, x, d->w);
        shaft_t *s = &S->s[S->n_shafts++];
        memset(s, 0, sizeof *s);
        s->t = (uint8_t)t; s->x = (uint8_t)x; s->w = d->w; s->f0 = (uint8_t)f; s->f1 = (uint8_t)f1;
        for (int i = 0; i < MAX_QF; i++) s->qh[i] = s->qt[i] = NO_ID;
        s->ncars = 1; car_init(&s->car[0], f);
        s->build = rushed || !g_build_on ? 0 : d->build_min;
        for (int ff = f; ff <= f1; ff++) for (int xx = x; xx < x + d->w; xx++) set_slab(ff, xx);
        spend(tw_cost(t, f, f1));
        if (rushed) S->st.rushed++;
        tw_compute_access();
        return NULL;
    }
    if (t == T_CARPARK) {
        int hh = top < 0 ? 2 : top - f + 1;
        for (int ff = f; ff < f + hh; ff++) carve(ff, x, d->w);
        int id = add_unit(t, f, x, d->w);
        if (id < 0) return "MAX ROOMS";
        S->u[id].v = (uint8_t)hh;
        for (int ff = f; ff < f + hh; ff++) for (int xx = x; xx < x + d->w; xx++) set_slab(ff, xx);
        S->u[id].build = rushed || !g_build_on ? 0 : d->build_min;
        if (rushed) S->st.rushed++;
        spend(tw_cost(t, f, f) * hh / 2);
        tw_compute_access();
        return NULL;
    }
    if (t == T_REFUGE) {   // whole floor: one piece per gap between shafts / stairs
        for (int xx = 0; xx < W; xx++) { int c = tw_cell(f, xx); if (c > 0 && S->u[c - 1].t == T_FLOOR) remove_unit(c - 1); }
        for (int xx = 0; xx < W;) {
            if (tw_cell(f, xx) || !supported(f, xx, 1)) { xx++; continue; }
            int x0 = xx;
            while (xx < W && !tw_cell(f, xx) && supported(f, xx, 1)) xx++;
            int id = add_unit(T_REFUGE, f, x0, xx - x0);
            if (id >= 0) S->u[id].build = rushed || !g_build_on ? 0 : d->build_min;
        }
        if (rushed) S->st.rushed++;
        spend(tw_cost(t, f, f));
        tw_compute_access();
        return NULL;
    }
    int w = d->w;
    if (d->flags & F_FILL) {
        while (w < 16 && x + w < W && tw_cell(f, x + w) == 0 && supported(f, x + w, 1) && S->funds >= (int64_t)d->cost * (w + 1)) w++;
    }
    for (int ff = f; ff < f + d->h; ff++) carve(ff, x, w);
    int id = add_unit(t, f, x, w);
    if (id < 0) return "MAX ROOMS";
    unit_t *u = &S->u[id];
    u->build = rushed || !g_build_on ? 0 : d->build_min;
    if (v >= 0) u->v = (uint8_t)v;
    if (rushed) S->st.rushed++;
    spend((d->flags & F_FILL) ? (int64_t)d->cost * w : tw_cost(t, f, f));
    if (t == T_STAIRS || t == T_LOBBY || t == T_FLOOR) tw_compute_access();
    return NULL;
}
const char *tw_demolish(int id)
{
    unit_t *u = &S->u[id];
    if (u->t == T_HOUSEK || u->t == T_SECURITY) return "CANNOT REMOVE";
    int c = g_T[u->t].cost / 10;
    if (u->t == T_REFUGE) {   // the whole refuge floor goes, back to empty floor
        int f = u->f;
        for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_REFUGE && S->u[i].f == f) { int x0 = S->u[i].x, w = S->u[i].w; remove_unit(i); add_unit(T_FLOOR, f, x0, w); }
        spend(c); tw_compute_access();
        return NULL;
    }
    remove_unit(id);
    spend(c);
    tw_compute_access();
    return NULL;
}
const char *tw_add_car(int si)
{
    shaft_t *s = &S->s[si];
    if (s->ncars >= MAX_CARS) return "MAX 4 CARS";
    if (g_T[s->t].car > S->funds) return "NOT ENOUGH $";
    car_init(&s->car[s->ncars++], s->f0);
    spend(g_T[s->t].car);
    return NULL;
}
int32_t tw_finish_price(int uid, int si)
{
    int t = uid >= 0 ? S->u[uid].t : S->s[si].t;
    int left = uid >= 0 ? S->u[uid].build : S->s[si].build, tot = g_T[t].build_min ? g_T[t].build_min : 1;
    int32_t p = (int32_t)((int64_t)g_T[t].cost * RUSH_PCT / 100 * left / tot);
    return p < 1000 ? 1000 : p;
}
const char *tw_finish_now(int uid, int si)
{
    int32_t p = tw_finish_price(uid, si);
    if (p > S->funds) return "NOT ENOUGH $";
    spend(p);
    if (uid >= 0) S->u[uid].build = 0; else S->s[si].build = 0;
    S->st.rushed++;
    tw_compute_access();
    return NULL;
}
const char *tw_buy_speed(int i)
{
    static char why[24];
    if (S->owned[i]) return "ALREADY OWNED";
    if (S->star < SPEED_STAR[i]) { snprintf(why, sizeof why, "NEEDS %d STARS", SPEED_STAR[i]); return why; }
    if (SPEED_PRICE[i] > S->funds) return "NOT ENOUGH $";
    spend(SPEED_PRICE[i]);
    S->owned[i] = 1; S->st.speed_buys++;
    return NULL;
}
int tw_find_spot(int t, int f, int from)
{
    for (int k = 0; k < W; k++) {
        int c[2] = {from + k, from - k};
        for (int j = 0; j < 2; j++) if (c[j] >= 0 && c[j] + g_T[t].w <= W && !tw_can_place(t, f, c[j], -1)) return c[j];
    }
    int x = from < 0 ? 0 : from;
    return x + g_T[t].w > W ? W - g_T[t].w : x;
}

/* ------------------------------------------------------------ routes */
bool tw_serves(const shaft_t *s, int f)
{
    return !s->build && f >= s->f0 && f <= s->f1 && (s->t == T_LIFT || tw_is_lobby_floor(f) || f == s->f0 || f == s->f1);
}
static int qi(const shaft_t *s, int f)
{
    if (s->t == T_LIFT) return f - s->f0;
    if (f == s->f0) return 0;
    int n = 1;
    for (int i = 0; i < N_SKY; i++) {
        if (SKY[i] <= s->f0 || SKY[i] >= s->f1) continue;
        if (SKY[i] == f) return n;
        n++;
    }
    return n;   // f1
}
static bool stairs_at(int f)
{
    for (uint16_t i = S->floor_head[f]; i != NO_ID; i = S->u[i].next) if (S->u[i].t == T_STAIRS && LIVE(&S->u[i])) return true;
    return false;
}
static bool stairs_ok(int a, int b)
{
    int lo = a < b ? a : b, hi = a < b ? b : a;
    if (hi - lo > STAIR_MAX) return false;
    for (int f = lo; f < hi; f++) if (!stairs_at(f)) return false;
    return true;
}
static int qlen(int si, int f)
{
    const shaft_t *s = &S->s[si];
    int n = 0;
    for (uint16_t g = s->qh[qi(s, f)]; g != NO_ID && n < 99; g = S->g[g].next) n++;
    return n;
}
typedef struct { int walk, n; uint8_t s[2], f[2], t[2]; } route_t;
static bool usable(int i, int f) { return S->s[i].ncars && !S->s[i].build && tw_shaft_live(i) && tw_serves(&S->s[i], f); }
/* At most one lift change, only at a lobby floor (like the original's one transfer). */
static bool route(int a, int b, route_t *r)
{
    r->walk = 0; r->n = 0;
    if (a == b) return true;
    if (stairs_ok(a, b)) { r->walk = a > b ? a - b : b - a; return true; }
    int bq = 1 << 30;
    bool found = false;
    for (int i = 0; i < S->n_shafts; i++) {
        if (usable(i, a) && usable(i, b)) {
            int q = qlen(i, a);
            if (q < bq) { bq = q; found = true; r->n = 1; r->s[0] = (uint8_t)i; r->f[0] = (uint8_t)a; r->t[0] = (uint8_t)b; }
        }
    }
    if (found) return true;
    for (int k = 0; k < N_SKY; k++) {
        int t = SKY[k];
        if (t == a || t == b) continue;
        for (int i = 0; i < S->n_shafts; i++) {
            if (!usable(i, a) || !usable(i, t)) continue;
            for (int j = 0; j < S->n_shafts; j++) {
                if (i == j || !usable(j, t) || !usable(j, b)) continue;
                int q = qlen(i, a) + qlen(j, t);
                if (q < bq) {
                    bq = q; found = true; r->n = 2;
                    r->s[0] = (uint8_t)i; r->f[0] = (uint8_t)a; r->t[0] = (uint8_t)t;
                    r->s[1] = (uint8_t)j; r->f[1] = (uint8_t)t; r->t[1] = (uint8_t)b;
                }
            }
        }
    }
    if (found) return true;
    for (int d = 1; d <= 2; d++) {
        for (int k = 0; k < 2; k++) {
            int a2 = k ? a + d : a - d;
            if (a2 < 0 || a2 >= NF || !stairs_ok(a, a2)) continue;
            for (int i = 0; i < S->n_shafts; i++) {
                if (usable(i, a2) && usable(i, b)) {
                    r->walk = d; r->n = 1; r->s[0] = (uint8_t)i; r->f[0] = (uint8_t)a2; r->t[0] = (uint8_t)b;
                    return true;
                }
            }
        }
    }
    return false;
}
bool tw_route_ok(int a, int b) { route_t r; return route(a, b, &r); }
void tw_compute_access(void)
{
    bool lobby = false;
    for (uint16_t i = S->floor_head[BASE]; i != NO_ID; i = S->u[i].next) if (S->u[i].t == T_LOBBY && LIVE(&S->u[i])) lobby = true;
    for (int f = 0; f < NF; f++) S->access[f] = lobby && (f == BASE || tw_route_ok(BASE, f));
}
static int entry_floor(void)
{
    for (int i = 0; i < S->n_units; i++) {
        if (S->u[i].t == T_METRO && LIVE(&S->u[i])) return rn(10) < 3 ? S->u[i].f + 1 : BASE;
    }
    return BASE;
}

/* ------------------------------------------------------------ people: groups, not persons */
static void arrive(group_t *g);
static void enqueue(int gi)
{
    group_t *g = &S->g[gi];
    shaft_t *s = &S->s[g->ls[g->leg]];
    int q = qi(s, g->lf[g->leg]);
    g->st = ST_Q; g->next = NO_ID;
    if (s->qt[q] == NO_ID) s->qh[q] = s->qt[q] = (uint16_t)gi;
    else { S->g[s->qt[q]].next = (uint16_t)gi; s->qt[q] = (uint16_t)gi; }
}
static void free_group(int gi)
{
    S->g[gi].st = ST_FREE;
    S->g[gi].next = S->g_free;
    S->g_free = (uint16_t)gi;
    S->n_groups--;
}
static void fail_trip(int uid, int n)
{
    if (uid == NO_ID) return;
    unit_t *u = &S->u[uid];
    int v = u->s_sum + 60 * n;
    u->s_sum = (uint16_t)(v > 65535 ? 65535 : v);
    u->s_n = (uint16_t)(u->s_n + n);
}
static bool spawn(int from, int to, int n, int uid, int kind, int dest, int home)
{
    S->st.spawned += n;
    route_t r;
    if (!route(from, to, &r)) {
        S->st.failed += n;
        fail_trip(uid, n);
        if (uid != NO_ID && (kind == K_IN || kind == K_GUEST || kind == K_BACK)) S->u[uid].inside += (uint8_t)n;
        return false;
    }
    group_t tmp = {0};
    tmp.from = (uint8_t)from; tmp.to = (uint8_t)to; tmp.n = (uint8_t)n; tmp.u = (uint16_t)uid; tmp.kind = (uint8_t)kind;
    tmp.dest = (uint16_t)dest; tmp.home = (uint8_t)home;
    if (S->g_free == NO_ID) { S->st.overflow += n; arrive(&tmp); return true; }
    int gi = S->g_free;
    S->g_free = S->g[gi].next;
    S->n_groups++;
    if (S->n_groups > S->st.peak_groups) S->st.peak_groups = S->n_groups;
    group_t *g = &S->g[gi];
    *g = tmp;
    g->nlegs = (uint8_t)r.n;
    for (int i = 0; i < r.n; i++) { g->ls[i] = r.s[i]; g->lf[i] = r.f[i]; g->lt[i] = r.t[i]; }
    g->walk = (uint16_t)(r.walk * WALK_PER_FLOOR);
    if (uid != NO_ID && (kind == K_IN || kind == K_BACK)) S->u[uid].transit += (uint8_t)n;
    if (r.walk) g->st = ST_WALK; else if (r.n) enqueue(gi); else { arrive(g); free_group(gi); }
    return true;
}
static void later(int at, int from, int to, int n, int uid, int kind)
{
    if (S->n_pend >= MAX_PEND) { spawn(from, to, n, uid, kind == K_LEAVE ? K_OUT : kind, NO_ID, 0); return; }
    pend_t *p = &S->p[S->n_pend++];
    p->at = (uint16_t)(at >= T_DAY / T_MIN ? T_DAY / T_MIN - 1 : at); p->from = (uint8_t)from; p->to = (uint8_t)to;
    p->n = (uint8_t)n; p->u = (uint16_t)uid; p->kind = (uint8_t)kind;
}
static void vip_verdict(void);
static void arrive(group_t *g)
{
    S->st.arrived += g->n;
    if (g->nlegs) {
        S->st.wait_sum += (int64_t)g->wait * g->n; S->st.wait_n += g->n;
        if (g->wait > S->st.max_wait) S->st.max_wait = g->wait;
    }
    unit_t *u = g->u != NO_ID ? &S->u[g->u] : NULL;
    if (u && u->t == T_NONE) u = NULL;
    if (u) {
        int v = u->s_sum + g->wait / T_MIN * g->n + (g->wait % T_MIN) * g->n / T_MIN;
        u->s_sum = (uint16_t)(v > 65535 ? 65535 : v);
        u->s_n = (uint16_t)(u->s_n + g->n);
    }
    switch (g->kind) {
    case K_IN: case K_BACK: case K_GUEST:
        if (u) { u->transit = (uint8_t)(u->transit > g->n ? u->transit - g->n : 0); u->inside += g->n; }
        break;
    case K_LUNCH: case K_VISIT: case K_RESSHOP: {
        if (g->dest != NO_ID && S->u[g->dest].t != T_NONE) S->u[g->dest].cust += g->n;
        int stay = g->kind == K_LUNCH ? 30 : 45 + rn(45);
        later(MINUTE() + stay, g->to, g->home, g->n, g->u, g->kind == K_VISIT ? K_LEAVE : K_BACK);
        break;
    }
    case K_VIP: if (S->vip_state == 2) S->vip_state = 3; S->vip_wait += g->wait; break;
    case K_VIPOUT: S->vip_wait += g->wait; vip_verdict(); break;
    case K_WED: S->wed_arrived += g->n; break;
    default: break;
    }
}
static void step_pending(void)
{
    int m = MINUTE();
    for (int i = S->n_pend - 1; i >= 0; i--) {
        pend_t p = S->p[i];
        if (p.at <= m || m < 5) {
            S->p[i] = S->p[--S->n_pend];
            spawn(p.from, p.to, p.n, p.u == NO_ID ? NO_ID : p.u, p.kind == K_LEAVE ? K_OUT : p.kind, NO_ID, 0);
        }
    }
}

/* ------------------------------------------------------------ lifts: collective control */
static int g_to(const group_t *g) { return g->lt[g->leg]; }
static bool wants_stop(const shaft_t *s, const car_t *c, int f)
{
    for (uint16_t i = c->head; i != NO_ID; i = S->g[i].next) if (g_to(&S->g[i]) == f) return true;
    if (!tw_serves(s, f) || c->load >= (s->t == T_EXP ? CAP_EXP : CAP_LIFT)) return false;
    for (uint16_t i = s->qh[qi(s, f)]; i != NO_ID; i = S->g[i].next) {
        int up = g_to(&S->g[i]) > f ? 1 : -1;
        if (c->dir == 0 || up == c->dir) return true;
    }
    return false;
}
static bool calls_ahead(const shaft_t *s, const car_t *c, int dir)
{
    for (uint16_t i = c->head; i != NO_ID; i = S->g[i].next) if ((g_to(&S->g[i]) - c->f) * dir > 0) return true;
    for (int f = c->f + dir; f >= s->f0 && f <= s->f1; f += dir) if (tw_serves(s, f) && s->qh[qi(s, f)] != NO_ID) return true;
    return false;
}
static void step_car(shaft_t *s, car_t *c)
{
    if (c->door) { c->door--; return; }
    int f = c->f, cap = s->t == T_EXP ? CAP_EXP : CAP_LIFT;
    bool moved = false;
    uint16_t *pp = &c->head;
    while (*pp != NO_ID) {   // unload
        int gi = *pp;
        group_t *g = &S->g[gi];
        if (g_to(g) != f) { pp = &g->next; continue; }
        *pp = g->next;
        c->load -= g->n; moved = true; c->pout += g->n; c->stop_f = (int16_t)f;
        g->leg++;
        if (g->leg < g->nlegs) enqueue(gi); else { arrive(g); free_group(gi); }
    }
    if (c->dir == 0 || !calls_ahead(s, c, c->dir)) {
        int d0 = c->dir ? c->dir : 1;
        c->dir = (int8_t)(calls_ahead(s, c, d0) ? d0 : calls_ahead(s, c, -d0) ? -d0 : 0);
        if (c->dir == 0 && tw_serves(s, f) && s->qh[qi(s, f)] != NO_ID) c->dir = (int8_t)(g_to(&S->g[s->qh[qi(s, f)]]) > f ? 1 : -1);
    }
    if (tw_serves(s, f) && c->dir && !(c->load && c->dwell >= 2)) {   // load the ones going our way (doors close after 2 loads)
        int q = qi(s, f);
        uint16_t prev = NO_ID, i = s->qh[q];
        while (i != NO_ID) {
            group_t *g = &S->g[i];
            uint16_t nx = g->next;
            int up = g_to(g) > f ? 1 : -1;
            if (up == c->dir && c->load + g->n <= cap) {
                if (prev == NO_ID) s->qh[q] = nx; else S->g[prev].next = nx;
                if (s->qt[q] == i) s->qt[q] = prev;
                g->next = c->head; c->head = i; c->load += g->n; g->st = ST_RIDE;
                moved = true; c->pin += g->n; c->stop_f = (int16_t)f;
            } else prev = i;
            i = nx;
        }
    }
    if (moved) { c->door = DOOR_TICKS; c->dwell++; return; }
    if (!c->dir) return;
    c->dwell = 0;
    int sp = s->t == T_EXP ? EXP_SPEED : 1;
    for (int k = 0; k < sp; k++) {
        int nf = c->f + c->dir;
        if (nf < s->f0 || nf > s->f1) { c->dir = 0; break; }
        c->f = (uint8_t)nf;
        if (wants_stop(s, c, nf)) break;
    }
}
static void step_people(void)
{
    for (int i = 0; i < S->n_shafts; i++) {
        shaft_t *s = &S->s[i];
        if (s->build || !tw_shaft_live(i)) continue;   // power cut / broken shaft: cars idle
        for (int k = 0; k < s->ncars; k++) step_car(s, &s->car[k]);
    }
    if (!S->n_groups) return;
    for (int gi = 0; gi < MAX_GROUPS; gi++) {
        group_t *g = &S->g[gi];
        if (g->st == ST_Q) { if (g->wait < 65535) g->wait++; }
        else if (g->st == ST_WALK && --g->walk == 0) {
            if (g->nlegs) enqueue(gi); else { arrive(g); free_group(gi); }
        }
    }
}
int tw_waiting(int si, int f, int *max_wait, bool *up, bool *down)
{
    const shaft_t *s = &S->s[si];
    int n = 0, w = 0;
    if (up) *up = false;
    if (down) *down = false;
    if (!tw_serves(s, f)) { if (max_wait) *max_wait = 0; return 0; }
    for (uint16_t i = s->qh[qi(s, f)]; i != NO_ID; i = S->g[i].next) {
        const group_t *g = &S->g[i];
        n += g->n; if (g->wait > w) w = g->wait;
        if (g_to(g) > f) { if (up) *up = true; } else if (down) *down = true;
    }
    if (max_wait) *max_wait = w;
    return n;
}

/* ------------------------------------------------------------ the day: who goes where */
static bool in_win(int m, int a, int b) { return m >= a && m < b; }   // minutes of the day
static bool spawn_roll(int need, int m, int end) { int mins = end - m; if (mins < 1) mins = 1; return chance(need * 14, mins * 10); }
bool tw_is_open(const unit_t *u)
{
    const tdef_t *d = &g_T[u->t];
    if (S->typhoon) return false;
    if (d->open1) { int m = MINUTE(); return m >= d->open0 * 60 && m < d->open1 * 60; }
    return true;
}
static bool near_type(int t, int f, int range)
{
    for (int i = 0; i < S->n_units; i++) { const unit_t *o = &S->u[i]; if (o->t == t && LIVE(o) && (o->f > f ? o->f - f : f - o->f) <= range) return true; }
    return false;
}
static bool atrium_near(int f) { return near_type(T_ATRIUM, f, 4); }
static int nearest_open(int f, bool food_only, bool no_cinema)
{
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < S->n_units; i++) {
        const unit_t *u = &S->u[i];
        if (u->t == T_NONE || !IS_COM(u->t) || !u->occ || u->fire_h || u->build || !S->access[u->f]) continue;
        if (food_only && !IS_FOOD(u->t)) continue;
        if (u->f == S->flood_f && S->flood_h) continue;
        if (no_cinema && u->t != T_CCT && u->t != T_DIMSUM && u->t != T_SHOP) continue;
        if (!tw_is_open(u)) continue;
        int d = u->f > f ? u->f - f : f - u->f;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static void construction(void)
{
    bool done = false;
    char b[48];
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t == T_NONE || !u->build) continue;
        if (--u->build == 0) { char fn[8]; snprintf(b, sizeof b, "%s ON %s/F IS READY", g_T[u->t].en, tw_floor_name(u->f, fn)); tw_note(b); done = true; }
    }
    for (int i = 0; i < S->n_shafts; i++) {
        if (S->s[i].build && --S->s[i].build == 0) { snprintf(b, sizeof b, "%s IS RUNNING", g_T[S->s[i].t].en); tw_note(b); done = true; }
    }
    if (done) tw_compute_access();
}
static bool has_metro(void) { return has_type(T_METRO, true); }
static void minute_tick(void)
{
    construction();
    int m = MINUTE();
    bool we = weekend(), no_work = we || S->typhoon || (S->rain_h && (rn(2) == 0));
    bool metro = has_metro();
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t == T_NONE || !u->occ || u->fire_h || u->build) continue;
        switch (u->t) {
        case T_OFFICE:
            if (no_work) break;
            if (in_win(m, 450, 540)) {
                int need = OFFICE_POP - u->inside - u->transit;
                if (need > 0 && spawn_roll(need, m, 540)) { int n = 1 + rn(3); if (n > need) n = need; spawn(entry_floor(), u->f, n, i, K_IN, NO_ID, 0); }
            } else if (in_win(m, 720, 750) && u->inside >= 4 && !u->lunched) {
                u->lunched = 1;
                int food = nearest_open(u->f, true, false);
                if (food >= 0) { u->inside -= 3; spawn(u->f, S->u[food].f, 3, i, K_LUNCH, food, u->f); }
            } else if (in_win(m, 1020, 1110) && u->inside > 0) {
                if (spawn_roll(u->inside, m, 1110)) { int n = 1 + rn(3); if (n > u->inside) n = u->inside; u->inside -= n; spawn(u->f, BASE, n, i, K_OUT, NO_ID, 0); }
            }
            break;
        case T_FLAT:
            if (!we && in_win(m, 420, 510) && u->inside >= 3 && chance(5, 100)) { u->inside -= 2; u->away2 = 1; spawn(u->f, BASE, 2, i, K_OUT, NO_ID, 0); }
            else if (!we && in_win(m, 1020, 1200) && u->away2 && chance(3, 100)) { u->away2 = 0; spawn(entry_floor(), u->f, 2, i, K_BACK, NO_ID, 0); }
            else if (we && in_win(m, 600, 960) && u->inside >= 2 && !u->lunched && chance(1, 100)) {
                int sh = nearest_open(u->f, false, true);
                if (sh >= 0) { u->lunched = 1; u->inside -= 1; spawn(u->f, S->u[sh].f, 1, i, K_RESSHOP, sh, u->f); }
            }
            break;
        case T_SINGLE: case T_TWIN: case T_SUITE:
            if (in_win(m, 1020, 1260) && u->booked && !u->inside && !u->transit) {
                if (chance(2, 100)) { u->booked = 0; u->stay = 1; spawn(entry_floor(), u->f, g_T[u->t].guests, i, K_GUEST, NO_ID, 0); }
            } else if (in_win(m, 420, 600) && u->inside && u->stay == 2 && chance(3, 100)) {
                int n = u->inside; u->inside = 0; u->stay = 0;
                spawn(u->f, BASE, n, i, K_OUT, NO_ID, 0);
                int pay = g_T[u->t].night * (int[]){6, 10, 15}[u->rent] / 10;
                earn(pay); u->dirty = 1;
            }
            break;
        case T_CCT: case T_DIMSUM: case T_SHOP: {
            if (!tw_is_open(u)) break;
            const tdef_t *d = &g_T[u->t];
            int len = (d->open1 - d->open0) * 60;
            // target per day = base * (we ? 1.35 : 1) * (0.3 + eval/100) * (metro ? 1.3 : 1); groups average 2.5 people
            int64_t tgt = (int64_t)d->base * (we ? 135 : 100) * (30 + u->eval) * (metro ? 13 : 10);   // x 100*100*10
            tgt = tgt * (atrium_near(u->f) ? 13 : 10) / 10 * (S->celeb ? 15 : 10) / 10;
            if (S->rain_h) tgt = tgt / 2;
            if (S->alarm_h) tgt = tgt / 4;
            if (S->fest) tgt = tgt * 13 / 10;
            if (S->typhoon_sig == 3) tgt = tgt * 6 / 10;
            if (chance((int)(tgt / 1000), len * 25 * 10)) spawn(entry_floor(), u->f, 1 + rn(4), NO_ID, K_VISIT, i, BASE);
            break;
        }
        case T_MHUB: case T_SPA: case T_FOSSIL: case T_GEM: case T_NMARKET: case T_AQUA: {   // underground sights
            if (!tw_is_open(u) || u->f == S->flood_f) break;
            const tdef_t *d = &g_T[u->t];
            int len = (d->open1 - d->open0) * 60;
            int64_t tgt = (int64_t)d->base * (we ? 150 : 100) * (30 + u->eval) * (S->celeb ? 15 : 10) / (vent_ok(u->f) ? 1 : 2);
            if (chance((int)(tgt / 1000), len * 25 * 10)) spawn(entry_floor(), u->f, 1 + rn(4), NO_ID, K_VISIT, i, BASE);
            break;
        }
        case T_POOL: case T_ICE: case T_CLIMB: case T_BANQUET: {   // leisure: shop-like, ice rink doubles at weekends, banquets 18:00-23:00
            if (!tw_is_open(u) || (u->t == T_BANQUET && m < 1080)) break;
            const tdef_t *d = &g_T[u->t];
            int len = u->t == T_BANQUET ? 300 : (d->open1 - d->open0) * 60;
            int64_t tgt = (int64_t)d->base * (u->t == T_ICE ? (we ? 220 : 100) : (we ? 140 : 100)) * (30 + u->eval) * (S->celeb ? 15 : 10);
            if (chance((int)(tgt / 1000), len * 25 * 10)) spawn(entry_floor(), u->f, u->t == T_BANQUET ? 6 + rn(5) : 1 + rn(4), NO_ID, K_VISIT, i, BASE);
            break;
        }
        case T_CINEMA:
            if (S->typhoon) break;
            if (in_win(m, 720, 780) || in_win(m, 1080, 1140)) {
                int tgt = g_T[T_CINEMA].base * (we ? 15 : 10) * (30 + u->eval);   // x 1000
                if (chance(tgt, 60 * 4 * 1000)) spawn(entry_floor(), u->f, 4, NO_ID, K_VISIT, i, BASE);
            }
            break;
        case T_MEGA:   // 3 shows: matinee, prime time (biggest), late show; audience enters on the bottom floor
            if (S->typhoon) break;
            {
                int k = in_win(m, 840, 900) ? 6 : in_win(m, 1080, 1140) ? 15 : in_win(m, 1230, 1290) ? 9 : 0;
                int tgt = g_T[T_MEGA].base * (we ? 15 : 10) * (30 + u->eval) * k / 10;
                if (k && chance(tgt, 60 * 4 * 1000)) spawn(entry_floor(), u->f, 4, NO_ID, K_VISIT, i, BASE);
            }
            break;
        case T_COASTER: {   // open 10-23, evenings x1.6 and weekends x2
            if (!tw_is_open(u)) break;
            int pk = (m >= 1080 ? 16 : 10) * (we ? 2 : 1);
            int64_t tgt = (int64_t)g_T[T_COASTER].base * pk * (30 + u->eval);
            if (chance((int)(tgt / 10), 13 * 60 * 25 * 10)) spawn(entry_floor(), u->f, 2 + rn(3), NO_ID, K_VISIT, i, BASE);
            break;
        }
        default: break;
        }
    }
    if (S->vip_state == 1 && in_win(m, 1080, 1140)) { S->vip_state = 2; spawn(BASE, S->u[S->vip_room].f, 1, NO_ID, K_VIP, NO_ID, 0); }
    if (S->vip_state == 4 && in_win(m, 600, 660)) { S->vip_state = 5; spawn(S->u[S->vip_room].f, BASE, 1, NO_ID, K_VIPOUT, NO_ID, 0); }
    if (S->wedding && !S->wed_sent && we && in_win(m, 660, 720)) {
        S->wed_sent = true; S->wed_arrived = 0;
        for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_CHAPEL) for (int k = 0; k < 6; k++) spawn(BASE, S->u[i].f, 5, NO_ID, K_WED, NO_ID, 0);
        for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_BANQUET && LIVE(&S->u[i])) { for (int k = 0; k < 4; k++) spawn(BASE, S->u[i].f, 8, NO_ID, K_WED, NO_ID, 0); tw_note("WEDDING BANQUET TONIGHT"); break; }
    }
    if (S->wed_sent && !S->tower_done && S->wed_arrived >= 30) { S->tower_done = true; S->star = 6; S->celebrate = 1; tw_note("A SKY WEDDING! TOWER RATING!"); }
}

/* ------------------------------------------------------------ new day, quarter */
static bool housekeeping_for(const unit_t *u)
{
    for (int i = 0; i < S->n_units; i++) {
        const unit_t *h = &S->u[i];
        if (h->t == T_HOUSEK && LIVE(h) && (h->f > u->f ? h->f - u->f : u->f - h->f) <= HOUSEKEEP_RANGE) return true;
    }
    return false;
}
static void eval_unit(unit_t *u)
{
    if (!IS_TENANT(u->t)) return;
    if (!S->access[u->f]) { u->eval = 0; return; }
    int wait10 = u->s_n ? u->s_sum * 10 / u->s_n : u->last_wait;   // avg wait, tenths of a minute
    u->last_wait = (uint8_t)(wait10 > 255 ? 255 : wait10);
    int pen = wait10 > 70 ? 70 : wait10;
    int e = 78 - pen + (int[]){10, 0, -15}[u->rent];
    for (uint16_t i = S->floor_head[u->f]; i != NO_ID; i = S->u[i].next) {
        const unit_t *o = &S->u[i];
        if (o == u) continue;
        int gap1 = o->x - (u->x + u->w), gap2 = u->x - (o->x + o->w), gap = gap1 > gap2 ? gap1 : gap2;
        if (gap > 6) continue;
        if (u->t == T_OFFICE && IS_COM(o->t)) e -= 15;
        if ((FL(u->t) & F_QUIET) && (IS_COM(o->t) || o->t == T_OFFICE)) e -= 15;
    }
    if (u->t == T_OFFICE && (has_type(T_CARPARK, true) || has_type(T_PARKING, true))) e += 8;      // car park: office rating
    if ((u->t == T_FLAT || IS_HOTEL(u->t)) && near_type(T_POOL, u->f, 15)) e += 8;                 // pool: condo / hotel rating
    if (near_type(T_GARDEN, u->f, 8)) e += 10;
    if (u->f < BASE && !vent_ok(u->f)) e -= 20;                                               // stale air down deep
    if (S->quake_h) e -= 15;                                                                  // still shaken                                                   // sky garden: calm, offsets noise
    for (int i = 0; i < S->n_units; i++) {   // coaster noise: rooms beside it (any of its 5 floors) and just above / below
        const unit_t *o = &S->u[i];
        if (o->t != T_COASTER || !LIVE(o) || u->f + 1 < o->f || u->f > o->f + 5) continue;
        int gap1 = o->x - (u->x + u->w), gap2 = u->x - (o->x + o->w), gap = gap1 > gap2 ? gap1 : gap2;
        if (gap <= 6) e -= (FL(u->t) & F_QUIET) ? 30 : 15;
    }
    if (IS_HOTEL(u->t)) { if (!housekeeping_for(u)) e -= 25; if (u->roach) e = 0; }
    if (u->t == T_FLAT && S->star >= 3) {
        bool near = false;
        for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_CLINIC && (S->u[i].f > u->f ? S->u[i].f - u->f : u->f - S->u[i].f) <= 15) near = true;
        if (!near) e -= 10;
    }
    if (e < 0) e = 0;
    if (e > 100) e = 100;
    u->eval = (uint8_t)((e + u->eval + 1) / 2);
    u->s_sum = 0; u->s_n = 0;
}
static void housekeeping(void)
{
    for (int i = 0; i < S->n_units; i++) {
        const unit_t *h = &S->u[i];
        if (h->t != T_HOUSEK || !LIVE(h)) continue;
        int n = HOUSEKEEP_ROOMS;
        for (int j = 0; j < S->n_units && n > 0; j++) {
            unit_t *u = &S->u[j];
            if (u->t != T_NONE && IS_HOTEL(u->t) && u->dirty && !u->roach && (u->f > h->f ? u->f - h->f : h->f - u->f) <= HOUSEKEEP_RANGE) { u->dirty = 0; u->dirty_days = 0; n--; }
        }
    }
}
static void new_quarter(void)
{
    int64_t rent = 0, m = 0;
    char fn[8];
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t == T_OFFICE && u->occ) {
            if (u->eval < 35) { u->occ = 0; u->inside = 0; notes("OFFICE ON %s/F MOVED OUT", tw_floor_name(u->f, fn)); continue; }
            rent += OFFICE_RENT * (int[]){6, 10, 15}[u->rent] / 10;
        }
        if (u->t == T_FLAT && u->occ && u->eval < 25) { u->occ = 0; u->inside = 0; }
        if (u->t != T_NONE) m += g_T[u->t].maint;
        if (u->t == T_CARPARK && LIVE(u)) rent += 40000 * tw_uh(u);     // season parking
        if (u->t == T_STUDIO && LIVE(u)) rent += 250000;                // studio lease
        if (u->t == T_LAB && LIVE(u)) rent += 400000;                   // secret lab contract
        if (u->t == T_GEO && LIVE(u)) rent += 600000;                   // geothermal power sales
        if (u->t == T_VAULT && LIVE(u)) rent += 1000000;                // vault storage
        if (u->t == T_LOBBY && S->star >= 3) m += 300;
    }
    earn(rent); S->st.rent_q = (int32_t)rent;
    for (int i = 0; i < S->n_shafts; i++) m += (int64_t)g_T[S->s[i].t].maint * S->s[i].ncars;
    spend(m);
    if (S->funds < GRANT_BELOW && S->grant_day != (int32_t)S->day) {
        S->grant_day = (int32_t)S->day;
        int64_t g = GRANT - (S->funds < 0 ? S->funds : 0);
        earn(g); S->st.grants += (int32_t)g;
        notei("SMALL BUSINESS GRANT +$%dK", (int)(g / 1000));
    }
    if (quarter() == 0) { earn(20000 * S->star); notei("KUNG HEI FAT CHOI! +$%dK", 20 * S->star); }
    S->st.q_in = S->st.q_out = 0;
}
void tw_population(void)
{
    int p = 0;
    for (int i = 0; i < S->n_units; i++) {
        const unit_t *u = &S->u[i];
        if (u->t == T_NONE || !u->occ) continue;
        if (u->t == T_OFFICE) p += OFFICE_POP;
        else if (u->t == T_FLAT) p += FLAT_POP;
        else if (IS_HOTEL(u->t)) p += (u->inside || u->stay) ? g_T[u->t].guests : 0;
        else if (IS_COM(u->t)) p += u->cust_y;
    }
    S->pop = p;
}
static int star_pop(int n) { return g_star_pop_override >= 0 && n >= 4 ? g_star_pop_override : STAR_POP[n > 6 ? 6 : n]; }   // CORE (7) keeps the TOWER population bar
int tw_star_reqs(int n, const char *names[8], bool ok[8])
{
    static char popn[16];
    int k = 0;
    snprintf(popn, sizeof popn, "POP %d", star_pop(n));
    names[k] = popn; ok[k++] = S->pop >= star_pop(n);
    if (n == 3) { names[k] = "SECURITY"; ok[k++] = has_type(T_SECURITY, true); }
    if (n >= 3) { names[k] = "REFUGE FLOORS"; ok[k++] = tw_uncovered_floor() < 0; }
    if (n == 4) {
        names[k] = "2 SUITES"; ok[k++] = count_type(T_SUITE) >= 2;
        names[k] = "CLINIC"; ok[k++] = has_type(T_CLINIC, true);
        names[k] = "RECYCLING"; ok[k++] = has_type(T_RECYCLE, true);
        names[k] = "CAR PARK"; ok[k++] = has_type(T_PARKING, true) || has_type(T_CARPARK, true);
        names[k] = "VIP VISIT"; ok[k++] = S->vip_ok;
    }
    if (n == 5) { names[k] = "METRO"; ok[k++] = has_type(T_METRO, true); }
    if (n == 7) {   // CORE rating (Underground challenge)
        static const uint8_t SIGHTS[8] = {T_MHUB, T_SPA, T_FOSSIL, T_GEM, T_NMARKET, T_AQUA, T_LAB, T_GEO};
        int sights = 0;
        for (int i = 0; i < 8; i++) sights += has_type(SIGHTS[i], true);
        static char sn[16];
        snprintf(sn, sizeof sn, "%d/6 DEEP SIGHTS", sights);
        names[k] = "DIG TO B100"; ok[k++] = S->dug == 0;
        names[k] = "CORE VAULT"; ok[k++] = has_type(T_VAULT, true);
        names[k] = "PUMPS"; ok[k++] = tw_deep_uncovered(0) < 0;
        names[k] = "VENTS"; ok[k++] = tw_deep_uncovered(1) < 0;
        names[k] = sn; ok[k++] = sights >= 6;
    }
    if (n == 6) { names[k] = "SKY CHAPEL"; ok[k++] = has_type(T_CHAPEL, true); names[k] = "WEDDING"; ok[k++] = S->tower_done; }
    return k;
}
static bool reqs_met(int n, bool skip_vip)
{
    const char *nm[8]; bool ok[8];
    int k = tw_star_reqs(n, nm, ok);
    for (int i = 0; i < k; i++) if (!ok[i] && !(skip_vip && strcmp(nm[i], "VIP VISIT") == 0)) return false;
    return true;
}
static void check_stars(void)
{
    while (S->star < 5 && reqs_met(S->star + 1, false)) { S->star++; notei("TOWER PROMOTED TO %d STARS!", S->star); }
    if (S->star == 5 && !S->wedding && S->pop >= star_pop(6) && has_type(T_CHAPEL, true)) { S->wedding = true; S->wed_sent = false; tw_note("A WEDDING IS BOOKED"); }
}
static void vip_verdict(void)
{
    const unit_t *room = &S->u[S->vip_room];
    bool good = S->vip_wait < 4 * T_MIN && room->eval >= 55 && !room->roach && S->vip_state >= 3;
    S->vip_ok = good;
    tw_note(good ? "THE VIP LOVED IT!" : "THE VIP WAS NOT IMPRESSED");
    S->vip_state = 0;
    check_stars();
}
static void events(void)
{
    if (S->star == 3 && !S->vip_ok && S->vip_state == 0 && reqs_met(4, true) && chance(35, 100)) {
        int best = -1;
        for (int i = 0; i < S->n_units; i++) {
            const unit_t *u = &S->u[i];
            if (u->t == T_SUITE && LIVE(u) && u->occ && !u->roach && !u->dirty && !u->inside && (best < 0 || u->eval > S->u[best].eval)) best = i;
        }
        if (best >= 0) { S->vip_room = (uint16_t)best; S->vip_state = 1; S->vip_wait = 0; S->u[best].booked = 0; tw_note("A VIP IS COMING TONIGHT!"); }
    } else if (S->vip_state == 3) S->vip_state = 4;
    if (g_events_on && S->fire_u == NO_ID && S->n_units > 20 && chance(1, 100)) {
        int id = rn(S->n_units);
        unit_t *u = &S->u[id];
        if (u->t != T_NONE && IS_TENANT(u->t) && u->f > BASE && LIVE(u)) {
            bool guard = false;
            for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_SECURITY && LIVE(&S->u[i]) && (S->u[i].f > u->f ? S->u[i].f - u->f : u->f - S->u[i].f) <= SECURITY_RANGE) guard = true;
            bool safe = refuge_covered(u->f);
            u->fire_h = (guard ? 2 : 8) * (safe ? 1 : 2);
            S->fire_u = (uint16_t)id;
            char fn[8];
            notes(!safe ? "FIRE ON %s/F! NO REFUGE FLOOR" : guard ? "FIRE ON %s/F! SECURITY IS ON IT" : "FIRE ON %s/F!", tw_floor_name(u->f, fn));
        }
    }
    /* ---- kid-friendly HK-flavoured random events (banner + brief effect, never softlock) ---- */
    if (!g_events_on) return;
    int yd = (int)(S->day % 36);   // a "year" is 12 weeks x 3 days
    S->fest = 0; S->santa = 0;
    if (yd == 20) { S->fest = 2; tw_note("MID-AUTUMN! MOONCAKES FOR ALL"); }             // 中秋
    else if (yd == 33) { S->fest = 1; S->santa = 1; tw_note("MERRY CHRISTMAS! SANTA IS HERE"); }
    else if (yd == 34 && chance(1, 2)) { S->santa = 1; tw_note("SANTA CLAUS IS RIDING THE LIFTS!"); }

    if (!S->typhoon && !S->rain_h && !S->power_h && chance(8, 100)) {                   // 黑色暴雨
        S->rain_h = 4 + rn(4);
        tw_note("BLACK RAIN WARNING! STAY INDOORS");
    }
    if (!S->power_h && S->n_shafts > 0 && chance(4, 100)) {                             // 停電
        S->power_h = 6 + rn(6);
        tw_note("POWER CUT! LIFTS STOPPED - PAY TO FIX");
    }
    if (S->broken_si == 0xff && S->n_shafts > 0 && chance(5, 100)) {                    // 升降機故障
        int si = rn(S->n_shafts);
        if (!S->s[si].build) { S->broken_si = (uint8_t)si; S->broken_h = 12 + rn(12); tw_note("LIFT BROKEN - PAY TO FIX"); }
    }
    if (!S->celeb && !S->typhoon && chance(4, 100)) {                                  // surprise celebrity (no studio needed)
        S->celeb = 1; tw_note("SURPRISE! A STAR IS IN THE TOWER");
    }
    if (!S->quake_h && chance(3, 100)) {                                               // 小地震
        S->quake_h = 8 + rn(8);
        for (int i = 0; i < S->n_units; i++) { unit_t *u = &S->u[i]; if (u->t != T_NONE && u->eval > 10) u->eval -= 10; }
        if (S->dig_h) S->dig_h = (uint16_t)(S->dig_h + 3 + rn(4));                     // dig delay underground
        tw_note("EARTHQUAKE! EVERYONE IS SHAKEN");
    }
    if (!S->alarm_h && has_type(T_SECURITY, true) && chance(4, 100)) {                 // false robbery alarm
        S->alarm_h = 2 + rn(2);
        tw_note("FALSE ALARM! SECURITY IS CHECKING");
    }
    if (!S->complain && S->vip_ok && S->star >= 4 && chance(5, 100)) {                 // VIP complaint
        S->complain = 1;
        for (int i = 0; i < S->n_units; i++) { unit_t *u = &S->u[i]; if (u->t == T_SUITE && LIVE(u) && u->eval > 5) u->eval -= 5; }
        tw_note("VIP COMPLAINT: TOO SLOW LIFTS!");
    }
}
static void hour_tick(void)
{
    if (S->fire_u != NO_ID) {
        unit_t *u = &S->u[S->fire_u];
        if (u->t == T_NONE) S->fire_u = NO_ID;
        else if (--u->fire_h == 0) {
            S->fire_u = NO_ID;
            u->eval = u->eval > 30 ? u->eval - 30 : 0;
            bool guard = false;
            for (int i = 0; i < S->n_units; i++) if (S->u[i].t == T_SECURITY && LIVE(&S->u[i]) && (S->u[i].f > u->f ? S->u[i].f - u->f : u->f - S->u[i].f) <= SECURITY_RANGE) guard = true;
            if (!refuge_covered(u->f)) {   // no refuge floor: the fire spreads along the whole floor
                for (uint16_t j = S->floor_head[u->f]; j != NO_ID; j = S->u[j].next) {
                    unit_t *o = &S->u[j];
                    if (IS_TENANT(o->t) && o->occ) { o->occ = 0; o->inside = 0; o->eval = o->eval > 50 ? o->eval - 50 : 0; }
                }
                S->st.fires_big++;
                tw_note("NO REFUGE FLOOR: FIRE GUTTED THE FLOOR");
            } else if (!guard) {
                u->occ = 0; u->inside = 0; tw_note("THE FIRE GUTTED A ROOM");
                if (!S->power_h) { S->power_h = 4; tw_note("BLACKOUT AFTER THE FIRE!"); }
            } else tw_note("FIRE OUT");
        }
    }
    if (S->dig_h && --S->dig_h == 0) {
        char fn[8]; S->dug--; notes("%s EXCAVATED", tw_floor_name(S->dug, fn));
        if (chance(18, 100)) {   // treasure find when digging (kid-friendly HK flavour)
            int32_t t = 50000 + rn(450000);
            earn(t); notei("TREASURE FOUND! +$%dK", (int)(t / 1000));
        }
    }
    if (S->power_h && --S->power_h == 0) tw_note("POWER IS BACK");
    if (S->rain_h && --S->rain_h == 0) tw_note("THE BLACK RAIN HAS STOPPED");
    if (S->quake_h && --S->quake_h == 0) tw_note("THE SHAKING HAS STOPPED");
    if (S->alarm_h && --S->alarm_h == 0) tw_note("FALSE ALARM - ALL CLEAR");
    if (S->broken_h && --S->broken_h == 0 && S->broken_si != 0xff) { S->broken_si = 0xff; tw_note("THE LIFT FIXED ITSELF"); }
    if (S->flood_h && --S->flood_h == 0) { S->flood_f = 0xff; tw_note("THE WATER IS PUMPED OUT"); }
    tw_population();
}
void tw_new_day(void)
{
    S->day++;
    S->tick = 0;
    bool q = S->day % 3 == 0;
    char b[48], fn[8];
    S->complain = 0;
    S->typhoon_sig = 0; S->typhoon = false;
    if (g_events_on && quarter() == 2 && day_kind() < 2) {   // typhoon season: Signal 1 / 3 / 8
        int r = rn(100);
        if (r < 8) { S->typhoon_sig = 8; S->typhoon = true; tw_note("TYPHOON SIGNAL 8: OFFICES+SHOPS SHUT"); }
        else if (r < 18) { S->typhoon_sig = 3; tw_note("TYPHOON SIGNAL 3: STRONG WIND"); }
        else if (r < 30) { S->typhoon_sig = 1; tw_note("TYPHOON SIGNAL 1: STAND BY"); }
    }
    S->celeb = !S->typhoon && has_type(T_STUDIO, true) && chance(1, 6);
    if (S->deep && !S->flood_h) {   // seepage: an unpumped deep floor floods now and then
        int ff = tw_deep_uncovered(0);
        if (ff >= 0 && chance(3, 10)) {
            S->flood_f = (uint8_t)ff; S->flood_h = 6;
            for (uint16_t i = S->floor_head[ff]; i != NO_ID; i = S->u[i].next) { unit_t *o = &S->u[i]; o->eval = o->eval > 20 ? o->eval - 20 : 0; }
            notes("FLOOD ON %s! BUILD PUMP ROOMS", tw_floor_name(ff, fn));
        }
    }
    if (S->deep && !S->core_done && S->tower_done) {
        const char *nm[8]; bool okk[8]; int k = tw_star_reqs(7, nm, okk), all = 1;
        for (int i = 0; i < k; i++) all &= okk[i];
        if (all) { S->core_done = 1; S->celebrate = 2; tw_note("CORE RATING! THE DEEPEST TOWER ON EARTH"); }
    }
    if (S->celeb) tw_note("CELEBRITY AT THE TV STUDIO! CROWDS!");
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t == T_NONE) continue;
        u->lunched = 0;
        eval_unit(u);
        if (IS_COM(u->t)) {
            u->cust_y = (uint8_t)(u->cust > 255 ? 255 : u->cust);
            if (u->occ) earn((int64_t)u->cust * g_T[u->t].per_cust);
            u->low_days = (uint8_t)(u->occ && u->cust < 8 ? u->low_days + 1 : 0);
            if (u->low_days >= 4) { u->occ = 0; u->low_days = 0; snprintf(b, sizeof b, "%s ON %s/F CLOSED", g_T[u->t].en, tw_floor_name(u->f, fn)); tw_note(b); }
            u->cust = 0;
        }
        if (IS_HOTEL(u->t)) {
            if (u->stay == 1) u->stay = 2;
            if (u->dirty && ++u->dirty_days >= 3 && !u->roach) { u->roach = 1; notes("COCKROACHES ON %s/F!", tw_floor_name(u->f, fn)); }
        }
    }
    housekeeping();
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t == T_NONE || !IS_TENANT(u->t) || u->occ || !S->access[u->f] || u->fire_h || u->build) continue;
        if (IS_HOTEL(u->t)) { if (!u->roach) u->occ = 1; continue; }
        int pre = 60 + (int[]){10, 0, -15}[u->rent];
        if (pre < 40 || rn(100) >= 50) continue;
        u->occ = 1;
        if (u->eval < 60) u->eval = 60;
        if (u->t == T_FLAT) {
            int v = FLAT_SALE * (int[]){8, 10, 12}[u->rent] / 10;
            earn(v); u->inside = 3;
            snprintf(b, sizeof b, "FLAT SOLD ON %s/F +$%dK", tw_floor_name(u->f, fn), v / 1000); tw_note(b);
        }
        if (u->t == T_OFFICE) u->inside = 0;
    }
    for (int i = 0; i < S->n_units; i++) {
        unit_t *u = &S->u[i];
        if (u->t != T_NONE && IS_HOTEL(u->t) && u->occ && !u->roach && !u->dirty && !u->inside && !(S->vip_state && S->vip_room == i))
            u->booked = rn(100) < u->eval * (weekend() ? 10 : 8) / 10;
    }
    if (S->vip_state == 2) { S->vip_wait = 1000000; vip_verdict(); }
    if (q) new_quarter();
    tw_population();
    check_stars();
    events();
}

/* ------------------------------------------------------------ one tick, one frame */
void tw_tick(void)
{
    S->tick++;
    step_people();
    if (S->tick % T_MIN == 0) { step_pending(); minute_tick(); }
    if (S->tick % T_HOUR == 0) hour_tick();
    if (S->tick >= T_DAY) tw_new_day();
}
/* lift drawing state: cars glide (Q8 floors), doors slide, people walk in/out */
void tw_animate(void)
{
    for (int i = 0; i < S->n_shafts; i++) {
        shaft_t *s = &S->s[i];
        for (int k = 0; k < s->ncars; k++) {
            car_t *c = &s->car[k];
            if (c->stop_f >= 0) {
                c->hold_f = (uint8_t)c->stop_f; c->hold = 16;
                int add = (c->pout > 3 ? 3 : c->pout);
                for (int j = 0; j < add && c->n_anim < 6; j++) { c->anim_t[c->n_anim] = (int8_t)(-j * 3); c->anim_out[c->n_anim++] = 1; }
                add = c->pin > 3 ? 3 : c->pin;
                for (int j = 0; j < add && c->n_anim < 6; j++) { c->anim_t[c->n_anim] = (int8_t)(-j * 3); c->anim_out[c->n_anim++] = 0; }
                c->pin = c->pout = 0; c->stop_f = -1;
            }
            int tgt = (c->hold ? c->hold_f : c->f) << 8, diff = tgt - c->disp, ad = diff < 0 ? -diff : diff;
            if (ad > 5 && c->door_a) c->door_a = (uint8_t)(c->door_a > 64 ? c->door_a - 64 : 0);   // close the doors, then go
            else if (ad > 5) {
                int st = ad * 22 / 100;
                if (st < 26) st = 26;
                if (st > ad) st = ad;
                c->disp += diff > 0 ? st : -st;
            } else if (ad <= 5) {
                c->disp = tgt;
                if (c->hold) { c->hold--; c->door_a = (uint8_t)(c->hold > 4 ? (c->door_a + 64 > 255 ? 255 : c->door_a + 64) : (c->door_a > 64 ? c->door_a - 64 : 0)); }
            }
            if (!c->hold && c->door_a) c->door_a = (uint8_t)(c->door_a > 64 ? c->door_a - 64 : 0);
            int n = 0;
            for (int j = 0; j < c->n_anim; j++) {
                if (++c->anim_t[j] < 14) { c->anim_t[n] = c->anim_t[j]; c->anim_out[n++] = c->anim_out[j]; }
            }
            c->n_anim = (uint8_t)n;
        }
    }
    if (S->msg_t) S->msg_t--;
}

/* ------------------------------------------------------------ new tower, save, load */
static void reset_pools(void)
{
    for (int f = 0; f < NF; f++) S->floor_head[f] = NO_ID;
    for (int i = 0; i < MAX_GROUPS; i++) { S->g[i].st = ST_FREE; S->g[i].next = (uint16_t)(i + 1 < MAX_GROUPS ? i + 1 : NO_ID); }
    S->g_free = 0; S->n_groups = 0; S->n_pend = 0;
    for (int i = 0; i < MAX_UNITS; i++) S->u[i].t = T_NONE;
}
void tw_new(uint32_t seed)
{
    memset(S, 0, sizeof *S);
    reset_pools();
    S->rng = seed ? seed : 1;
    S->tick = 6 * T_HOUR; S->funds = START_FUNDS; S->star = 1; S->speed = 1;
    S->owned[0] = S->owned[1] = 1;
    S->vip_room = NO_ID; S->fire_u = NO_ID; S->grant_day = -1;
    S->last_t = T_OFFICE; S->last_v = 0xff;
    S->dug = BASE - 4; S->flood_f = 0xff; S->broken_si = 0xff;   // B1-B4 dug; no broken lift
    tw_compute_access();
}
static uint64_t get_le(const uint8_t *p, int sz);
#define MAGIC 0x34525754u   /* "TWR4": 2 save slots + event state */
int tw_save(uint8_t *b)
{
    int n = 0;
#define PUT(v, sz) do { uint64_t _v = (uint64_t)(v); for (int _i = 0; _i < (sz); _i++) b[n++] = (uint8_t)(_v >> (8 * _i)); } while (0)
    PUT(MAGIC, 4); PUT(S->day, 4); PUT(S->funds, 8); PUT(S->star, 1);
    PUT(S->vip_ok | S->wedding << 1 | S->tower_done << 2 | S->rush << 3 | S->deep << 4 | S->core_done << 5, 1);
    PUT(S->dug, 1); PUT(S->dig_h, 2);
    PUT(S->power_h, 1); PUT(S->rain_h, 1); PUT(S->quake_h, 1); PUT(S->alarm_h, 1);
    PUT(S->broken_si, 1); PUT(S->broken_h, 1); PUT(S->fest, 1); PUT(S->typhoon_sig, 1);
    PUT(S->owned[2] | S->owned[3] << 1, 1); PUT(S->speed, 1); PUT(S->last_t, 1); PUT(S->last_v, 1); PUT(S->rng, 4);
    for (int f = S->dug; f < NF; f++) PUT(S->slab[f], 8);   // nothing below the dug floor
    int nu = 0;
    for (int i = 0; i < S->n_units; i++) if (S->u[i].t != T_NONE) nu++;
    PUT(nu, 2); PUT(S->n_shafts, 1);
    for (int i = 0; i < S->n_units; i++) {
        const unit_t *u = &S->u[i];
        if (u->t == T_NONE) continue;
        PUT(u->t, 1); PUT(u->f, 1); PUT(u->x, 1); PUT(u->w, 1); PUT(u->v, 1);
        PUT(u->occ | u->rent << 1 | u->dirty << 3 | u->roach << 4, 1); PUT(u->eval, 1); PUT(u->cust_y, 1); PUT(u->dirty_days, 1); PUT(u->build, 2);
    }
    for (int i = 0; i < S->n_shafts; i++) {
        const shaft_t *s = &S->s[i];
        PUT(s->t, 1); PUT(s->x, 1); PUT(s->f0, 1); PUT(s->f1, 1); PUT(s->ncars, 1); PUT(s->build, 2);
    }
    uint32_t sum = 0;
    for (int i = 0; i < n; i++) sum = sum * 31 + b[i];
    PUT(sum, 4);
#undef PUT
    return n;
}
bool tw_load(const uint8_t *b, int len)
{
    int n = 0;
#define GET(sz) (n += (sz), get_le(b + n - (sz), (sz)))
    if (len < 4 + 8 + 30) return false;
    uint32_t sum = 0;
    for (int i = 0; i < len - 4; i++) sum = sum * 31 + b[i];
    if ((uint32_t)get_le(b + len - 4, 4) != sum || (uint32_t)GET(4) != MAGIC) return false;
    uint32_t seed = 1;
    tw_new(seed);
    S->day = (uint32_t)GET(4); S->funds = (int64_t)GET(8); S->star = (uint8_t)GET(1);
    int fl = (int)GET(1);
    S->vip_ok = fl & 1; S->wedding = fl >> 1 & 1; S->tower_done = fl >> 2 & 1; S->rush = fl >> 3 & 1; S->deep = fl >> 4 & 1; S->core_done = fl >> 5 & 1;
    S->dug = (uint8_t)GET(1); S->dig_h = (uint16_t)GET(2);
    S->power_h = (uint8_t)GET(1); S->rain_h = (uint8_t)GET(1); S->quake_h = (uint8_t)GET(1); S->alarm_h = (uint8_t)GET(1);
    S->broken_si = (uint8_t)GET(1); S->broken_h = (uint8_t)GET(1); S->fest = (uint8_t)GET(1); S->typhoon_sig = (uint8_t)GET(1);
    S->typhoon = S->typhoon_sig >= 8;
    if (S->dug > BASE - 4) { tw_new(1); return false; }
    S->flood_f = 0xff;
    int ow = (int)GET(1);
    S->owned[2] = ow & 1; S->owned[3] = ow >> 1 & 1;
    S->speed = (uint8_t)GET(1); S->last_t = (uint8_t)GET(1); S->last_v = (uint8_t)GET(1); S->rng = (uint32_t)GET(4);
    if (!S->rng) S->rng = 1;
    if (S->speed > 3 || !S->owned[S->speed]) S->speed = 1;
    S->tick = 0;
    for (int f = S->dug; f < NF; f++) S->slab[f] = GET(8);
    int nu = (int)GET(2), ns = (int)GET(1);
    if (nu > MAX_UNITS || ns > MAX_SHAFTS || len != n + nu * 11 + ns * 7 + 4) { tw_new(1); return false; }
    for (int i = 0; i < nu; i++) {
        unit_t *u = &S->u[i];
        memset(u, 0, sizeof *u);
        u->t = (uint8_t)GET(1); u->f = (uint8_t)GET(1); u->x = (uint8_t)GET(1); u->w = (uint8_t)GET(1); u->v = (uint8_t)GET(1);
        int uf = (int)GET(1);
        u->occ = uf & 1; u->rent = uf >> 1 & 3; u->dirty = uf >> 3 & 1; u->roach = uf >> 4 & 1;
        u->eval = (uint8_t)GET(1); u->cust_y = (uint8_t)GET(1); u->dirty_days = (uint8_t)GET(1); u->build = (uint16_t)GET(2);
        if (u->t >= T_COUNT || u->f >= NF || u->x + u->w > W) { tw_new(1); return false; }
        u->inside = u->t == T_FLAT && u->occ ? 3 : 0;
        list_add(i);
    }
    S->n_units = (uint16_t)nu;
    for (int i = 0; i < ns; i++) {
        shaft_t *s = &S->s[i];
        memset(s, 0, sizeof *s);
        s->t = (uint8_t)GET(1); s->x = (uint8_t)GET(1); s->f0 = (uint8_t)GET(1); s->f1 = (uint8_t)GET(1); s->ncars = (uint8_t)GET(1); s->build = (uint16_t)GET(2);
        s->w = g_T[s->t].w;
        for (int q = 0; q < MAX_QF; q++) s->qh[q] = s->qt[q] = NO_ID;
        for (int k = 0; k < s->ncars && k < MAX_CARS; k++) car_init(&s->car[k], s->f0);
    }
    S->n_shafts = (uint8_t)ns;
#undef GET
    tw_compute_access();
    tw_population();
    return true;
}
void tw_save_meta(const uint8_t *b, int n, uint32_t *day, uint8_t *star, int64_t *funds)
{
    if (day) *day = 0;
    if (star) *star = 0;
    if (funds) *funds = 0;
    if (!b || n < 18) return;
    uint32_t mag = (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
    if (mag != 0x34525754u && mag != 0x33525754u && mag != 0x32525754u) return;
    if (day) *day = (uint32_t)b[4] | (uint32_t)b[5] << 8 | (uint32_t)b[6] << 16 | (uint32_t)b[7] << 24;
    if (funds) {
        uint64_t f = 0;
        for (int i = 0; i < 8; i++) f |= (uint64_t)b[8 + i] << (8 * i);
        *funds = (int64_t)f;
    }
    if (star) *star = b[16];
}

static uint64_t get_le(const uint8_t *p, int sz)
{
    uint64_t v = 0;
    for (int i = sz - 1; i >= 0; i--) v = v << 8 | p[i];
    return v;
}
