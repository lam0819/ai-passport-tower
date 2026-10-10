
/* ---------------------------------------------------------------- sound (tiny beeps) */
let AC = null;
function sfx(k) {
  if (!S.sound || typeof AudioContext === 'undefined') return;
  try {
    AC = AC || new AudioContext(); if (AC.state === 'suspended') AC.resume();
    const seq = { tick: [880], build: [523, 784], bad: [196], fanfare: [523, 659, 784, 1047], ok: [660] }[k] || [440];
    seq.forEach((fr, i) => { const o = AC.createOscillator(), g = AC.createGain(); o.type = 'square'; o.frequency.value = fr;
      g.gain.value = 0.04; o.connect(g); g.connect(AC.destination); o.start(AC.currentTime + i * 0.09); o.stop(AC.currentTime + i * 0.09 + 0.08); });
  } catch (e) { /* audio is optional */ }
}

/* ---------------------------------------------------------------- calm background music (day / night loops) */
// Same notes as the firmware (main/sound.c): {MIDI note, eighths}, 0 = rest; one bass root per bar.
const TUNES = {
  1: { bpm: 92, notes: [[72,2],[74,2],[76,3],[79,1],[76,2],[74,2],[72,4],[69,2],[72,2],[74,4],[76,2],[74,2],[72,2],[69,2],[67,4],[69,2],[72,2],[76,3],[74,1],[72,4],[0,4],[79,2],[76,2],[74,2],[72,2],[74,4],[72,4],[0,4]], bass: [36,33,41,43,36,33,41,36] },
  2: { bpm: 70, notes: [[69,4],[0,2],[72,2],[76,6],[74,2],[72,4],[0,4],[67,4],[69,4],[64,8],[0,4],[72,2],[74,2],[76,4],[79,4],[76,4],[74,2],[72,2],[69,8],[0,8]], bass: [33,29,31,28,33,29,36,33] },
};
const SOUND_NAMES = ['SOUND: MUSIC + FX', 'SOUND: FX ONLY', 'SOUND: OFF'];
function wantMusic() { if ((S.snd || 0) !== 0) return 0; if (S.mode === 'title') return 2; return night() || hour() >= 19 ? 2 : 1; }
const MU = { id: 0, t: 0, i: 0, e8: 0 };
function musicTick() {   // schedules ~0.5 s ahead; WebAudio does the rest (low CPU)
  const want = wantMusic();
  if (typeof AudioContext === 'undefined' || !AC) { MU.id = want; return; }
  if (want !== MU.id) { MU.id = want; MU.t = AC.currentTime + 0.1; MU.i = 0; MU.e8 = 0; }
  const tu = TUNES[MU.id]; if (!tu) return;
  const e = 30 / tu.bpm, hz = (n) => 440 * Math.pow(2, (n - 69) / 12);
  while (MU.t < AC.currentTime + 0.5) {
    const [n, len] = tu.notes[MU.i];
    const tone = (f, t0, d, vol, type) => { const o = AC.createOscillator(), g = AC.createGain(); o.type = type; o.frequency.value = f;
      g.gain.setValueAtTime(vol, t0); g.gain.linearRampToValueAtTime(vol * 0.5, t0 + d * 0.9); g.gain.linearRampToValueAtTime(0, t0 + d);
      o.connect(g); g.connect(AC.destination); o.start(t0); o.stop(t0 + d + 0.02); };
    if (n) tone(hz(n), MU.t, len * e * 0.9, 0.035, 'triangle');
    for (let k = 0; k < len; k++) if ((MU.e8 + k) % 2 === 0) { const bar = Math.floor((MU.e8 + k) / 8) % tu.bass.length, beat = ((MU.e8 + k) / 2) & 1;
      tone(hz(tu.bass[bar] + (beat ? 7 : 0)), MU.t + k * e, e * 1.8, 0.03, 'triangle'); }
    MU.e8 += len; MU.t += len * e; MU.i = (MU.i + 1) % tu.notes.length;
  }
}

