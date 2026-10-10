// All tunables in one place, one for one with C in prototype/index.html.
#pragma once
#include <stdint.h>

// --- screen
#define SCREEN_W 320
#define SCREEN_H 240
#define BAND_ROWS 24                 // one floor row; 2 * 320*24*2 = 30.7 KB of band buffers
#define FPS 30
#define FRAME_US (1000000 / FPS)

// --- layout (pixels)
#define TW_TX 24                     // tower left edge
#define TW_TY 14                     // first floor row
#define TW_FH 24                     // floor row height
#define TW_SEG 4                     // pixels per segment
#define TW_VIS 8                     // floors on screen
#define HINT_Y 212                   // message / hint bar (28 rows)
#define MAP_X 290
#define MAP_Y 15

// --- tower
#define W 64                         // segments per floor
#define NF 200                       // B100..B1, G, 1/F..99/F (B5..B100 open with the Underground challenge)
#define BASE 100                     // G
#define N_SKY 13                     // lobby floors: B90..B15 (deep), G, 15/F..90/F
#define MAX_UNITS 768
#define MAX_SHAFTS 24
#define MAX_CARS 4
#define MAX_GROUPS 512
#define MAX_PEND 256
#define MAX_QF 34                    // queue heads per shaft (served floors)
#define LIFT_SPAN 30
#define CAP_LIFT 15
#define CAP_EXP 25
#define EXP_SPEED 3
#define DOOR_TICKS 2
#define STAIR_MAX 4
#define WALK_PER_FLOOR 6

// --- time: 1 tick = 5 game seconds
#define T_MIN 12
#define T_HOUR 720
#define T_DAY 17280
#define NIGHT_TPF 48

// --- economy
#define START_FUNDS 1000000
#define GRANT_BELOW 50000
#define GRANT 200000
#define RUSH_PCT 25
#define HOUSEKEEP_RANGE 6
#define HOUSEKEEP_ROOMS 24
#define SECURITY_RANGE 10
#define OFFICE_POP 6
#define FLAT_POP 3
#define FLAT_SALE 150000
#define OFFICE_RENT 10000
