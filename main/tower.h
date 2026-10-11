// 摩天大廈 SKYRISE: the tower simulation. Pure C, no ESP-IDF, no malloc:
// every pool is a fixed static array (see PLAN.md 5.2). The host harness
// (tools/host) runs this same file.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef enum {
    T_LOBBY, T_FLOOR, T_STAIRS, T_LIFT, T_OFFICE, T_FLAT, T_CCT, T_SINGLE, T_HOUSEK, T_SECURITY,
    T_TWIN, T_SUITE, T_SHOP, T_DIMSUM, T_CINEMA, T_CLINIC, T_RECYCLE, T_PARKING, T_EXP, T_METRO, T_CHAPEL,
    T_REFUGE, T_MEGA, T_COASTER,     // appended: save files keep their type ids
    T_CARPARK, T_POOL, T_ICE, T_ATRIUM, T_BANQUET, T_CLIMB, T_GARDEN, T_STUDIO,
    T_PUMP, T_VENT, T_MHUB, T_SPA, T_FOSSIL, T_GEM, T_NMARKET, T_AQUA, T_LAB, T_GEO, T_VAULT,   // underground challenge
    T_COUNT, T_NONE = 0xff,
} ttype_t;

enum { F_TENANT = 1, F_FOOD = 2, F_COMMERCE = 4, F_HOTEL = 8, F_QUIET = 16, F_SHAFT = 32, F_FILL = 64 };
enum { WH_ANY, WH_LOBBY, WH_BASE, WH_METRO, WH_TOP, WH_PARK, WH_HIGH };   // WH_PARK: B4..G, WH_HIGH: 60/F and up

typedef struct {
    const char *zh, *en;
    uint8_t w, h, star, flags, where, open0, open1, guests;
    int32_t cost, car, maint, night, per_cust, base;
    uint16_t build_min;              // construction time in game minutes
    uint16_t col;                    // natural RGB565
    uint8_t dmin;                    // underground facility: needs this basement depth (B<dmin>) or deeper; 0 = normal
} tdef_t;
extern const tdef_t g_T[T_COUNT];
extern const uint8_t g_build_order[T_COUNT];

#define NO_ID 0xffff
#define MAX_H 5                      // tallest room (coaster)
#define REFUGE_GAP 20                // every floor above 20/F needs a refuge floor at most 20 floors below it

typedef struct {
    uint8_t t, f, x, w, v;           // type, bottom floor, segment, width, shop variant (height comes from the type)
    uint8_t occ : 1, rent : 2, dirty : 1, roach : 1, booked : 1, lunched : 1, away2 : 1;
    uint8_t eval, inside, transit, stay, dirty_days, low_days, fire_h, last_wait, cust_y;
    uint16_t cust, next;             // next: same-floor list
    uint16_t s_sum, s_n;             // people-minutes waited / people, since the last evaluation
    uint16_t build;                  // construction minutes left (total comes from the type)
} unit_t;                            // 26 B

typedef struct {
    uint8_t from, to, n, kind, leg, nlegs, st;
    uint8_t ls[2], lf[2], lt[2];     // legs: shaft, from, to
    uint16_t u, dest, wait, walk, next;
    uint8_t home;
} group_t;

typedef struct {                     // sim + drawing state of one car
    uint8_t f, load, door; int8_t dir;
    uint16_t head;                   // riders
    int32_t disp;                    // shown floor, Q8 (200 floors do not fit in 16 bits)
    uint8_t door_a, hold, hold_f, pin, pout, dwell; int16_t stop_f;
    int8_t anim_t[6]; uint8_t anim_out[6], n_anim;
} car_t;

typedef struct {
    uint8_t t, x, w, f0, f1, ncars;
    uint16_t build;                  // minutes left
    car_t car[MAX_CARS];
    uint16_t qh[MAX_QF], qt[MAX_QF]; // FIFO per served floor
} shaft_t;

typedef struct { uint16_t at; uint8_t from, to, n, kind; uint16_t u; } pend_t;

typedef struct {
    int64_t spawned, arrived, failed, overflow, wait_sum, wait_n;
    int32_t max_wait, peak_groups, rent_q, grants, rushed, speed_buys, fires_big;
    int64_t income, spend, q_in, q_out;
} tstats_t;

