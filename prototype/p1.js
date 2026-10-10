'use strict';
/* 摩天大廈 SKYRISE — browser prototype (single file).
 * An original tower-management homage for AI Passport (320x240, 3 keys read one at a time).
 * Same rules and tunables as the planned firmware (see PLAN.md). Time: 1 tick = 5 game seconds. */

const C = {
  W: 64,                 // segments per floor (4 px each)
  EVENTS_ON: true,
  NF: 200, BASE: 100,    // floor index 0..199: B100..B1 = 0..99, G = 100, 1/F .. 99/F = 199; B5..B100 open with the Underground challenge
  FH: 24, SEG: 4, TX: 24, TY: 14, VIS: 8,
  TICK_S: 5, DAY: 17280, HOUR: 720, MIN: 12,
  TPF: [0, 4, 8, 16],    // ticks per frame at pause / 1x / 2x / 4x (1x: a day in ~2.4 min)
  NIGHT_TPF: 48,         // midnight..6:00 runs fast
  START_FUNDS: 1000000,
  CAP: { lift: 15, exp: 25 }, MAX_CARS: 4, LIFT_SPAN: 30, EXP_SPEED: 3, DOOR: 2,
  MAX_UNITS: 768, MAX_SHAFTS: 24, MAX_GROUPS: 512, MAX_PEND: 256,
  STAIR_MAX: 4,          // people walk at most 4 flights
  WALK_PER_FLOOR: 6,     // ticks (30 s)
  SKY: [10, 25, 40, 55, 70, 85, 100, 115, 130, 145, 160, 175, 190],  // lobby floors: deep lobbies B90..B15, G, sky lobbies 15/F..90/F
  GRANT_BELOW: 50000, GRANT: 200000,
  HOUSEKEEP_RANGE: 6, HOUSEKEEP_ROOMS: 24, SECURITY_RANGE: 10,
  // stars (scaled for a 64-segment-wide tower; SimTower used 300/1000/5000/10000/15000)
  STAR_POP: [0, 0, 200, 600, 2000, 3500, 5000],
  OFFICE_POP: 6, FLAT_POP: 3,
  // construction time in game hours (0 = instant); RUSH = pay +25% to finish at once
  BUILD_ON: true, RUSH_PCT: 25,
  BUILD_H: { lobby: 0, floor: 0, stairs: 0.5, lift: 6, exp: 12, office: 3, flat: 4, cct: 4, single: 2, twin: 3, suite: 4, housek: 3, security: 3,
             shop: 4, dimsum: 6, cinema: 12, clinic: 12, recycle: 12, parking: 2, metro: 24, chapel: 48,
             refuge: 6, mega: 24, coaster: 48, carpark: 6, pool: 12, ice: 12, atrium: 18, banquet: 18, climb: 8, garden: 12, studio: 24,
             pump: 4, vent: 4, mhub: 24, spa: 18, fossil: 24, gem: 18, nmarket: 12, aqua: 24, lab: 24, geo: 24, vault: 48 },
  REFUGE_GAP: 20,   // HK FS Code 2011 B18: refuge floors at most 25 storeys apart (20 industrial); the game uses 20
  // faster clock, bought once with in-game money only
  SPEED_PRICE: [0, 0, 300000, 1000000], SPEED_STAR: [0, 0, 1, 3],   // 4x clock needs 3 stars
};

