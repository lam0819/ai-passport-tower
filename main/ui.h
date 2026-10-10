// Controls and menus (platform-free; the host harness runs this file too).
// Three keys, one at a time: UP / DOWN tap = step, hold = repeat;
// OK tap = select, OK hold 0.5 s = back (in the tower view: pause / run).
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum { M_TITLE, M_PLAY, M_MENU, M_PLACE, M_INSPECT, M_STATUS, M_NEWS, M_END };
enum { KEY_UP = 1, KEY_DOWN, KEY_OK, KEY_BACK };
#define MAX_ITEMS 64
typedef struct { char label[34]; char sub[10]; uint8_t dis, act; int16_t arg, arg2; } item_t;
typedef struct {
    uint8_t mode;
    uint8_t end_kind, end_sel;       // ending screen: 1 = TOWER (asks about the Underground challenge), 2 = CORE; 0 = YES / 1 = KEEP BUILDING
    int cur, cam, x;
    struct { int t, v, stage, x, f0, top; } place;
    struct { int i, n; int16_t list[24]; } insp;     // >= 0 room id, < 0 -(shaft+1)
    struct { char title[40]; item_t it[MAX_ITEMS]; uint8_t n, sel, back; int16_t ctx; } menu;
    uint32_t frame;
    uint8_t held; int hold_n; bool ok_long;
    int save_in;                                      // frames until the debounced autosave
    uint8_t sound;                                    // 0 music + fx, 1 fx only, 2 off (saved in NVS)
    bool (*load_cb)(void);                            // load the last-used slot (legacy / title fallback)
    void (*save_cb)(void);                            // autosave to the last-used slot
    bool (*load_slot_cb)(int slot);                   // 0 or 1
    void (*save_slot_cb)(int slot);
    void (*slot_info_cb)(int slot, char *out, int n); // "EMPTY" or "DAY n *s $xxx"
    int  (*last_slot_cb)(void);
    void (*sfx_cb)(int what);                         // 0 select, 1 build, 2 bad, 3 fanfare
    void (*sound_cb)(int mode);                       // apply + persist a new sound mode
} ui_t;
extern ui_t g_ui;

void ui_init(void);
void ui_keys(bool up, bool down, bool ok);   // once per frame: held state of the three keys
void ui_act(int key);                        // one logical key (tests use this directly)
int ui_frame(void);                          // runs this frame's sim ticks; returns how many
void ui_follow(void);
// Background music wanted now: 0 none, 1 day loop, 2 night loop.
int ui_music(void);
int ui_place_top(void);                       // top floor for a variable-height placement (car park), else -1
