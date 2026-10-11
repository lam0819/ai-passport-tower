# 摩天大廈 SKYRISE — AI Passport 第五款遊戲 / 5th AI Passport game

> 原創向 SimTower (1994) 致敬嘅大廈經營模擬。唔用任何 Maxis / OPeNBooK 素材、名或者文字；美術同聲音全部程式生成。
> An original tower-management homage to SimTower (1994). No Maxis / OPeNBooK assets, names or text; all art and sound procedural.
> Name confirmed: **摩天大廈 SKYRISE**.

Status: **design + browser prototype** (`prototype/index.html`, headless tests pass). Firmware not started.

---

## 1. 研究摘要 Research summary (SimTower mechanics we borrow, with sources)

| Topic 主題 | What the original does 原作做法 | Source |
|---|---|---|
| Size 大細 | Max 100 floors above ground, 9–10 below. Standard lifts span ≤30 floors, express lifts the whole height (stop every 15 floors at sky lobbies). | [Wikipedia][wp], [GameFAQs kiwizoid][kz], [manual][man] |
| Stars 星級 | Start ★1 with $2M. ★2 pop 300; ★3 pop 1,000 + security; ★4 pop 5,000 + 2 suites + medical + recycling + favourable VIP; ★5 pop 10,000 + metro; TOWER: cathedral on floor 100 + 15,000 pop + weekend wedding. Each star unlocks facilities. | [Relentless Optimizer][ro], [GameFAQs 2168][gf1], [GameFAQs 6905][gf2] |
| Tenants 租戶 | Office $40K, 6 people, rent paid each quarter; condo $80K, 3 people, one-time $150K sale; fast food / restaurant / shop / cinema earn by customers; hotel single/twin/suite pay per stay. | [Relentless Optimizer][ro], [GameFAQs MentalMan][mm] |
| Time 時間 | Day split into periods (fast at night, slow at rush hour). Quarter = 2 weekdays + 1 weekend; 4 quarters a year. Offices work weekdays, shops peak on weekends. | [manual][man], [kiwizoid][kz] |
| People & stress 人流同壓力 | Sims queue for lifts; stress rises with waiting (black → pink → red). Stress lowers room evaluation; low evaluation → tenants leave. At most one lift change per trip; walk ≤4 flights of stairs. | [SimTower wiki: Evaluation][ev], [GameFAQs 6905][gf2], [Relentless Optimizer][ro] |
| Noise 噪音 | Offices dislike commercial noise; condos/hotels dislike offices and commercial. Keep floor types together. | [wiki: Evaluation][ev], [Relentless Optimizer][ro] |
| Hotel 酒店 | Rooms get dirty after checkout; housekeeping cleans them (needs service lift); dirty >3 days → cockroaches that spread. | [GameFAQs 6905][gf2], [kiwizoid][kz] |
| Population 人口 | Offices/condos static; hotel goes up and down; shops count yesterday's customers. | [Relentless Optimizer][ro] |
| Events 事件 | VIP visit (needed for ★4), fire (security fights it, or pay for helicopter), bomb ransom, buried treasure when digging, Santa at year end, wedding at cathedral = TOWER. | [kiwizoid][kz], [Wikipedia][wp], [manual][man] |

[wp]: https://en.wikipedia.org/wiki/SimTower
[ro]: https://relentlessoptimizer.com/gaming/2021/03/13/simtower-reference/
[gf1]: https://gamefaqs.gamespot.com/mac/564236-simtower/faqs/2168
[gf2]: https://gamefaqs.gamespot.com/pc/565191-simtower/faqs/6905
[kz]: https://gamefaqs.gamespot.com/pc/565191-simtower/faqs/28905
[mm]: https://gamefaqs.gamespot.com/pc/565191-simtower/faqs/7772
[ev]: https://simtower.fandom.com/wiki/Evaluation
[man]: https://archive.org/stream/SimTower_-_Manual_-_PC/SimTower_-_Manual_-_PC_djvu.txt

**What we change 我哋改咗咩:** the full **100 floors** (G–99/F) + 4 basements, but a narrower tower (64 segments wide, 4 px each, so a floor fits the 320×240 screen), population targets scaled down to match the width, people simulated as **groups** rather than individuals, no terrorism/bomb event (kid-friendly), and HK flavour throughout.

---

## 2. 遊戲設計 Game design

### 2.1 目標 Goal
起一座香港式綜合大廈：寫字樓、住宅、酒店、茶餐廳、酒樓、商舖、戲院、地鐵站，一路升星，最後喺天台起「天空禮堂」，週末辦一場天空婚禮就攞到 **TOWER** 最高評級。
Build an HK mixed-use high-rise and climb the stars. The final goal is the rooftop **Sky Chapel** and a weekend sky wedding, which gives the **TOWER** rating.

### 2.2 設施 Facilities (prototype numbers)