/* Facilities. cost in $, w in segments, h floors, star needed, pop = people counted when occupied. */
const T = {
  lobby:   { zh: '大堂', en: 'LOBBY', w: 8, h: 1, cost: 5000, star: 1, col: '#c8b48a', where: 'lobby' },
  floor:   { zh: '空層', en: 'EMPTY FLOOR', w: 1, h: 1, cost: 500, fill: 16, star: 1, col: '#5a5a66' },
  stairs:  { zh: '樓梯', en: 'STAIRS', w: 8, h: 1, cost: 5000, star: 1, col: '#8c7a5a' },
  lift:    { zh: '升降機', en: 'LIFT', w: 4, h: 1, cost: 200000, car: 80000, star: 1, col: '#3a3f4a', shaft: 'lift', maint: 10000 },
  office:  { zh: '寫字樓', en: 'OFFICE', w: 9, h: 1, cost: 40000, star: 1, col: '#7fa8d8', rent: 10000, noisyTo: true },
  flat:    { zh: '住宅', en: 'FLAT', w: 16, h: 1, cost: 80000, star: 1, col: '#e8c890', sale: 150000, quiet: true },
  cct:     { zh: '茶餐廳', en: 'CHA CHAAN TENG', w: 16, h: 1, cost: 100000, star: 1, col: '#e8584a', perCust: 100, base: 35, noisy: true, open: [7, 21] },
  single:  { zh: '單人房', en: 'HOTEL SINGLE', w: 4, h: 1, cost: 20000, star: 2, col: '#b8e0c0', night: 1500, guests: 1, hotel: true, quiet: true },
  housek:  { zh: '房務部', en: 'HOUSEKEEPING', w: 15, h: 1, cost: 50000, star: 2, col: '#a0a0b8', maint: 10000 },
  security:{ zh: '保安室', en: 'SECURITY', w: 16, h: 1, cost: 100000, star: 2, col: '#4a6aa8', maint: 20000 },
  twin:    { zh: '雙人房', en: 'HOTEL TWIN', w: 6, h: 1, cost: 50000, star: 3, col: '#90d0a8', night: 3000, guests: 2, hotel: true, quiet: true },
  suite:   { zh: '套房', en: 'HOTEL SUITE', w: 10, h: 1, cost: 100000, star: 3, col: '#60b888', night: 6000, guests: 2, hotel: true, quiet: true },
  shop:    { zh: 'SHOP', en: 'SHOP', w: 12, h: 1, cost: 100000, star: 3, col: '#e890c8', perCust: 150, base: 25, noisy: true, open: [10, 21] },
  dimsum:  { zh: '酒樓', en: 'DIM SUM HALL', w: 24, h: 1, cost: 200000, star: 3, col: '#d84040', perCust: 200, base: 35, noisy: true, open: [7, 15] },
  cinema:  { zh: '戲院', en: 'CINEMA', w: 31, h: 2, cost: 500000, star: 3, col: '#503070', perCust: 120, base: 60, noisy: true, show: [13, 19] },
  clinic:  { zh: '診所', en: 'CLINIC', w: 26, h: 1, cost: 500000, star: 3, col: '#f0f0f0' },
  recycle: { zh: '回收站', en: 'RECYCLING', w: 25, h: 2, cost: 500000, star: 3, col: '#5a8a4a', maint: 50000 },
  parking: { zh: '停車場', en: 'CAR PARK', w: 16, h: 1, cost: 50000, star: 3, col: '#707070', where: 'base', maint: 10000 },
  exp:     { zh: '高速升降機', en: 'EXPRESS LIFT', w: 6, h: 1, cost: 400000, car: 150000, star: 3, col: '#2a2f3a', shaft: 'exp', maint: 20000 },
  metro:   { zh: '地鐵站', en: 'METRO STATION', w: 30, h: 3, cost: 1000000, star: 4, col: '#c03030', where: 'metro', maint: 100000 },
  refuge:  { zh: '防火層', en: 'REFUGE', w: 64, h: 1, cost: 300000, star: 1, col: '#606862', maint: 20000, whole: true },
  mega:    { zh: '戲院', en: 'MEGA CINEMA', w: 40, h: 3, cost: 1500000, star: 4, col: '#281840', perCust: 160, base: 150, noisy: true, show: [14, 19, 21] },
  coaster: { zh: '過山車', en: 'COASTER', w: 48, h: 5, cost: 3000000, star: 4, col: '#181c34', perCust: 250, base: 140, noisy: true, loud: true, open: [10, 23], maint: 50000 },
  carpark: { zh: '停車場', en: 'PARKING', w: 20, h: 2, cost: 300000, star: 2, col: '#646870', where: 'park', maint: 20000 },
  pool:    { zh: '泳池', en: 'POOL', w: 24, h: 2, cost: 600000, star: 3, col: '#3c8cc8', perCust: 60, base: 70, open: [7, 22], maint: 20000 },
  ice:     { zh: '溜冰場', en: 'ICE RINK', w: 28, h: 2, cost: 800000, star: 3, col: '#c8e6f5', perCust: 90, base: 70, open: [10, 22], maint: 30000 },
  atrium:  { zh: '中庭', en: 'ATRIUM', w: 24, h: 4, cost: 1000000, star: 3, col: '#e6dcc8', maint: 30000 },
  banquet: { zh: '宴會廳', en: 'BANQUET', w: 32, h: 2, cost: 1200000, star: 4, col: '#aa1e28', perCust: 300, base: 30, open: [11, 23], maint: 20000 },
  climb:   { zh: '攀石', en: 'CLIMB', w: 12, h: 3, cost: 400000, star: 3, col: '#96785a', perCust: 80, base: 40, open: [10, 22], maint: 10000 },
  garden:  { zh: '空中花園', en: 'SKY GARDEN', w: 24, h: 2, cost: 800000, star: 4, col: '#468c46', where: 'high', maint: 20000 },
  studio:  { zh: '電視台', en: 'TV STUDIO', w: 28, h: 3, cost: 1500000, star: 4, col: '#1e1e28' },
  pump: { zh: '泵房', en: 'PUMP ROOM', w: 6, h: 1, cost: 150000, star: 1, col: '#3c5a82', maint: 15000, dmin: 1 },
  vent: { zh: '通風', en: 'VENT SHAFT', w: 4, h: 1, cost: 120000, star: 1, col: '#6e7878', maint: 10000, dmin: 1 },
  mhub: { zh: '地鐵', en: 'METRO HUB', w: 40, h: 3, cost: 2000000, star: 1, col: '#c83c32', perCust: 40, base: 400, open: [6, 24], maint: 50000, dmin: 10 },
  spa: { zh: '溫泉', en: 'HOT SPRING', w: 28, h: 2, cost: 1500000, star: 1, col: '#c87850', perCust: 220, base: 80, open: [10, 24], maint: 30000, dmin: 20 },
  fossil: { zh: '化石館', en: 'FOSSIL MUSEUM', w: 32, h: 3, cost: 1800000, star: 1, col: '#aa966e', perCust: 150, base: 90, open: [10, 19], maint: 30000, dmin: 30 },
  gem: { zh: '寶石', en: 'GEM MINE', w: 20, h: 2, cost: 1500000, star: 1, col: '#5a3c78', perCust: 400, base: 40, open: [10, 21], maint: 20000, dmin: 40 },
  nmarket: { zh: '夜市', en: 'NIGHT MARKET', w: 40, h: 1, cost: 1200000, star: 1, col: '#a02832', perCust: 120, base: 140, open: [17, 24], maint: 20000, dmin: 50 },
  aqua: { zh: '水族館', en: 'AQUARIUM', w: 36, h: 3, cost: 2500000, star: 1, col: '#14468c', perCust: 200, base: 110, open: [10, 21], maint: 50000, dmin: 60 },
  lab: { zh: '實驗室', en: 'SECRET LAB', w: 24, h: 2, cost: 3000000, star: 1, col: '#283c3c', dmin: 70 },
  geo: { zh: '地熱', en: 'GEOTHERMAL', w: 32, h: 2, cost: 4000000, star: 1, col: '#782814', dmin: 85 },
  vault: { zh: '地心寶庫', en: 'CORE VAULT', w: 40, h: 2, cost: 10000000, star: 1, col: '#dcb43c', dmin: 99 },
  chapel:  { zh: '天空禮堂', en: 'SKY CHAPEL', w: 28, h: 4, cost: 3000000, star: 5, col: '#f8f0d8', where: 'top' },
};
const TKEYS = Object.keys(T);
const TIDX = Object.fromEntries(TKEYS.map((k, i) => [k, i]));
const BUILD_ORDER = ['lobby', 'floor', 'stairs', 'lift', 'office', 'flat', 'cct', 'single', 'housek', 'security', 'refuge',
  'twin', 'suite', 'shop', 'dimsum', 'cinema', 'clinic', 'recycle', 'parking', 'carpark', 'pool', 'ice', 'climb', 'atrium', 'exp',
  'mega', 'coaster', 'banquet', 'garden', 'studio', 'metro', 'chapel', 'pump', 'vent', 'mhub', 'spa', 'fossil', 'gem', 'nmarket', 'aqua', 'lab', 'geo', 'vault'];