/* ---------------------------------------------------------------- input: 3 keys, one at a time */
// UP / DOWN: tap = 1 step, hold = repeat (faster the longer you hold).
// OK: tap = select / open menu. Hold 0.5 s = back / leave (fires once, no tap on release).
const K = { up: 0, down: 0, ok: 0 }, KH = { up: 0, down: 0, ok: 0 };
let okLong = false;
function setKey(k, down) {
  if (k !== 'ok' && S.swap) k = k === 'up' ? 'down' : 'up';
  if (down) { if (!K[k]) { for (const o in K) if (o !== k) K[o] = 0; K[k] = 1; KH[k] = 0; } }
  else {
    if (K[k] && k === 'ok' && !okLong) act('ok');
    K[k] = 0; if (k === 'ok') okLong = false;
  }
}
function pollKeys() {
  for (const k of ['up', 'down']) if (K[k]) {
    const h = KH[k]++;
    if (h === 0 || (h >= 12 && (h < 40 ? h % 4 === 0 : true))) act(k);
  }
  if (K.ok) { if (++KH.ok === 15) { okLong = true; act('back'); } }
}
function openMenu(title, items, sel) { S.menu = { title, items, sel: sel || 0, prev: S.mode }; S.mode = 'menu'; }
function closeMenu(to) { S.mode = to || 'play'; S.menu = null; }
function mainMenu() {
  const items = [];
  if (S.last) items.push({ label: 'BUILD AGAIN: ' + (S.last === 'shop' && S.lastV !== undefined ? SHOPS[S.lastV].en : T[S.last].en), fn: () => startPlace(S.last, S.lastV) });
  items.push({ label: 'BUILD...', fn: buildMenu },
    { label: 'INSPECT ' + floorName(S.cur) + '/F', fn: startInspect },
    { label: 'SPEED: ' + ['PAUSE', '1X', '2X', '4X'][S.speed], fn: () => { do S.speed = (S.speed + 1) % 4; while (S.speed && !S.owned[S.speed]); mainMenu(); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('SPEED')); } },
    { label: 'SPEED SHOP...', fn: speedShop },
    { label: 'RUSH BUILD: ' + (S.rush ? 'ON +' + C.RUSH_PCT + '%' : 'OFF'), fn: () => { S.rush = !S.rush; mainMenu(); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('RUSH')); } },
    { label: SOUND_NAMES[S.snd || 0], fn: () => { S.snd = ((S.snd || 0) + 1) % 3; S.sound = S.snd < 2; save(); mainMenu(); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('SOUND')); } },
    ...(S.deep && S.dug > 0 ? [{ label: (S.digH ? 'RUSH DIG ' + floorName(S.dug - 1) + ' (' + S.digH + 'H LEFT)' : 'DIG ' + floorName(S.dug - 1) + ' (' + digHours() + 'H)'),
      sub: '$' + kfmt(S.digH ? Math.floor(digCost() * C.RUSH_PCT / 100) : digCost()),
      fn: () => { const w = dig(); note(w || (S.digH ? 'DIGGING ' + floorName(S.dug - 1) + '...' : floorName(S.dug) + ' EXCAVATED')); sfx(w ? 'bad' : 'build'); if (!w) { S.cur = S.digH ? S.dug - 1 : S.dug; follow(); save(); } closeMenu(); } }] : []),
    { label: 'TOWER STATUS', fn: () => { S.mode = 'status'; S.menu = null; } },
    { label: 'NEWS', fn: () => { S.mode = 'news'; S.menu = null; } },
    { label: 'GO TO G/F', fn: () => { S.cur = C.BASE; follow(); closeMenu(); } },
    { label: 'GO TO TOP', fn: () => { S.cur = topFloor(); follow(); closeMenu(); } },
    ...(S.powerH ? [{ label: 'REPAIR POWER', sub: '$' + kfmt(repairPowerCost()), fn: () => { const w = repairPower(); note(w || 'POWER ON'); sfx(w ? 'bad' : 'fanfare'); closeMenu(); } }] : []),
    ...(S.brokenSi >= 0 ? [{ label: 'REPAIR LIFT', sub: '$' + kfmt(repairLiftCost()), fn: () => { const w = repairLift(); note(w || 'LIFT FIXED'); sfx(w ? 'bad' : 'fanfare'); closeMenu(); } }] : []),
    { label: 'SAVE 1  ' + slotInfo(0), fn: () => { saveSlot(0); note('SAVED TO SLOT 1'); closeMenu(); } },
    { label: 'SAVE 2  ' + slotInfo(1), fn: () => { saveSlot(1); note('SAVED TO SLOT 2'); closeMenu(); } },
    { label: 'LOAD 1  ' + slotInfo(0), dis: slotInfo(0) === 'EMPTY', fn: () => { if (loadSlot(0)) { note('LOADED SLOT 1'); closeMenu(); } else note('EMPTY SLOT'); } },
    { label: 'LOAD 2  ' + slotInfo(1), dis: slotInfo(1) === 'EMPTY', fn: () => { if (loadSlot(1)) { note('LOADED SLOT 2'); closeMenu(); } else note('EMPTY SLOT'); } },
    { label: 'TITLE', fn: () => { closeMenu('title'); titleMenu(); } });
  openMenu(floorName(S.cur) + '/F  ' + dateStr(), items);
}
function titleMenu() {
  openMenu('SKYRISE', [
    { label: 'LOAD 1  ' + slotInfo(0), dis: slotInfo(0) === 'EMPTY', fn: () => { if (loadSlot(0)) { S.mode = 'play'; note('LOADED SLOT 1'); } else note('EMPTY'); } },
    { label: 'LOAD 2  ' + slotInfo(1), dis: slotInfo(1) === 'EMPTY', fn: () => { if (loadSlot(1)) { S.mode = 'play'; note('LOADED SLOT 2'); } else note('EMPTY'); } },
    { label: 'NEW TOWER', fn: () => { newGame(); S.mode = 'play'; note('NEW TOWER'); sfx('fanfare'); } },
  ]);
}
function speedShop() {
  const it = [2, 3].map((i) => ({ label: (S.owned[i] ? '[OWNED] ' : S.star < C.SPEED_STAR[i] ? '\u2605' + C.SPEED_STAR[i] + ' ' : '') + ['', '', '2X', '4X'][i] + ' CLOCK', sub: S.owned[i] ? '' : '$' + kfmt(C.SPEED_PRICE[i]), dis: !!S.owned[i] || S.star < C.SPEED_STAR[i],
    fn: () => { const w = buySpeed(i); note(w || ['', '', '2X', '4X'][i] + ' CLOCK BOUGHT (IN-GAME $)'); sfx(w ? 'bad' : 'fanfare'); if (!w) S.speed = i; closeMenu(); } }));
  it.push({ label: 'BACK', fn: () => closeMenu() });
  openMenu('SPEED SHOP (IN-GAME MONEY ONLY)', it);
}
function buildMenu() {
  const items = [];
  for (const t of BUILD_ORDER) {
    const d = T[t], lock = d.star > S.star || (d.dmin && depth(S.dug) < d.dmin);
    if (d.dmin && !S.deep) continue;   // underground sights appear with the challenge
    const vs = t === 'shop' ? SHOPS.map((v, i) => i) : t === 'carpark' ? [2, 3, 4] : [undefined];
    for (const v of vs) items.push({ label: (lock ? (d.dmin ? 'B' + d.dmin + ' ' : '\u2605' + d.star + ' ') : '') + (v === undefined ? d.zh + ' ' + d.en : t === 'carpark' ? d.zh + ' PARKING ' + v + 'F' : 'SHOP: ' + SHOPS[v].en), sub: '$' + kfmt(d.shaft ? d.cost + d.car : d.cost), dis: lock, t, v, fn: () => startPlace(t, v) });
  }
  openMenu('BUILD ON ' + floorName(S.cur) + '/F', items, Math.max(0, items.findIndex((i) => i.t === S.last && (S.lastV === undefined || i.v === undefined || i.v === S.lastV))));
}
function topFloor() { let t = C.BASE; for (let f = 0; f < C.NF * C.W; f++) if (S.struct[f]) t = Math.max(t, Math.floor(f / C.W)); return t; }
function follow() { if (S.cur < S.cam) S.cam = S.cur; if (S.cur > S.cam + C.VIS - 1) S.cam = S.cur - C.VIS + 1; S.cam = Math.max(0, Math.min(C.NF - C.VIS, S.cam)); }
function startPlace(t, v) {
  S.last = t; S.lastV = v; closeMenu('place');
  const d = T[t];
  let f = S.cur;
  if (d.where === 'top') f = C.NF - d.h;
  if (d.where === 'metro') f = 0;
  if (d.where === 'lobby' && !isLobbyFloor(f)) f = C.BASE;
  S.cur = f; follow();
  S.place = { t, v, stage: 'x', x: 0, top: f };
  S.place.x = findSpot(t, f, S.x);
}
function findSpot(t, f, from) {
  for (let k = 0; k < C.W; k++) for (const x of [from + k, from - k]) if (x >= 0 && x + T[t].w <= C.W && !canPlace(t, f, x, undefined, S.place && S.place.v)) return x;
  return Math.max(0, Math.min(C.W - T[t].w, from));
}
function startInspect() {
  const list = floorThings(S.cur);
  if (!list.length) { note('NOTHING ON ' + floorName(S.cur) + '/F'); closeMenu(); return; }
  closeMenu('inspect'); S.insp = { i: 0, list };
}
function floorThings(f) {
  const l = [];
  for (const u of S.units) if (f >= u.f && f < u.f + u.h) l.push({ u });
  S.shafts.forEach((s, i) => { if (f >= s.f0 && f <= s.f1) l.push({ s: i, x: s.x }); });
  return l.sort((a, b) => (a.u ? a.u.x : a.x) - (b.u ? b.u.x : b.x));
}
function unitMenu(it) {
  if (it.s !== undefined) {
    const s = S.shafts[it.s];
    const fin = s.build ? [{ label: 'FINISH NOW', sub: '$' + kfmt(Math.round(T[s.t].cost * C.RUSH_PCT / 100 * s.build / s.build0)), fn: () => { const w = finishNow(s); note(w || 'FINISHED'); sfx(w ? 'bad' : 'build'); closeMenu('inspect'); } }] : [];
    openMenu(T[s.t].en + ' ' + floorName(s.f0) + '-' + floorName(s.f1) + ' CARS ' + s.cars.length + '/' + C.MAX_CARS, [...fin,
      { label: 'ADD CAR', sub: '$' + kfmt(T[s.t].car), fn: () => { const w = addCar(it.s); note(w || 'CAR ADDED'); sfx(w ? 'bad' : 'build'); closeMenu('inspect'); } },
      { label: 'BACK', fn: () => closeMenu('inspect') }]);
    return;
  }
  const u = it.u, d = T[u.t];
  const items = [];
  if (u.build) items.push({ label: 'FINISH NOW', sub: '$' + kfmt(Math.max(1000, Math.round(d.cost * C.RUSH_PCT / 100 * u.build / u.build0))), fn: () => { const w = finishNow(u); note(w || 'FINISHED'); sfx(w ? 'bad' : 'build'); closeMenu('inspect'); } });
  if (TENANT(u.t)) items.push({ label: 'RENT: ' + ['LOW', 'NORMAL', 'HIGH'][u.rent], fn: () => { u.rent = (u.rent + 1) % 3; unitMenu(it); } });
  items.push({ label: 'DEMOLISH', sub: '$' + kfmt(d.cost / 10), fn: () => openMenu('DEMOLISH ' + d.en + '?', [
    { label: 'NO', fn: () => closeMenu('inspect') },
    { label: 'YES, DEMOLISH', fn: () => { const w = demolish(u); note(w || 'DEMOLISHED'); sfx(w ? 'bad' : 'ok'); S.insp.list = floorThings(S.cur); S.insp.i = 0; closeMenu(S.insp.list.length ? 'inspect' : 'play'); } }]) });
  items.push({ label: 'BACK', fn: () => closeMenu('inspect') });
  openMenu(u.t === 'shop' ? 'SHOP: ' + SHOPS[(u.v || 0) % 6].en : d.zh + ' ' + d.en, items);
}
function act(k) {
  switch (S.mode) {
    case 'title':
      titleMenu();
      return;
    case 'play':
      if (k === 'up') { S.cur = Math.min(C.NF - 1, S.cur + 1); follow(); }
      else if (k === 'down') { S.cur = Math.max(0, S.dug - 1, Math.min(S.cur, S.cur - 1)); follow(); }   // one floor of rock below the dug floor
      else if (k === 'ok') mainMenu();
      else if (k === 'back') { S.speed = S.speed ? 0 : 1; note(S.speed ? 'RUNNING' : 'PAUSED'); }
      return;
    case 'menu': {
      const m = S.menu;
      if (k === 'up') m.sel = (m.sel + m.items.length - 1) % m.items.length;
      else if (k === 'down') m.sel = (m.sel + 1) % m.items.length;
      else if (k === 'ok') { const it = m.items[m.sel]; if (it.dis) { sfx('bad'); note('LOCKED: ' + it.label.split(' ')[0]); } else it.fn(); }
      else if (k === 'back') closeMenu(m.prev === 'inspect' ? 'inspect' : 'play');
      return;
    }
    case 'place': {
      const p = S.place, d = T[p.t];
      if (p.stage === 'x') {
        if (k === 'up') p.x = Math.max(0, p.x - 1);
        else if (k === 'down') p.x = Math.min(C.W - d.w, p.x + 1);
        else if (k === 'ok') {
          if (d.shaft) { const w = canPlace(p.t, S.cur, p.x); if (w) { note(w); sfx('bad'); } else { p.stage = 'top'; p.top = S.cur; p.f0 = S.cur; } }
          else { const w = place(p.t, S.cur, p.x, undefined, p.v); if (w) { note(w); sfx('bad'); } else { sfx('build'); S.x = p.x; p.x = findSpot(p.t, S.cur, p.x + d.w); } }
        } else if (k === 'back') { S.x = p.x; S.place = null; S.mode = 'play'; }
      } else {
        if (k === 'up') { p.top = Math.min(C.NF - 1, p.top + 1); S.cur = p.top; follow(); }
        else if (k === 'down') { p.top = Math.max(p.top - 1, 0); S.cur = p.top; follow(); }
        else if (k === 'ok') {
          const f0 = Math.min(p.f0, p.top), f1 = Math.max(p.f0, p.top);
          const w = place(p.t, f0, p.x, f1); if (w) { note(w); sfx('bad'); } else { sfx('build'); note(d.en + ' ' + floorName(f0) + '-' + floorName(f1) + '/F BUILT'); S.place = null; S.mode = 'play'; }
        } else if (k === 'back') p.stage = 'x';
      }
      return;
    }
    case 'inspect': {
      const I = S.insp;
      if (k === 'up') I.i = (I.i + I.list.length - 1) % I.list.length;
      else if (k === 'down') I.i = (I.i + 1) % I.list.length;
      else if (k === 'ok') unitMenu(I.list[I.i]);
      else if (k === 'back') { S.insp = null; S.mode = 'play'; }
      return;
    }
    case 'status': case 'news':
      if (k === 'ok' || k === 'back') S.mode = 'play';
      return;
    case 'end':
      if (S.endKind === 1 && !S.deep && (k === 'up' || k === 'down')) { S.endSel ^= 1; sfx('tick'); }
      else if (k === 'ok') {
        S.mode = 'play';
        if (S.endKind === 1 && !S.deep && S.endSel === 0) { S.deep = true; S.cur = C.BASE - 4; follow(); note('UNDERGROUND CHALLENGE: DIG TO B100!'); save(); }
        else note(S.endKind === 2 ? 'CORE RATING - THE DEEPEST TOWER ON EARTH' : 'KEEP BUILDING - THE TOWER IS YOURS');
      }
      return;
  }
}
const kfmt = (n) => n >= 1e6 ? (n / 1e6).toFixed(n % 1e6 ? 1 : 0) + 'M' : n >= 1000 || n <= -1000 ? Math.round(n / 1000) + 'K' : String(Math.round(n));