| ★ | 設施 Facility | w×h | Cost | Earns / does |
|---|---|---|---|---|
| 1 | 大堂 Lobby (G + sky lobby every 15 floors: 15/30/45/60/75/90/F) | 8 | $5K | entrance; lift transfer floors |
| 1 | 空層 Empty floor (auto-fills ≤16 segs per OK) | 1+ | $500/seg | structure; other rooms, stairs and lifts can be built over it |
| 1 | 樓梯 Stairs | 8 | $5K | links floor f↔f+1; people walk ≤4 flights |
| 1 | 升降機 Lift (≤30 floors, ≤4 cars, 15 people) | 4 | $200K + $80K/car | upkeep $10K/car/quarter |
| 1 | 寫字樓 Office | 9 | $40K | 6 workers; rent $6K / $10K / $15K per quarter (low/normal/high) |
| 1 | 住宅 Flat | 16 | $80K | 3 residents; sold once for $120K / $150K / $180K; resold if it empties |
| 1 | 茶餐廳 Cha chaan teng | 16 | $100K | $100 per customer (walk-ins, office lunch, residents) |
| 2 | 單人房 Hotel single / 房務部 Housekeeping / 保安室 Security | 4 / 15 / 16 | $20K / $50K / $100K | $1.5K per night; cleans 24 rooms/day within ±6 floors; puts out fires within ±10 floors |
| 3 | 雙人房 Twin / 套房 Suite | 6 / 10 | $50K / $100K | $3K / $6K per night |
| 3 | 商舖 Shop / 酒樓 Dim sum hall / 戲院 Cinema (2 floors) | 12 / 24 / 31 | $100K / $200K / $500K | $150 / $200 / $120 per customer; cinema shows at 13:00 and 19:00 |
| 3 | 診所 Clinic / 回收站 Recycling (2 floors) / 停車場 Car park (basement) | 26 / 25 / 16 | $500K / $500K / $50K | needed for ★4; flats far from a clinic lose evaluation |
| 3 | 高速升降機 Express lift (any height, 25 people, ×3 speed, stops at lobbies + ends) | 6 | $400K + $150K/car | |
| 4 | 地鐵站 Metro station (B4–B2) | 30×3 | $1M | ★5; +30% shop visitors, some people arrive by metro |
| 5 | 天空禮堂 Sky Chapel (roof, 96–99/F) | 28×4 | $3M | wedding → TOWER |

### 2.3 星級 Stars (scaled for 64 segments wide, 100 floors)

| ★ | 條件 Requirement | Unlocks |
|---|---|---|
| 2 | Pop 200 | hotel single, housekeeping, security |
| 3 | Pop 600 + security | twin, suite, shop, dim sum, cinema, clinic, recycling, car park, express lift |
| 4 | Pop 2,000 + 2 suites + clinic + recycling + car park + **VIP happy** | metro |
| 5 | Pop 3,500 + metro | sky chapel |
| TOWER | Pop 5,000 + sky chapel + weekend **sky wedding** (30 guests reach the roof) | — |

Status screen shows a checklist of the next star (`[x] POP 600`, `[ ] SECURITY`).

### 2.4 時間 Time
1 tick = 5 game s. Day = 17,280 ticks. Speeds: pause, 1× (a day ≈ 2.4 min); **2× and 4× clocks are bought once in the SPEED SHOP with in-game money only** (2× $300K from ★1; **4× $1M, unlocks at ★3**; no real money, no ads). 00:00–06:00 always runs fast. Calendar like the original: **WD1, WD2, WE** = one quarter; 4 quarters = 1 year.
- 每日 00:00：evaluate rooms, shop income, tenants move in, hotel bookings, star check, events, autosave.
- 每季 Quarter start: office rent, unhappy tenants move out, upkeep, grant check, Q1 = Chinese New Year 利是 bonus.