const LEISURE = (t) => ['pool', 'ice', 'climb', 'banquet'].includes(t);
const DEEP_SIGHTS = ['mhub', 'spa', 'fossil', 'gem', 'nmarket', 'aqua'];
const TENANT = (t) => ['office', 'flat', 'cct', 'single', 'twin', 'suite', 'shop', 'dimsum', 'cinema', 'mega', 'coaster', 'pool', 'ice', 'banquet', 'climb', ...DEEP_SIGHTS].includes(t);
const FOOD = (t) => t === 'cct' || t === 'dimsum';
const COMMERCE = (t) => ['cct', 'dimsum', 'shop', 'cinema', 'mega', 'coaster', 'pool', 'ice', 'banquet', 'climb', ...DEEP_SIGHTS].includes(t);
const unitH = (u) => u.h;

function depth(f) { return f < C.BASE ? C.BASE - f : 0; }
function digMult(d) { return d <= 30 ? 1 : d <= 60 ? 2 : d <= 85 ? 3 : 4; }   // soil, rock, granite, magma
function digCost() { const d = depth(S.dug - 1); return (100000 + 15000 * d) * digMult(d); }
function digHours() { const d = depth(S.dug - 1); return Math.floor((2 + Math.floor(d / 10)) * (digMult(d) + 1) / 2); }
function repairPowerCost() { return 80000; }
function repairLiftCost() { return 50000; }
function repairPower() {
  if (!S.powerH) return 'POWER IS ON';
  const c = repairPowerCost(); if (c > S.funds) return 'NOT ENOUGH $';
  spend(c); S.powerH = 0; note('POWER RESTORED! LIFTS RUNNING'); return null;
}
function repairLift() {
  if (S.brokenSi < 0) return 'NO BROKEN LIFT';
  const c = repairLiftCost(); if (c > S.funds) return 'NOT ENOUGH $';
  spend(c); S.brokenSi = -1; S.brokenH = 0; note('THE LIFT IS FIXED!'); return null;
}
function shaftLive(si) { return !S.powerH && S.brokenSi !== si; }
function dig() {
  if (!S.deep) return 'UNDERGROUND LOCKED';
  if (S.dug === 0) return 'B100 REACHED';
  if (S.digH) { const c = Math.floor(digCost() * C.RUSH_PCT / 100); if (c > S.funds) return 'NOT ENOUGH $'; spend(c); S.digH = 1; return null; }
  const c = digCost(); if (c > S.funds) return 'NOT ENOUGH $';
  spend(c);
  if (!C.BUILD_ON) { S.dug--; return null; }
  S.digH = S.digH0 = digHours(); return null;
}
function deepNear(t, f, r) { return S.units.some((o) => o.t === t && !o.build && o.f + o.h - 1 >= f - r && o.f <= f + r); }
function deepUncovered(kind) {
  const lim = kind ? 20 : 10, t = kind ? 'vent' : 'pump', r = kind ? 15 : 10;
  for (let f = C.BASE - lim - 1; f >= 0; f--) if (S.units.some((u) => u.t !== 'floor' && u.f <= f && u.f + u.h - 1 >= f) && !deepNear(t, f, r)) return f;
  return -1;
}
function ventOK(f) { return depth(f) <= 20 || deepNear('vent', f, 15); }
function floorName(f) { return f < C.BASE ? 'B' + (C.BASE - f) : f === C.BASE ? 'G' : String(f - C.BASE); }
const isLobbyFloor = (f) => C.SKY.includes(f);