typedef struct {
    // tower
    unit_t u[MAX_UNITS]; uint16_t n_units;          // slots; t == T_NONE = free
    uint16_t floor_head[NF];
    uint64_t slab[NF];
    shaft_t s[MAX_SHAFTS]; uint8_t n_shafts;
    group_t g[MAX_GROUPS]; uint16_t g_free, n_groups;
    pend_t p[MAX_PEND]; uint16_t n_pend;
    uint8_t access[NF];
    // time and money
    uint32_t tick; uint32_t day;
    int64_t funds; int32_t pop; uint8_t star, speed, owned[4], rush;
    // events
    bool vip_ok, wedding, wed_sent, tower_done, typhoon;
    uint16_t vip_room; uint8_t vip_state; uint32_t vip_wait;   // 0 none, 1 booked, 2 sent, 3 arrived, 4 night done, 5 left
    uint16_t wed_arrived; uint16_t fire_u; int32_t grant_day;
    uint8_t celeb;
    uint8_t celebrate;               // 1 = TOWER rating reached, 2 = CORE rating: the UI shows the ending once
    uint8_t deep, core_done;         // Underground challenge accepted / CORE rating reached
    uint8_t dug;                     // deepest excavated floor index (BASE-4 = B4 at start, 0 = B100)
    uint16_t dig_h;                  // hours left on the floor being excavated (dug-1)
    uint8_t flood_f, flood_h;        // flooded basement floor and hours left (0xff / 0 = none)
    uint8_t power_h, rain_h, quake_h, alarm_h;  // hours left: power cut / black rain / quake / false alarm
    uint8_t broken_si, broken_h;     // shaft index out of order (0xff = none) and hours until free repair
    uint8_t fest;                    // 0 none, 1 Christmas, 2 Mid-Autumn (decoration day)
    uint8_t santa, complain;         // Santa visit today / VIP complaint pending (one-shot flags)
    uint8_t typhoon_sig;             // 0 none, 1 / 3 / 8 (HK signals); typhoon bool kept for old checks
    uint32_t rng;
    tstats_t st;
    char msg[48]; uint16_t msg_t; char log[12][48]; uint8_t n_log;
    uint8_t last_t, last_v;
} tower_t;

extern tower_t g_tw;
#define TW (&g_tw)

void tw_new(uint32_t seed);
const char *tw_can_place(int t, int f, int x, int top);
const char *tw_place(int t, int f, int x, int top, int v);   // v: shop variant or -1
const char *tw_demolish(int uid);
const char *tw_demolish_shaft(int si);
const char *tw_add_car(int si);
const char *tw_finish_now(int uid, int si);  // uid or si (-1 for none)
const char *tw_buy_speed(int i);
int32_t tw_cost(int t, int f, int f1);
int32_t tw_finish_price(int uid, int si);
void tw_tick(void);
void tw_animate(void);           // once per drawn frame (lift drawing state)
int tw_cell(int f, int x);       // 0 empty, uid+1, -(si+1)
bool tw_route_ok(int a, int b);
void tw_compute_access(void);
void tw_population(void);
int tw_star_reqs(int n, const char *names[8], bool ok[8]);
void tw_note(const char *s);
int tw_waiting(int si, int f, int *max_wait, bool *up, bool *down);
bool tw_serves(const shaft_t *s, int f);
bool tw_is_open(const unit_t *u);
bool tw_night(void);
int tw_hour_x10(void);
const char *tw_floor_name(int f, char buf[8]);
bool tw_is_lobby_floor(int f);
bool tw_is_refuge_floor(int f);
int tw_uh(const unit_t *u);       // room height in floors (multi-storey car park: 2-4, kept in v)   // a built refuge floor
int tw_uncovered_floor(void);
int32_t tw_dig_cost(void);         // price of excavating the next basement floor (rock layers cost more)
int tw_dig_hours(void);
const char *tw_dig(void);          // start digging the next floor, or rush the one in progress
int tw_deep_uncovered(int kind);   // 0: first occupied basement below B10 with no pump within 10 floors, 1: below B20 with no vent within 15; -1 = none
int tw_depth(int f);               // basement level (1 = B1), 0 above ground
int tw_find_spot(int t, int f, int from);
void tw_new_day(void);
int32_t tw_repair_power_cost(void);
int32_t tw_repair_lift_cost(void);
const char *tw_repair_power(void); // pay to restore lifts after a power cut
const char *tw_repair_lift(void);  // pay to fix the broken shaft
bool tw_shaft_live(int si);        // false while power is out or that shaft is broken
void tw_save_meta(const uint8_t *buf, int n, uint32_t *day, uint8_t *star, int64_t *funds);  // peek a blob
// save: packs to a byte blob; returns size. load returns false if the blob is bad.
#define SAVE_MAX (48 + MAX_UNITS * 11 + MAX_SHAFTS * 7 + NF * 8)
int tw_save(uint8_t *buf);
bool tw_load(const uint8_t *buf, int n);
uint32_t tw_rand(void);