### 2.5 人流 People (aggregated, fixed pools)
- **No per-person simulation.** Each room has counters (`inside`, `transit`) and decides once per game minute whether a **group** of 1–6 people sets off (office: arrive 7:30–9:00, lunch 12:00 to the nearest open food place, leave 17:00–18:30; flats: out in the morning, back in the evening, weekend shopping; hotel: check in 17–21, check out 7–10; shops and cinema: walk-ins from G or the metro).
- Group pool **512**, delayed-return pool **256**. When the pool is full, the trip "overflows": the people arrive instantly with no stress, so the game never blocks. Tests show peak 147 groups in a deliberately overloaded tower.
- **Routes 路線:** same floor → walk; ≤4 floors with stairs → walk; a lift serving both floors (the shortest queue wins); otherwise **one transfer at a lobby floor** (G/15/30/…/90); or 1–2 flights of stairs to reach a lift. No route → the trip fails and counts as an hour of waiting (the room's evaluation crashes, which is the "no access" feedback).
- **Lifts 升降機:** collective control (finish the current direction, serve calls on the way, reverse when nothing is ahead). Capacity 15/25, doors 2 ticks, express moves 3 floors/tick. Each floor of each shaft has a FIFO queue of groups; groups waiting get +1 wait per tick. A shaft can be **demolished** (inspect it → `DEMOLISH`): every rider/waiter is released (their trip counts as failed), the whole column turns back into empty floor, and it costs 10% of the shaft like rooms.
- **Stress → evaluation 壓力 → 評價:** each room keeps total waited time; daily `eval = ½·old + ½·(78 − min(70, avgWaitMin×10) + rent adj (+10 / 0 / −15) − noise 15 − no-housekeeping 25 − no-clinic 10)`. Weekends keep the last weekday's wait. Shown as an orange/red dot on the room, plus queue figures next to lifts (black → pink → red, like the original).
- Offices with eval < 35 leave at the quarter; flats < 25 go back on sale; shops with < 8 customers for 4 days close.

### 2.6 事件 Events (original, HK flavour)
| Event | Effect |
|---|---|
| 名人到訪 VIP | When everything else for ★4 is ready: stays in your best suite; happy if total lift wait < 4 min and suite eval ≥ 55 → ★4 |
| 八號風球 Typhoon Signal 8 | Q3 weekdays sometimes: offices and shops closed for the day, rain on screen |
| 小火警 Small fire | Rare; security within 10 floors puts it out in 2 h (eval −30), otherwise the room is gutted after 8 h (tenant leaves) |
| 新年利是 CNY lai see | Q1: +$20K × stars |
| 天空婚禮 Sky wedding | ★5 + chapel + pop: next weekend 11:00, 30 guests ride to the roof → TOWER |
| (planned) 中秋燈籠 / 聖誕燈飾 | Cosmetic lanterns / lights in Q3 / Q4 weekends |

No bomb or terrorism events (kid-friendly). The original's "buried treasure" may come back as a basement-dig bonus (open question).

### 2.7 唔會卡死嘅經濟 Never-stuck economy
- Start $1M. Flats are the early cash engine (one-time sale ≈ 1.9× cost); offices give steady quarterly rent; food gives daily cash.
- **Small business grant 中小企資助:** at a quarter start, if funds < $50K → +$200K (and any debt cleared). Tested.
- Building is refused if you can't afford it; demolish costs 10%.
- **Construction time 施工時間:** rooms show scaffolding + a progress bar (office 3 h, flat 4 h, lift 6 h, express 12 h, cinema/clinic/recycling 12 h, metro 24 h, chapel 48 h game time; lobby/empty floor instant). **RUSH BUILD** (menu toggle) = +25% price, ready at once; **FINISH NOW** (inspect a site) = pay 25% × work left. In-game money only.
- **Speed shop 加速:** 2× clock $300K, 4× clock $1M (needs ★3), one-time, in-game money only. Nothing in the game is sold for real money. Upkeep can push funds negative, but the grant catches it.
- The bot in the tests (plain greedy builder, 1 lift) reaches ★2 on day 10 and ★3 on day 28 with **no grant needed**. The grant is a safety net, not a crutch.

---

## 3. 操作 Controls (3 keys, read one at a time)

Keys: **UP / OK / DOWN** on one ADC ladder, so only one key counts at a time (no chords). OK uses tap vs hold.

| Screen | UP | DOWN | OK tap | OK hold (0.5 s) |
|---|---|---|---|---|
| Title | — | — | Continue (or start) | New tower |
| Tower view | floor up | floor down (hold = repeat, accelerates) | **Menu** | Pause / run |
| Menu (list) | prev | next | choose | back |
| Build: place room | slide ◀ | slide ▶ | **build here** (stays in build mode, jumps to the next free spot) | done |
| Build: lift, step 2 | top floor up | top floor down | build lift | back to step 1 |
| Inspect floor | prev room | next room | room menu (FINISH NOW, rent L/N/H, DEMOLISH) | back |

Inspecting a **lift / express shaft** (not a room) opens the shaft menu: `FINISH NOW` (if scaffolding), `ADD CAR` (if affordable / < 4 cars), `DEMOLISH` → `YES, DEMOLISH`. Housekeeping and security are permanent and refuse with `CANNOT REMOVE`.
| Status / News | — | — | close | close |

Main menu: `BUILD AGAIN: <last>` (one OK to continue building), `BUILD...`, `INSPECT n/F`, `SPEED` (cycles owned clocks), `SPEED SHOP...`, `RUSH BUILD: ON/OFF`, `TOWER STATUS` (star checklist + money), `NEWS`, `GO TO G/F`, `GO TO TOP`, `SAVE & TITLE`.
Why not a radial menu: with 3 keys a radial ring is just a list with extra steps. A short vertical list with the last-used action on top costs the fewest key presses (`OK, OK` = build another of the same).
Placement helpers: rooms snap to the nearest valid spot; a red ghost and a reason (`NO FLOOR BELOW`, `NEEDS 3 STARS`, `NOT ENOUGH $`) show on the hint bar; "empty floor" auto-fills up to 16 segments; rooms, stairs and lifts can be placed over empty floor. Optional setting: swap UP/DOWN.

---

## 4. 畫面 Screen layout (320×240)

```
y 0-13    HUD: $ funds | POP | ★★★★☆ | WD2 Q2 Y1 08:36 | speed
y 14-205  8 floor rows × 24 px. x 0-21 floor labels (cursor floor yellow),
          x 24-279 tower: 64 segments × 4 px. Sky colour follows the clock.
x 288-319 minimap: 104 floors scaled into 192 px (B4..99/F), yellow frame = visible window
y 212-239 hint / message bar (2 lines: message or key help + context)
```
Floor rows went 12 → 16 → 20 → **24 px** (user decisions; **8 floors visible**) for more detailed shop and lift art: 13 px storefronts with tiled floors, taller lift cabins with riders, ceiling beams in rooms. One floor row = one LCD band (320 × 24).

### 4.1 港式舖頭 HK shop fronts: recognisable by look, no Chinese on signs (user decision)
Each 24 px shop row: signboard (8 px) with a **pixel icon + one tiny English word** → striped awning (2 px) → storefront (13 px, art + tiled floor) with counter + shopkeeper + customers, and a protruding vertical lightbox (colour dots, scanning light at night). The player picks the type in the build list (`SHOP: PHARMACY` etc.).

| Type | Sign (icon + word) | Storefront art |
|---|---|---|
| Convenience | blue/orange, fridge icon, `24H` | a row of **glowing drinks fridges** with coloured bottles |
| Pharmacy | green, white cross, `PHARM` | a **big green cross** panel + white shelves of boxes |
| Goldsmith | red/gold, ingot, `GOLD` | dark glass case of **gold that glitters** (moving sparkles) |
| Bakery | brown, bun, `BREAD` | **loaves and buns** + a pink **layer cake** |
| Electronics | dark/cyan, TV, `TV` | a **TV wall** (2 × N screens changing picture) |
| Fashion | magenta, dress, `STYLE` | **mannequins** in coloured dresses |

Cha chaan teng `CAFE` (cup icon), dim sum `DIM SUM` (steamer icon, red lanterns), `CINEMA` marquee. Closed = roller shutter; vacant = `TO LET` (flats `SALE`). Other rooms are labelled in tiny English (`GUARD`, `CLINIC`, `RECYCLE`, `METRO`, `CHAPEL`).
- **Day/night:** from dusk the tower darkens; neon glows on signs, lit interiors, lit windows, lift cabins.
- Chinese stays only in menus/title: 45 glyphs × 12×12 = **1.2 KB** (was 67 glyphs at 12 + 10 px = 3.1 KB; the 10 px signboard set is gone).

### 4.2 升降機動畫 Lift animation
- **Smooth motion:** the sim moves cars a whole floor per tick; the drawing keeps a separate `disp` position (Q8) that glides toward the sim floor (≥0.1 floor/frame, eased 22 %), so cars visibly travel along the shaft instead of jumping. If a new stop arrives while the doors are open, the doors close first, then the car glides (fixes a door/glide deadlock found by the firmware harness).
- **Doors close and go:** after two loads in a row at one floor, a car with riders aboard departs (otherwise a steady trickle at G kept it loading forever).
- **Doors:** at each stop the car holds ~16 frames: two door panels slide open (4 frames), people walk out/in, the doors slide shut.
- **People boarding/alighting:** the sim reports `pin/pout` per stop; the drawing spawns ≤3 walking figures each way (6-figure cap per car).
- **Floor indicator:** a black LED strip on top of every car shows ▴/▾ + the floor (`▴23`). **Hall lanterns** (up/down triangles) light on each floor where people are waiting; the queue shows little people, black → pink → red with waiting time (+N when more than 5).
- Under construction: shafts show scaffolding; cars don't run.

### 4.3 30 fps budget for the visuals
- Everything is drawn from rectangles + ~70 glyphs. No sprite sheets, so there's no flash/RAM cost beyond code.
- Lift animation adds per car: `disp` (Q8.8 int16), `doorA` (uint8), `hold`, `holdF`, `pin/pout`, 6 × figure (2 B) ≈ **20 B/car**.
- Dirty bands: a moving car dirties only its **shaft column (24 px wide) on the 2 rows it overlaps**; door/people animation dirties that one row's shaft ±20 px; neon pulse/lightbox scan/cinema bulbs dirty only their own small rects every 4th frame; the dusk/dawn switch is a full redraw a few times per day. Typical frame at 1×: 2–4 cars moving + 1–3 neon rects ≈ 8–12 small rects ≈ 3–5 ms of SPI at 40 MHz.

## 5. 韌體架構 Firmware plan (ESP32-C3, ESP-IDF v6, `ota_1`)

Same pattern as heist / firefighter / coin carnival: app in **`ota_1`**, launched by the AI Passport launcher; NVS namespace **`tower`**; no PSRAM; no malloc after boot; integers only (no FPU needed: all sim maths is integer already).

### 5.1 資料結構 Data (static pools; as built in `main/tower.h`)
```c
typedef struct { uint8_t t, f, x, w, v;          // type, floor, segment, width, shop type (height from type)
                 uint8_t occ:1, rent:2, dirty:1, roach:1, booked:1, lunched:1, away2:1;
                 uint8_t eval, inside, transit, stay, dirty_days, low_days, fire_h, last_wait, cust_y;
                 uint16_t cust, next;            // next = per-floor room list
                 uint16_t s_sum, s_n, build; } unit_t;                       // 26 B
typedef struct { uint8_t from, to, n, kind, leg, nlegs, st, ls[2], lf[2], lt[2];
                 uint16_t u, dest, wait, walk, next; uint8_t home; } group_t; // 26 B (intrusive list)
typedef struct { uint8_t f, load, door; int8_t dir; uint16_t head; int16_t disp;
                 uint8_t door_a, hold, hold_f, pin, pout, dwell; int16_t stop_f;
                 int8_t anim_t[6]; uint8_t anim_out[6], n_anim; } car_t;     // sim + animation
typedef struct { uint8_t t, x, w, f0, f1, ncars; uint16_t build; car_t car[4];
                 uint16_t qh[34], qt[34]; } shaft_t;   // FIFO per served floor, 264 B
```

### 5.2 記憶體 RAM at 100 floors: **measured** (sizeof on the host + `idf.py size`)

| Item | Bytes |
|---|---|
| rooms 768 × 26 B | 19,968 |
| groups (people) 512 × 26 B (peak in tests: 150 at an overloaded rush hour) | 13,312 |
| shafts 24 × 264 B (4 cars with animation state + queue FIFOs) | 6,336 |
| pending returns 256 × 8 B | 2,048 |
| per-floor list heads, slab bitset (104 × 8 B), access, stats, news log | ~2,000 |
| **`tower_t` total** | **43,616** |
| LCD band buffers 2 × (320 × 24 × 2), DMA double buffered | 30,720 |
| save blob buffer (static, no malloc) | 9,496 |
| UI (menus) + dirty-region signatures | ~2,900 |
| **Game total** | **≈ 86.7 KB** |
| `idf.py size`: .bss 92,928 + .data 7,118 (incl. ESP-IDF) | DRAM used 47.6 %, **168 KB left** |

Higher than the 55 KB estimate because rooms carry their runtime counters (26 B, not 16 B), car animation state lives in the shaft, rows are 24 px (bands 30.7 KB) and the save buffer is static. Still well inside the C3 (no PSRAM): 168 KB of DRAM free. Easy trims if ever needed: groups 512 → 384 (−3.3 KB), save blob written in chunks (−9 KB).
Flash: app image **463 KB** (11.6 % of the 3.9 MB `ota_1` slot). CJK: 45 glyphs × 12×12 = 1.2 KB (menus/title only).

### 5.3 30 fps 同局部重畫 Partial redraw (as built: `main/paint.c`)
- Every frame each region gets a 32-bit **signature** of what it shows: the 10 floor rows (rooms, cursor, ghost, neon phase), each **shaft window** per row (car position, doors, riders, queue, walkers; the shaft column ±32 px), HUD + hint bar, minimap, overlays. Only regions whose signature changed are composed into the 320 × 20 band buffer and sent; with a menu open the paused world is drawn once.
- Neon pulse, cinema bulbs and flames tick every 8 frames, and only in rows that have them.
- **Measured** in the host harness (14-floor tower, 3 cars, shops, morning rush at 1×, 900 frames): **avg ~13 % of the screen sent per frame (≈ 3.9 ms of SPI at 40 MHz), worst ≈ 15.1 ms** (a scroll is a full 76.8 K px ≈ 31 ms, rare). Frame budget 33.3 ms.
- On the device the loop logs every 10 s: fps, late frames, sim µs, paint µs avg/max, px/frame, group pool, free heap.

### 5.4 模擬成本 Sim cost
Prototype: **0.3 µs per tick** averaged over whole days (incl. rush hours) in desktop Chrome, ~4 ms per game day. **Firmware C on the host: 0.13 µs per tick** over 40 bot days (483,840 ticks). On the C3 (~30–50× slower than desktop JS JIT for this kind of code) that is ~10–15 µs/tick, i.e. **≤1 ms per frame at 4× (16 ticks)** and ~1.5 ms in the fast night (48 ticks). The heavy parts are routes (computed on spawn: ≤16 shafts × 4 lobby floors) and the per-minute room loop (≤768 rooms every 12 ticks). Both are bounded.

### 5.5 存檔 Save (NVS `tower`)
- One blob `save` in namespace `tower`: header (magic `TWR2`, day, funds, star, flags, owned clocks, rush, speed, last build + shop type, rng) + slab bitset 832 B + rooms 11 B each (t, f, x, w, shop type, flags, eval, custY, dirtyDays, build minutes) + shafts 7 B each + checksum. **Worst case 9,496 B; a full 100-floor test tower (768 rooms, 5 shafts) = 9,313 B**, a typical tower 2–4 KB. A corrupt blob is refused (checksum) and a new tower starts.
- NVS space: the shared `nvs` partition is 48 KB (0xC000, the assistant's table). NVS splits blobs > 4 KB into chunks itself; a 9.3 KB save takes ~300 entries of 32 B. The game logs used/free NVS entries at boot so we can confirm on the device.
- People in transit are **not** saved (load = everyone at home/work as at 00:00).
- Autosave: at the start of each quarter (≈ every 7 min at 1×), when you leave via `SAVE & TITLE`, and 30 s after the last build/demolish (debounced). Wear: at most ~9 KB per ~7 min, fine for NVS's wear levelling.
- Reset tower: hold OK on the title screen.

### 5.6 音效 Sound + background music
- Coin Carnival's synth task (16 kHz, 10 ms chunks, ES8311): effects (select, build, error, star fanfare) are mixed on top of the music.
- **Two calm original loops**, chosen by the clock: **DAY** (C major pentatonic, 92 bpm) 06:00–19:00, **NIGHT** (sparse A minor pentatonic, 70 bpm, long notes) 19:00–06:00 and on the title. Soft triangle lead + slow root-fifth triangle bass: a few integer ops per sample, no tables beyond the ~50 notes.
- Menu item `SOUND: MUSIC + FX / FX ONLY / OFF`, saved in NVS key `snd` (namespace `tower`, separate from the tower save, so it is kept across new towers).
- The prototype plays the same notes through WebAudio (scheduled 0.5 s ahead).

---

## 6. 原型 Prototype (`prototype/index.html`)
Single file, no dependencies (built from `prototype/head.html` + `p1..p5.js` by `tools/build_proto.sh`). 320×240 canvas scaled ×2, on-screen ▲ OK ▼ buttons, keyboard ↑/↓/Enter. It implements everything above: 100 floors + 4 basements, building rules, construction time + RUSH/FINISH NOW, speed shop, auto-fill floors, lifts with collective control and full animation, stairs, one-transfer routes, aggregated groups, office/flat/hotel/shop/cinema schedules, evaluation with stress and noise, housekeeping and cockroaches, quarter rent, upkeep, grant, stars ★1–5 + TOWER, VIP, typhoon, fire, CNY, wedding, six HK shop types with day/night neon and shutters, save/load to localStorage key `tower`.

`tools/proto_test.sh` runs `tools/proto_test.js` in headless Chrome. **88 checks, all pass** (adds: 20 px rows, choosing all 6 shop types in the build list, BUILD AGAIN remembers the type, 4× locked until ★3), including key-driven UI flows, people conservation, more cars → shorter waits, overload → evaluation crash → tenants leave, hotel/housekeeping, never-stuck grant, a greedy bot reaching ★2/★3 with no grant, VIP → ★4, metro → ★5, chapel on 96–99/F + wedding → TOWER, save round trip (389 rooms), construction/rush/finish-now pricing, speed shop (cannot buy without in-game money), smooth lift glide, doors open and close at a stop, people boarding/alighting, all six shop types rendered at day/noon/night, and no JS errors.

Screenshots: prototype `docs/proto_*.png`; firmware (host-rendered from the C code) `docs/*.png`, see §8.

## 7. 開放問題 Open questions
Decided: name 摩天大廈 SKYRISE ✔, 100 floors ✔, pace ✔ + in-game speed shop ✔, ~60–70 CJK glyphs ✔, 20 px rows ✔, player picks the shop type ✔, 4× at ★3 ✔, rush +25 % ✔.
1. **Service lift + housekeeping travel:** housekeeping range is ±6 floors (no service lift item). Add the service lift?
2. **Lift settings:** expose express-stop choice per shaft, or nothing?
3. **Events:** keep the small fire? Add basement "treasure" and mid-autumn/Christmas decorations?
4. **Autosave interval:** every quarter + 30 s after edits OK?
5. **Room cap:** 768 rooms (pool) is enough for 100 floors of offices/flats, but not 100 floors of 4-segment hotel singles. OK, or raise to 1024 (+6.5 KB)?
6. **Transfers:** routes allow one change at a sky lobby (as in the original), so floors above ~45 need an express to a sky lobby. Fine, or allow two changes?
7. **Music:** day/night loops are in (§5.6). Want more variety (e.g. a weekend tune, a wedding jingle)?

## 8. 韌體 Firmware (built)
`main/`: `tower.c` (simulation, a one-for-one port of `p1–p3.js`, no malloc, integers only), `ui.c` (keys + menus, `p4.js`), `draw.c` (procedural art + CJK, `p5.js`), `paint.c` (dirty regions), `game.c` (30 fps loop, NVS, stats), plus Coin Carnival's `display / input / boot / sound / gfx`. `tools/gen_cjk.py` makes `cjk.c` (Noto Sans CJK HK, OFL).
- Build: ESP-IDF v6, `idf.py set-target esp32c3 && idf.py build && idf.py size`; flash with `tools/flash.sh -p /dev/cu.usbmodem* --boot` (app only, into `ota_1`).
- Host harness `tools/host/run.sh`: compiles the same C files for the PC, runs **80 checks** (controls, rules, people conservation, lifts, stress, hotel, food, grant, bot ★2/★3, VIP ★4, metro ★5, TOWER, construction/rush/finish now, speed shop with 4× at ★3, lift animation, 100-floor save round trip + corrupt-save refusal, frame cost) and renders screenshots into `docs/`.
- Screens: `docs/shops_six_day.png`, `docs/shops_six_night.png`, `docs/title.png`, `day_shops.png`, `night_shops.png`, `lifts_rush.png`, `construction.png`, `build_menu.png`, `place_shop.png`, `speed_shop.png`, `status.png`, `inspect.png`.

## Multi-storey facilities + refuge floors (Oct 10 2026) / 多層設施 + 防火層

**防火層 REFUGE (★1, $300K, whole floor).** HK rule: *Code of Practice for Fire Safety in Buildings 2011* (Buildings Department), clause **B18**. Every building more than 25 storeys above the lowest ground storey needs refuge floors, at most **25 storeys** apart (**20** for industrial buildings, Use Classification 6). The floor must have no other accommodation, be open-sided for cross-ventilation, and carry fire-service equipment. Domestic or composite buildings of 25–40 storeys may use the main roof instead (B18.5).
Sources: https://www.bd.gov.hk/doc/en/resources/codes-and-references/code-and-design-manuals/fs_code2011.pdf and PNAP APP-153.
Game rule (simplified to ~20 as decided):
- Any occupied floor above 20/F needs a built refuge floor at most 20 floors below it.
- Build REFUGE on an empty floor (empty-floor pieces only). It fills every gap between shafts and stairs. Unsupported holes are skipped.
- Lifts and express shafts may pass through it.
- It is **not allowed on a sky-lobby floor**: B18 forbids other accommodation, and the lobby must stay a lobby.
- No rooms can be built on it. Demolishing any piece turns the whole floor back into empty floor.
- No rent, $20K/quarter upkeep.
- **Star ★3, ★4 and ★5 each require "REFUGE FLOORS"** (no uncovered occupied floor).
- **A fire on an uncovered floor** burns twice as long and **guts every tenant on that floor**, with −50 rating.
- Art: bare concrete, yellow hatched assembly area, red sprinkler main and drops, duct, hose-reel cabinets, green EXIT signs (lit at night), plant room with pumps, open railed sides, emergency green tint at night.

| Facility | Floors | ★ | Cost | Income / effect | Where |
|---|---|---|---|---|---|
| MEGA CINEMA 戲院 | 3 | 4 | $1.5M | shows 14:00 / 18:00 (biggest) / 20:30, $160 per ticket, enter on bottom floor | any |
| COASTER 過山車 | 5 | 4 | $3M | 10:00–23:00, evenings ×1.6, weekends ×2, $250 per ride; **noise**: −15 (−30 for flats/hotels) to rooms within 6 segments on its floors and the floors above and below | any |
| PARKING 停車場 | 2/3/4 (menu picks) | 2 | $150K per floor | $40K × floors per quarter; **offices +8 rating**; counts as ★4 CAR PARK | **B4–G only** |
| POOL 泳池 | 2 | 3 | $600K | 07:00–22:00, $60; flats/hotels within 15 floors +8 | any |
| ICE RINK 溜冰場 | 2 | 3 | $800K | 10:00–22:00, weekends ×2.2, $90 | any |
| ATRIUM 中庭 | 4 | 3 | $1M | shops/food within 4 floors get ×1.3 visitors | any (not across a sky lobby) |
| BANQUET 宴會廳 | 2 | 4 | $1.2M | banquets 18:00–23:00, groups of 6–10 × $300; the sky wedding brings 4 extra banquet parties | any |
| CLIMB 攀石 | 3 | 3 | $400K | 10:00–22:00, $80 | any |
| SKY GARDEN 空中花園 | 2 | 4 | $800K | +10 rating to everything within 8 floors (offsets noise/stress) | **60/F and up** |
| TV STUDIO 電視台 | 3 | 4 | $1.5M | $250K lease per quarter; **celebrity day** about 1 day in 6: all shops and leisure ×1.5 visitors | any |

**Memory.**
- Every facility is one 26-byte `unit_t` spanning its floors. The car park's height (2–4) is kept in the existing variant byte `v`.
- The refuge floor is 1–4 unit pieces per floor.
- `MAX_H` = 5 is now the tallest room. The cell lookup, world draw and dirty-row hash scan 5 floors down (was 4).
- `tower_t` is unchanged at 43,624 B, plus 1 B for the celebrity flag in padding.
- The coaster track is a 256 B static table built once with integer sine (no FPU).
- The menu item cap went 32 → 48 (ui_t +800 B).
- `idf.py size`: .bss 93,992 + .data 7,118, **DRAM 47.9 % used, 167 KB free**. The app is 462,848 B, and the ota_1 slot has room to spare.
- CJK glyphs: 64 × 26 B = 1,664 B of flash (+防火過山泳池溜冰中庭宴會攀石花園電視台).

**Redraw.**
- Coaster and MEGA rows are marked dirty every animation step (s_blink = frame/2, so 15 fps motion) only while open and occupied. The host test confirms only their 5–8 rows change.
- Car park, pool, ice, atrium, banquet, climb, garden and studio animate on the slow 4-frame step.
- Refuge EXIT glow is night-only and slow.
- Everything else stays clean.

## Ending / 結局 (Oct 10 2026)
**Win condition (TOWER rating, ★6):**
- At ★5 with a SKY CHAPEL on the top floors and population ≥ STAR_POP[6], a wedding is booked.
- On the next weekend, 11:00–12:00, 6 groups of 5 guests ride to the chapel. Each BANQUET hall adds 4 parties of 8.
- When 30 guests have arrived, the tower gets the TOWER rating: `tower_done`, star = 6.

**Ending screen (new, `M_END`):**
- Shows once, at the moment the rating is reached.
- The game autosaves and pauses the clock.
- Fireworks: 5 procedural bursts with rockets, 16 sparks each, integer sine and slight gravity. They redraw the whole screen every 4 frames (7.5 fps, about 30 ms of SPI per redraw, inside the frame budget).
- Panel: "TOWER RATING! 摩天大廈 SKYRISE COMPLETE", then day, population and money.
- **OK: KEEP BUILDING** returns to play, where the HUD keeps the TOWER badge.
- RAM: no buffers, one flag byte (`celebrate`, not saved, so loading a finished tower doesn't replay the ending).

## Underground Challenge / 地底挑戰 (Oct 10 2026)
**Start / 開始:** The TOWER ending now asks **CONTINUE CHALLENGE? DIG 100 FLOORS UNDERGROUND**.
- UP/DOWN picks YES / KEEP BUILDING; OK confirms.
- **YES** sets `deep`, moves the cursor to B4 and autosaves.
- **KEEP BUILDING** returns to normal play.
- 塔級結局後問玩家要唔要「向下挖 100 層」。

**Floor range / 樓層:** `NF` 104 → 200 (B100..B1, G, 1/F..99/F), with `BASE` = 100.
- Before the challenge, B5 and below show as "SOLID ROCK" and can't be scrolled to.
- `dug` is the deepest excavated floor (B4 at start). You can scroll one floor below it.
- Lobby floors: B90/B75/…/B15 are deep lobbies, alongside G and the 15/F…90/F sky lobbies (13 in all). An express lift from B90 to G stops at all of them, and local lifts (max 30 floors) link each band.

**Digging / 挖掘:** Main menu → **DIG B<n>**, one floor at a time. Price = (100K + 15K × depth) × layer factor:
- soil B1–30 ×1
- rock B31–60 ×2
- granite B61–85 ×3
- magma B86–100 ×4

Examples: B5 $175K, B35 $1.25M, B65 $3.2M, B100 $6.4M.
- Dig time = (2 + depth/10) h × (factor+1)/2. B5 takes 2 h, B65 16 h, B100 30 h.
- **RUSH DIG** pays +25 % and finishes on the next hour.
- A drill animates across the floor being dug, with a progress fill.

**Deep facilities (unlock by depth, one room record each):**
| | name | size | from | income |
|-|-|-|-|-|
| 泵房 | PUMP ROOM | 6×1 | B1 | covers ±10 floors against flooding |
| 通風 | VENT SHAFT | 4×1 | B1 | covers ±15 floors (air) |
| 地鐵 | METRO HUB | 40×3 | B10 | visitors, trains glide on 2 platforms |
| 溫泉 | HOT SPRING | 28×2 | B20 | steam, bathers |
| 化石館 | FOSSIL MUSEUM | 32×3 | B30 | T-rex skeleton |
| 寶石 | GEM MINE | 20×2 | B40 | glittering gems, mine cart |
| 夜市 | NIGHT MARKET | 40×1 | B50 | 17:00–24:00, awnings, bulb strings |
| 水族館 | AQUARIUM | 36×3 | B60 | fish + whale shark |
| 實驗室 | SECRET LAB | 24×2 | B70 | $400K/quarter lease |
| 地熱 | GEOTHERMAL | 32×2 | B85 | $600K/quarter, turbines + magma channel |
| 地心寶庫 | CORE VAULT | 40×2 | B100 only | $1M/quarter, the round vault door |

Deep sights take visitors through the normal commerce path (weekend ×1.5, celebrity ×1.5).

**Hazards / 危機:**
- **Flooding:** an occupied floor below B10 with no PUMP within 10 floors has a 30 %/day chance to flood. The flood lasts 6 h, with water rising on screen. The floor's rooms lose 20 eval and get no visitors.
- **Ventilation:** below B20, a room with no VENT within 15 floors gets −20 eval and half the visitors.
- **Deep access:** lifts are limited to 30 floors, so you need express lifts plus deep lobbies.

**CORE rating / 地心級 (second ending):** needs all of the following:
- the TOWER population bar
- refuge floors
- dug to B100
- a CORE VAULT
- every deep floor pumped and ventilated
- 6 of the 8 deep sights

The ending shows a second fireworks screen ("CORE RATING! 地心寶庫 B100 CORE VAULT OPEN", OK: KEEP PLAYING), and the HUD badge becomes CORE.

**Background / 背景:** each basement row is drawn in its depth band:
- soil brown, rock grey, granite purple-grey, and magma dark red with glowing cracks that blink (every 8 frames)
- undug rows add a strata line; dug rows are lighter
- empty dug floors are drawn see-through so the band shows

**RAM / NVS (measured, host harness):**
- `tower_t` 45,264 B (+1.6 KB: `slab` 8 B and `floor_head` 2 B for 96 more floors, plus a few bytes of state). Car `disp` is now int32, because 200 floors in Q8 overflow int16.
- `ui` 3,400 B (+0.8 KB: MAX_ITEMS 48 → 64).
- Save: largest 10,264 B. Only floors ≥ `dug` are saved, so a tower that hasn't started digging saves no more than before. That fits the 48 KB NVS partition easily.
- Save magic is now "TWR3". No device saves exist yet (never flashed).
- CJK: 81 glyphs, 2,106 B.
- Build (ESP-IDF v6, box): image 463,052 B; DRAM used 157,090 B, 164,206 B free; app 0x81000 in a 0x3e0000 slot, 87 % free.

**Redraw:** underground rows join the dirty-row signature (dug state, dig hours, flood hours). Magma rows and the drill animate every 8 / 2 frames. Deep facilities animate like the other rides, only in their own rows. No malloc anywhere.

## Save slots + random events / 存檔同隨機事件 (Oct 10 2026)

**2 save slots**
- Title menu: LOAD 1 / LOAD 2 / NEW TOWER (each slot shows `DAY n *s $xxx` or EMPTY).
- In-game menu: SAVE 1 / SAVE 2 / LOAD 1 / LOAD 2 / TITLE (+ REPAIR POWER / REPAIR LIFT when needed).
- Autosave (quarterly + 30s after build) always writes the **last-used** slot.
- NVS keys: `s0`, `s1`, `slot` (last used), `snd`. Legacy single `save` migrates to slot 1 once.
- Prototype: `localStorage` `tower0` / `tower1` / `tower_slot`.
- Save magic TWR4 (event fields packed). Meta peek reads day/star/funds without a full load.

**Random events (HK flavour, kid-friendly, banner + brief effect, never softlock)**
| Event | Effect |
|-|-|
| Black rain | 4–7 h, half visitors, some workers stay home, heavy rain draw |
| Power cut | lifts stop until REPAIR POWER $80K (or clears in ~6–11 h) |
| Lift breakdown | one shaft dead until REPAIR LIFT $50K (or free after 12–23 h) |
| Celebrity surprise | extra crowds (no studio needed) |
| Small earthquake | −10 eval, dig +3–6 h delay, screen shake |
| Typhoon Signal 1/3/8 | S1 stand by; S3 strong wind / fewer visitors; S8 offices+shops shut |
| Fire | (existing) + blackout after unguarded fire |
| Mid-Autumn | lanterns, tip boost (day 20 of each 36-day year) |
| Christmas + Santa | lights + Santa on lifts (day 33–34) |
| Treasure find | 18% on dig complete, +$50–500K |
| False robbery alarm | 2–3 h, shops near-empty |
| VIP complaint | suite −5 eval |

`g_events_on` / `C.EVENTS_ON` gates random rolls for deterministic tests (VIP booking still runs).