/* ---------------------------------------------------------------- rng */
let rngS = 0x1234567;
function seed(s) { rngS = (s >>> 0) || 1; }
function rand() { rngS ^= rngS << 13; rngS >>>= 0; rngS ^= rngS >>> 17; rngS ^= rngS << 5; rngS >>>= 0; return rngS / 4294967296; }
const ri = (n) => Math.floor(rand() * n);

/* ---------------------------------------------------------------- state */
const S = {};
function freshState() {
  Object.assign(S, {
    mode: 'title', menu: null, msg: '', msgT: 0, log: [],
    tick: 6 * C.HOUR, day: 0,                // day 0 = WD1 Q1 Y1; days go WD1, WD2, WE
    funds: C.START_FUNDS, star: 1, pop: 0, speed: 1,
    units: [], shafts: [], groups: [], pend: [],
    grid: new Int16Array(C.NF * C.W),         // 0 empty, >0 unit id+1, <0 -(shaft index+1)
    struct: new Uint8Array(C.NF * C.W),
    access: new Uint8Array(C.NF),
    cur: C.BASE, cam: C.BASE - 2, x: 28,       // cursor floor, lowest visible floor, cursor segment
    vipOK: false, vip: null, wedding: false, wedSent: false, wedArrived: 0, wedWait: 0, lastV: undefined, building: false, towerDone: false, celebrate: false, deep: false, coreDone: false, dug: C.BASE - 4, digH: 0, digH0: 0, floodF: -1, floodH: 0, endKind: 0, endSel: 0, powerH: 0, rainH: 0, quakeH: 0, alarmH: 0, brokenSi: -1, brokenH: 0, fest: 0, santa: false, complain: false, typhoonSig: 0, slot: 0, typhoon: false, fire: null,
    grantQ: -1, nextId: 1, sound: true, snd: 0, swap: false, rush: false, owned: [1, 1, 0, 0],
    stats: { spawned: 0, arrived: 0, failed: 0, overflow: 0, waitSum: 0, waitN: 0, maxWait: 0, peakGroups: 0,
             income: 0, spend: 0, rushed: 0, speedBuys: 0, dayIn: 0, dayOut: 0, qIn: 0, qOut: 0, rentQ: 0, sales: 0, grants: 0 },
    last: 'office', place: null,
  });
}
const quarter = () => Math.floor(S.day / 3) % 4;
const year = () => Math.floor(S.day / 12) + 1;
const dayKind = () => S.day % 3;           // 0 WD1, 1 WD2, 2 WE
const weekend = () => dayKind() === 2;
const hour = () => S.tick / C.HOUR;
function dateStr() { return ['WD1', 'WD2', 'WE'][dayKind()] + ' Q' + (quarter() + 1) + ' Y' + year(); }
function clockStr() { const m = Math.floor(S.tick / C.MIN); return String(Math.floor(m / 60)).padStart(2, '0') + ':' + String(m % 60).padStart(2, '0'); }
function note(text) { S.msg = text; S.msgT = 150; S.log.unshift(dateStr() + ' ' + clockStr() + ' ' + text); if (S.log.length > 30) S.log.pop(); }
const unitById = (id) => S.units.find((u) => u.id === id);

/* ---------------------------------------------------------------- building rules */
function hasRefuge(f, liveOnly) { return S.units.some((u) => u.t === 'refuge' && u.f === f && (!liveOnly || !u.build)); }
function isRefugeFloor(f) { return hasRefuge(f, true); }
function refugeCovered(f) { if (f <= C.BASE + C.REFUGE_GAP) return true; for (let r = f - 1; r >= f - C.REFUGE_GAP; r--) if (hasRefuge(r, true)) return true; return false; }
function uncoveredFloor() {
  const used = new Set(S.units.filter((u) => u.t !== 'floor' && u.t !== 'refuge').map((u) => u.f));
  for (let f = C.BASE + C.REFUGE_GAP + 1; f < C.NF; f++) if (used.has(f) && !refugeCovered(f)) return f;
  return -1;
}
function canRefuge(f) {
  if (f <= C.BASE) return 'ABOVE G/F ONLY';
  if (isLobbyFloor(f)) return 'SKY LOBBY FLOOR';
  if (hasRefuge(f)) return 'ALREADY REFUGE';
  let n = 0;   // unsupported holes are skipped; it needs some floor to stand on
  for (let x = 0; x < C.W; x++) {
    const g = S.grid[f * C.W + x];
    if (g < 0) continue;
    if (g > 0) { const o = unitById(g - 1); if (o && o.t === 'stairs') continue; if (o && o.t === 'floor') { n++; continue; } return 'FLOOR NOT EMPTY'; }
    n += supported(f, x, 1) ? 1 : 0;
  }
  if (!n) return 'NO FLOOR BELOW';
  if (cost('refuge', f) > S.funds) return 'NOT ENOUGH $';
  return null;
}
function parkH(v) { return v >= 2 && v <= 4 ? v : 2; }
function canPlace(t, f, x, top, v) {
  const d = T[t];
  if (d.star > S.star) return 'NEEDS ' + d.star + ' STARS';
  if (t === 'refuge') return canRefuge(f);
  if (d.dmin) {
    if (!S.deep) return 'UNDERGROUND CHALLENGE';
    if (f >= C.BASE) return 'UNDERGROUND ONLY';
    if (t === 'vault' && f !== 0) return 'B100 ONLY';
    if (depth(f) < d.dmin) return 'B' + d.dmin + ' AND DEEPER';
  }
  if (f < S.dug) return S.deep ? 'DIG FIRST' : 'SOLID ROCK';
  if (t === 'carpark') {
    const vv = v !== undefined ? v : (S.place && S.place.t === t ? S.place.v : 2), h = parkH(vv);
    if (x < 0 || x + d.w > C.W) return 'OUTSIDE';
    if (f < C.BASE - 4 || f + h - 1 > C.BASE) return 'B4-G ONLY';
    for (let ff = f; ff < f + h; ff++) for (let xx = x; xx < x + d.w; xx++) {
      const g = S.grid[ff * C.W + xx];
      if (g < 0 || (g > 0 && !['floor', 'lobby'].includes((unitById(g - 1) || {}).t))) return 'BLOCKED';
    }
    if (f + h - 1 < C.BASE && !supported(f + h - 1, x, d.w)) return 'DIG FROM ABOVE';
    if (d.cost * h / 2 > S.funds) return 'NOT ENOUGH $';
    return null;
  }
  if (d.where === 'high' && f < C.BASE + 60) return '60/F AND ABOVE';
  if (x < 0 || x + d.w > C.W) return 'OUTSIDE';
  if (d.shaft) {
    const f1 = top === undefined ? f : top;
    if (f1 < f) return 'BAD SPAN';
    if (d.shaft === 'lift' && f1 - f + 1 > C.LIFT_SPAN) return 'MAX ' + C.LIFT_SPAN + ' FLOORS';
    if (S.shafts.length >= C.MAX_SHAFTS) return 'MAX ' + C.MAX_SHAFTS + ' SHAFTS';
    for (let ff = f; ff <= f1; ff++) {
      for (let xx = x; xx < x + d.w; xx++) {
        const g = S.grid[ff * C.W + xx];
        if (g !== 0 && !(g > 0 && unitById(g - 1) && ['lobby', 'floor', 'refuge'].includes(unitById(g - 1).t))) return 'BLOCKED';   // lifts pass through refuge floors
      }
      if (ff === f && !supported(ff, x, d.w)) return 'NO FLOOR BELOW';
    }
    if (cost(t, f, f1) > S.funds) return 'NOT ENOUGH $';
    return null;
  }
  const h = d.h;
  if (f + h - 1 >= C.NF) return 'TOO HIGH';
  if (d.where === 'lobby' && !isLobbyFloor(f)) return 'LOBBY: G + EVERY 15 FLOORS';
  if (d.where !== 'lobby' && t !== 'stairs' && t !== 'floor' && isLobbyFloor(f) && f !== C.BASE) return 'SKY LOBBY FLOOR';
  if (d.where === 'base' && f >= C.BASE) return 'BASEMENT ONLY';
  if (d.where === 'metro' && f !== C.BASE - 4) return 'B4-B2 ONLY';
  if (d.where === 'top' && f + h - 1 !== C.NF - 1) return 'TOP 4 FLOORS ONLY';
  if (t === 'chapel' && S.units.some((u) => u.t === 'chapel')) return 'ONLY ONE';
  if (t === 'metro' && S.units.some((u) => u.t === 'metro')) return 'ONLY ONE';
  if (f === C.BASE && t !== 'lobby' && t !== 'stairs') return 'G/F IS THE LOBBY';
  for (let ff = f; ff < f + h; ff++) if (t !== 'stairs' && hasRefuge(ff)) return 'REFUGE FLOOR';
  for (let ff = f; ff < f + h; ff++) for (let xx = x; xx < x + d.w; xx++) {
    const g = S.grid[ff * C.W + xx];
    if (g !== 0) {
      if (g > 0 && unitById(g - 1) && (unitById(g - 1).t === 'floor' || (t === 'stairs' && unitById(g - 1).t === 'lobby'))) continue;
      return 'BLOCKED';
    }
  }
  if (!supported(f < C.BASE ? f + h - 1 : f, x, d.w)) return f < C.BASE ? 'DIG FROM ABOVE' : 'NO FLOOR BELOW';   // basements hang from the floor above their top
  if (t === 'stairs' && f + 1 >= C.NF) return 'TOP FLOOR';
  if (cost(t, f) > S.funds) return 'NOT ENOUGH $';
  return null;
}
function supported(f, x, w) {
  if (f === C.BASE) return true;
  const g = f > C.BASE ? f - 1 : f + 1;   // above ground: built below; basements: built above
  for (let xx = x; xx < x + w; xx++) if (!S.struct[g * C.W + xx]) return false;
  return true;
}
function buildTicks(t) { return C.BUILD_ON ? Math.round((C.BUILD_H[t] || 0) * C.HOUR) : 0; }
const rushOn = (t) => S.rush && buildTicks(t) > 0;
function cost(t, f, f1) {
  const d = T[t];
  const base = d.shaft ? d.cost + d.car + 2000 * ((f1 === undefined ? f : f1) - f) : d.cost;
  return rushOn(t) ? Math.round(base * (100 + C.RUSH_PCT) / 100) : base;
}
const live = (u) => !u.build;
function place(t, f, x, top, v) {
  const why = canPlace(t, f, x, top, v);
  if (why) return why;
  const d = T[t];
  if (d.shaft) {
    const f1 = top === undefined ? f : top;
    for (let ff = f; ff <= f1; ff++) carve(ff, x, d.w);
    const s = { t: d.shaft, x, w: d.w, f0: f, f1, cars: [], q: [], build: rushOn(t) ? 0 : buildTicks(t), build0: buildTicks(t) };
    for (let ff = 0; ff < C.NF; ff++) s.q.push([]);
    S.shafts.push(s);
    const si = S.shafts.length - 1;
    for (let ff = f; ff <= f1; ff++) for (let xx = x; xx < x + d.w; xx++) { S.grid[ff * C.W + xx] = -(si + 1); S.struct[ff * C.W + xx] = 1; }
    addCar(si, true);
    spend(cost(t, f, f1) - d.car);
    if (rushOn(t)) S.stats.rushed++;
    computeAccess();
    return null;
  }
  if (t === 'refuge') {   // whole floor, one piece per gap between shafts / stairs
    for (const o of S.units.filter((o) => o.f === f && o.t === 'floor')) removeUnit(o, true);
    for (let xx = 0; xx < C.W;) {
      if (S.grid[f * C.W + xx] || !supported(f, xx, 1)) { xx++; continue; }
      const x0 = xx; while (xx < C.W && !S.grid[f * C.W + xx] && supported(f, xx, 1)) xx++;
      const u = addPiece('refuge', f, x0, xx - x0); u.build = rushOn(t) ? 0 : buildTicks(t); u.build0 = buildTicks(t);
      for (let k = x0; k < xx; k++) S.struct[f * C.W + k] = 1;
    }
    if (rushOn(t)) S.stats.rushed++;
    spend(cost(t, f)); computeAccess();
    return null;
  }
  if (t === 'carpark') {
    const h = parkH(v);
    for (let ff = f; ff < f + h; ff++) carve(ff, x, d.w);
    const u = { id: S.nextId++, t, f, x, w: d.w, h, v: h, occ: 1, eval: 70, rent: 1, sSum: 0, sN: 0, inside: 0, transit: 0, cust: 0, custY: 0, inc: 0, dirty: 0, dirtyDays: 0, roach: 0, booked: 0, lowDays: 0, fire: 0 };
    u.build = rushOn(t) ? 0 : buildTicks(t); u.build0 = buildTicks(t);
    S.units.push(u);
    for (let ff = f; ff < f + h; ff++) for (let xx = x; xx < x + d.w; xx++) { S.grid[ff * C.W + xx] = u.id + 1; S.struct[ff * C.W + xx] = 1; }
    spend(d.cost * h / 2); computeAccess();
    return null;
  }
  let w = d.w;
  if (d.fill) { while (w < d.fill && x + w < C.W && !S.grid[f * C.W + x + w] && supported(f, x + w, 1) && S.funds >= d.cost * (w + 1)) w++; }
  // stairs over a lobby/empty floor: replace the cells
  const u = { id: S.nextId++, t, f, x, w, h: d.h, occ: 0, eval: 70, rent: 1, sSum: 0, sN: 0,
              inside: 0, transit: 0, cust: 0, custY: 0, inc: 0, dirty: 0, dirtyDays: 0, roach: 0, booked: 0, lowDays: 0, fire: 0 };
  for (let ff = f; ff < f + d.h; ff++) carve(ff, x, w);
  u.build = rushOn(t) ? 0 : buildTicks(t); u.build0 = buildTicks(t); u.v = v === undefined ? u.id % 6 : v;
  if (rushOn(t)) S.stats.rushed++;
  if (S.units.length >= C.MAX_UNITS) return 'MAX ' + C.MAX_UNITS + ' ROOMS';
  S.units.push(u);
  for (let ff = f; ff < f + d.h; ff++) for (let xx = x; xx < x + w; xx++) { S.grid[ff * C.W + xx] = u.id + 1; S.struct[ff * C.W + xx] = 1; }
  spend(d.fill ? d.cost * w : cost(t, f));
  if (t === 'stairs' || t === 'lobby' || t === 'floor') computeAccess();
  if (!TENANT(t)) u.occ = 1;
  if (u.build && T[t].shaft === undefined) S.building = true;
  return null;
}
/* cut [x, x+w) out of empty-floor (and, for stairs/lifts, lobby) pieces; the leftovers stay */
function carve(f, x, w) {
  for (let xx = x; xx < x + w; xx++) {
    const g = S.grid[f * C.W + xx];
    if (g <= 0) continue;
    const o = unitById(g - 1);
    if (!o || (o.t !== 'floor' && o.t !== 'lobby' && o.t !== 'refuge')) continue;
    removeUnit(o, true);
    const L = x - o.x, R = o.x + o.w - (x + w);
    if (L > 0) addPiece(o.t, f, o.x, L);
    if (R > 0) addPiece(o.t, f, x + w, R);
  }
}
function addPiece(t, f, x, w) {
  const u = { id: S.nextId++, t, f, x, w, h: 1, occ: 1, eval: 70, rent: 1, sSum: 0, sN: 0, inside: 0, transit: 0, cust: 0, custY: 0, inc: 0, dirty: 0, dirtyDays: 0, roach: 0, booked: 0, lowDays: 0, fire: 0 };
  S.units.push(u);
  for (let xx = x; xx < x + w; xx++) S.grid[f * C.W + xx] = u.id + 1;
  return u;
}
function removeUnit(u, silent) {
  for (let ff = u.f; ff < u.f + u.h; ff++) for (let xx = u.x; xx < u.x + u.w; xx++) if (S.grid[ff * C.W + xx] === u.id + 1) S.grid[ff * C.W + xx] = 0;
  S.units.splice(S.units.indexOf(u), 1);
  for (const g of S.groups) if (g.u === u.id) g.u = -1;
  for (const p of S.pend) if (p.u === u.id) p.u = -1;
  if (!silent) computeAccess();
}
function demolish(u) {
  // the slab stays (struct), so floors above keep standing; housekeeping/security can't be removed (like the original)
  if (u.t === 'housek' || u.t === 'security') return 'CANNOT REMOVE';
  if (u.t === 'refuge') {   // the whole refuge floor goes back to empty floor
    for (const o of S.units.filter((o) => o.t === 'refuge' && o.f === u.f)) { removeUnit(o, true); addPiece('floor', o.f, o.x, o.w); }
    spend(Math.floor(T.refuge.cost / 10)); computeAccess();
    return null;
  }
  removeUnit(u);
  spend(Math.floor(T[u.t].cost / 10));
  return null;
}
function addCar(si, free) {
  const s = S.shafts[si];
  if (s.cars.length >= C.MAX_CARS) return 'MAX ' + C.MAX_CARS + ' CARS';
  const c = T[s.t].car;
  if (!free && c > S.funds) return 'NOT ENOUGH $';
  s.cars.push(newCar(s.f0));
  spend(c);
  return null;
}
/* a car: sim state (f, dir, load, g, door) + drawing state (disp = shown floor, doorA 0..1 open, people in/out, stop) */
function newCar(f) { return { f, dir: 0, load: 0, g: [], door: 0, disp: f, doorA: 0, hold: 0, stopF: -1, pin: 0, pout: 0, anim: [] }; }
function spend(n) { S.funds -= n; S.stats.spend += n; S.stats.dayOut += n; S.stats.qOut += n; }
function earn(n) { S.funds += n; S.stats.income += n; S.stats.dayIn += n; S.stats.qIn += n; }

/* ---------------------------------------------------------------- routes */
function serves(s, f) { const si = S.shafts.indexOf(s); return !s.build && shaftLive(si < 0 ? 999 : si) && f >= s.f0 && f <= s.f1 && (s.t === 'lift' || isLobbyFloor(f) || f === s.f0 || f === s.f1); }
function stairsOk(a, b) {
  if (Math.abs(a - b) > C.STAIR_MAX) return false;
  const lo = Math.min(a, b), hi = Math.max(a, b);
  for (let f = lo; f < hi; f++) if (!S.units.some((u) => u.t === 'stairs' && u.f === f && live(u))) return false;
  return true;
}
const qlen = (s, f) => s.q[f].length;
/* At most one change of lift, and only at a lobby floor (like the original's one transfer). */
function route(a, b) {
  if (a === b) return { walk: 0, legs: [] };
  if (stairsOk(a, b)) return { walk: Math.abs(a - b), legs: [] };
  let best = null, bq = 1e9;
  S.shafts.forEach((s, i) => { if (s.cars.length && serves(s, a) && serves(s, b)) { const q = qlen(s, a); if (q < bq) { bq = q; best = { walk: 0, legs: [{ s: i, from: a, to: b }] }; } } });
  if (best) return best;
  for (const t of C.SKY) {
    if (t === a || t === b) continue;
    S.shafts.forEach((s1, i) => {
      if (!s1.cars.length || !serves(s1, a) || !serves(s1, t)) return;
      S.shafts.forEach((s2, j) => {
        if (i === j || !s2.cars.length || !serves(s2, t) || !serves(s2, b)) return;
        const q = qlen(s1, a) + qlen(s2, t);
        if (q < bq) { bq = q; best = { walk: 0, legs: [{ s: i, from: a, to: t }, { s: j, from: t, to: b }] }; }
      });
    });
  }
  // a few flights of stairs to or from a lift
  if (!best) {
    for (let d = 1; d <= 2 && !best; d++) for (const a2 of [a - d, a + d]) {
      if (a2 < 0 || a2 >= C.NF || !stairsOk(a, a2)) continue;
      S.shafts.forEach((s, i) => { if (!best && s.cars.length && serves(s, a2) && serves(s, b)) best = { walk: d, legs: [{ s: i, from: a2, to: b }] }; });
    }
  }
  return best;
}
function entryFloor() { const m = S.units.find((u) => u.t === 'metro'); return m && rand() < 0.3 ? m.f + 1 : C.BASE; }
function computeAccess() {
  const lobby = S.units.some((u) => u.t === 'lobby' && u.f === C.BASE && live(u));
  for (let f = 0; f < C.NF; f++) S.access[f] = lobby && (f === C.BASE || !!route(C.BASE, f)) ? 1 : 0;
}
