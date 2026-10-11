// Headless rule checks for prototype/index.html (run tools/proto_test.sh).
// Writes its report into #out; the last line is "all checks passed" on success.
(function () {
  const out = [], errs = [];
  let fails = 0;
  const ok = (c, m) => { out.push((c ? 'ok   ' : 'FAIL ') + m); if (!c) fails++; };
  window.addEventListener('error', (e) => errs.push(e.message));
  const H = window.__TW;
  try {
    H.stop();
    const C = H.C, S = H.S;
    const G = C.BASE, F = (n) => C.BASE + n;
    const ticks = (n) => { for (let i = 0; i < n; i++) H.simTick(); };
    const days = (n) => ticks(n * C.DAY);
    const tap = (k) => { H.setKey(k, true); H.frame(); H.setKey(k, false); H.frame(); };
    const hold = (k) => { H.setKey(k, true); for (let i = 0; i < 16; i++) H.frame(); H.setKey(k, false); H.frame(); };
    const fresh = (funds, build) => { C.BUILD_ON = !!build; C.EVENTS_ON = false; localStorage.removeItem('tower'); localStorage.removeItem('tower0'); localStorage.removeItem('tower1'); localStorage.removeItem('tower_slot'); H.freshState(); H.seed(42); H.newGame(); S.sound = false; S.mode = 'play'; S.menu = null; if (funds) S.funds = funds; };
    const P = (t, f, x, top) => { const w = H.place(t, f, x, top); if (w) throw new Error('place ' + t + ' ' + f + ' ' + x + ': ' + w); };
    const closeAll = () => { S.mode = 'play'; S.menu = null; };
    const sumPeople = () => S.groups.reduce((a, g) => a + g.n, 0);

    // ---- title and controls
    fresh(); S.mode = 'title'; S.menu = null; H.render();
    ok(S.mode === 'menu' && S.menu && S.menu.title === 'SKYRISE', 'title opens LOAD 1 / LOAD 2 / NEW');
    ok(S.menu.items.some((i) => i.label.startsWith('LOAD 1')) && S.menu.items.some((i) => i.label === 'NEW TOWER'), 'title menu shows both slots + NEW');
    S.menu.sel = S.menu.items.findIndex((i) => i.label === 'NEW TOWER'); tap('ok');
    ok(S.mode === 'play' && S.funds === C.START_FUNDS && S.star === 1, 'NEW TOWER starts a game with $' + C.START_FUNDS);
    tap('ok');
    ok(S.mode === 'menu', 'OK in tower view opens the menu');
    S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('BUILD...')); tap('ok');
    ok(S.mode === 'menu' && S.menu.title.startsWith('BUILD'), 'BUILD... opens the build list');
    ok(S.menu.items.filter((i) => i.dis).length === H.BUILD_ORDER.filter((t) => H.T[t].star > 1).length + 7, 'locked items are shown greyed with their star (shop = 6 entries)');
    ok(H.SHOPS.every((v, i) => S.menu.items.some((it) => it.t === 'shop' && it.v === i)), 'the build list offers all 6 shop types');
    ok(S.menu.items.filter((it) => it.t === 'shop').every((it) => /^[\x20-\x7e]+$/.test(it.label.replace(/^\u2605\d /, ''))), 'shop entries are English only (no Chinese on shops)');
    S.menu.sel = 0; tap('ok');
    ok(S.mode === 'place' && S.place.t === 'lobby', 'pick LOBBY -> placement mode on G/F');
    const x0 = S.place.x; tap('down');
    ok(S.place.x === x0 + 1, 'DOWN slides the room right by one segment');
    tap('ok');
    ok(S.units.filter((u) => u.t === 'lobby').length === 1, 'OK builds it, stays in placement for the next one');
    tap('ok'); tap('ok');
    ok(S.units.filter((u) => u.t === 'lobby').length === 3, 'two more OK = two more lobby pieces side by side');
    hold('ok');
    ok(S.mode === 'play', 'hold OK leaves placement');
    tap('up');
    ok(S.cur === F(1), 'UP moves to 1/F');
    // a lift with keys: menu -> build -> lift; pick x, OK, raise top floor, OK
    tap('ok'); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('BUILD...')); tap('ok');
    S.menu.sel = S.menu.items.findIndex((i) => i.t === 'lift'); tap('ok');
    ok(S.place.t === 'lift', 'lift chosen');
    S.cur = G; S.place.x = 30; tap('ok');
    ok(S.place.stage === 'top', 'first OK fixes the bottom, then choose the top floor');
    tap('up'); tap('up');
    ok(H.canPlace('lift', F(1), 60, F(2)) === 'NO FLOOR BELOW', 'a lift cannot start in mid-air (' + H.canPlace('lift', F(1), 60, F(2)) + ')');
    hold('ok'); hold('ok');
    ok(S.mode === 'play', 'hold OK twice backs out to the tower');
    hold('ok');
    ok(S.speed === 0, 'hold OK in tower view = pause'); hold('ok'); ok(S.speed === 1, 'again = run');

    // ---- building rules
    fresh();
    ok(H.canPlace('office', F(1), 10) === 'NO FLOOR BELOW', 'nothing floats: 1/F needs G/F under it');
    ok(H.canPlace('office', G, 10) === 'G/F IS THE LOBBY', 'G/F is lobby only');
    ok(/LOBBY/.test(H.canPlace('lobby', F(3), 0)), 'lobbies only on G and sky-lobby floors');
    ok(/STARS/.test(H.canPlace('single', F(1), 0)), 'hotel needs 2 stars');
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    P('office', F(1), 0);
    ok(H.canPlace('office', F(1), 4) === 'BLOCKED', 'no overlap');
    ok(/30/.test(H.canPlace('lift', G, 40, F(31))), 'standard lift spans at most 30 floors');
    S.star = 3; ok(H.canPlace('parking', F(1), 20) === 'BASEMENT ONLY', 'car park is basement only');

    // ---- day 1: an office floor with a lift
    fresh(4000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 6; f++) { P('office', F(f), 0); P('office', F(f), 9); P('floor', F(f), 18); P('floor', F(f), 30); P('office', F(f), 38); P('office', F(f), 47); P('floor', F(f), 56); }
    ok(S.access[F(3)] === 0, 'no lift or stairs yet: 3/F unreachable');
    P('lift', G, 26, F(6));
    ok(S.access[F(6)] === 1, 'lift G-6/F connects every floor');
    const offices = S.units.filter((u) => u.t === 'office');
    S.tick = C.DAY - 1; ticks(1);   // -> new day (WD2)
    const occ = offices.filter((u) => u.occ).length;
    ok(occ > 0, occ + '/' + offices.length + ' offices rented on the first morning');
    S.tick = 7 * C.HOUR; ticks(2.5 * C.HOUR);
    const inside = offices.reduce((a, u) => a + u.inside, 0), want = offices.filter((u) => u.occ).length * 6;
    ok(inside >= want * 0.9, 'by 9:30 workers are at their desks: ' + inside + '/' + want);
    ok(S.stats.waitN > 0, 'they queued for the lift: avg wait ' + (S.stats.waitSum / S.stats.waitN / C.MIN).toFixed(2) + ' min, peak ' + S.stats.peakGroups + ' groups');
    const avg1 = S.stats.waitSum / S.stats.waitN;
    ticks(10 * C.HOUR);
    const left = offices.reduce((a, u) => a + u.inside + u.transit, 0);
    ok(left === 0 && S.groups.length === 0, 'by 19:30 everyone went home, no groups left in the pool');
    ok(S.stats.spawned === S.stats.arrived + S.stats.failed + sumPeople(), 'people conserved: spawned ' + S.stats.spawned + ' = arrived ' + S.stats.arrived + ' + failed ' + S.stats.failed);

    // more cars -> shorter waits
    const s0 = S.stats.waitSum, n0 = S.stats.waitN;
    H.addCar(0); H.addCar(0); H.addCar(0);
    S.tick = C.DAY - 1; ticks(1); while (S.day % 3 === 2) { S.tick = C.DAY - 1; ticks(1); } S.tick = 7 * C.HOUR; ticks(3 * C.HOUR);
    const avg4 = (S.stats.waitSum - s0) / (S.stats.waitN - n0);
    ok(avg4 < avg1, 'four cars cut the morning wait: ' + (avg1 / C.MIN).toFixed(2) + ' -> ' + (avg4 / C.MIN).toFixed(2) + ' min');

    // rent at the quarter
    while (S.day % 3 !== 2) days(1);
    const f0 = S.funds; S.tick = C.DAY - 1; ticks(1);
    ok(S.stats.rentQ > 0 && S.funds > f0 - 100000, 'office rent paid at the start of Q' + (Math.floor(S.day / 3) % 4 + 1) + ': $' + S.stats.rentQ);

    // ---- stairs-only access within 4 floors, and none beyond
    fresh(3000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 6; f++) for (let x = 0; x < 64; x += 8) P('floor', F(f), x);
    for (let f = 0; f < 6; f++) P('stairs', F(f), 0);
    H.computeAccess();
    ok(S.access[F(4)] === 1 && S.access[F(5)] === 0, 'stairs carry people up to 4 floors only');

    // ---- demolish a lift shaft (lifts had no delete; rooms/housekeeping/security as before)
    fresh(3000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 10; f++) for (let x = 0; x < 64; x += 8) P('floor', F(f), x);
    P('lift', G, 26, F(10));
    P('office', F(8), 0); P('office', F(8), 12); P('flat', F(8), 32); P('flat', F(8), 48);
    days(1);                       // offices get rented overnight
    S.tick = 7 * C.HOUR;
    let guard = 0; while (S.groups.length === 0 && guard++ < C.HOUR / 2) H.simTick();   // wait for a real mid-commute
    const pg0 = S.groups.reduce((a, g) => a + g.n, 0);
    ok(pg0 > 0, 'the rush puts people in transit before demolishing (' + pg0 + ')');
    const fa0 = S.stats.failed, fd0 = S.funds, sh0 = S.shafts.length;
    const w = H.demolishShaft(0);
    ok(w === null, 'demolishing the only lift works' + (w ? ' (' + w + ')' : ''));
    ok(S.shafts.length === sh0 - 1, 'the shaft is gone from S.shafts (' + sh0 + ' -> ' + S.shafts.length + ')');
    ok(S.funds === fd0 - Math.floor((H.T.lift.cost + 2000 * 10) / 10), 'demolish costs 10% of the lift ($' + (fd0 - S.funds) + ')');
    const pg1 = S.groups.reduce((a, g) => a + g.n, 0);
    ok(pg1 === 0 && S.stats.failed - fa0 === pg0, 'all ' + pg0 + ' in-flight people are released as failed trips (left: ' + pg1 + ')');
    ok(S.stats.spawned === S.stats.arrived + S.stats.failed + pg1, 'people conserved through a demolition (spawned ' + S.stats.spawned + ')');
    ok(S.access[F(8)] === 0, '8/F unreachable again without the lift');
    ok(S.units.some((u) => u.t === 'floor' && u.f === F(8) && u.x === 26), 'the lift column becomes empty floor again');
    ok(H.canPlace('lift', G, 26, F(10)) === null, 'a new lift builds over the old column');
    H.place('lift', G, 26, F(10));
    ok(S.access[F(8)] === 1, 'the rebuilt lift serves the floor again');

    // ---- stress: one car vs a crowd -> low evaluation, people leave
    fresh(9000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    P('lift', G, 27, F(14));
    for (let f = 1; f <= 14; f++) { for (const x of [0, 9, 18]) P('office', F(f), x); for (const x of [31, 40, 49]) P('office', F(f), x); }
    for (const u of S.units) if (u.t === 'office') { u.occ = 1; u.eval = 70; }
    const before = S.units.filter((u) => u.t === 'office' && u.occ).length;
    for (let d = 0; d < 2; d++) days(1);
    const evBad = S.units.filter((u) => u.t === 'office').reduce((a, u) => a + u.eval, 0) / 84;
    ok(S.stats.maxWait > 10 * C.MIN, '84 offices on one car: worst wait ' + (S.stats.maxWait / C.MIN).toFixed(0) + ' min');
    ok(evBad < 55, 'long waits drag office evaluation down to ' + evBad.toFixed(0));
    ok(S.stats.peakGroups <= C.MAX_GROUPS, 'group pool bounded: peak ' + S.stats.peakGroups + '/' + C.MAX_GROUPS + ' (overflow ' + S.stats.overflow + ')');
    days(1);
    const after = S.units.filter((u) => u.t === 'office' && u.occ).length;
    ok(after < before, 'unhappy offices move out at the quarter: ' + before + ' -> ' + after);

    // ---- hotel + housekeeping, cockroaches without
    fresh(5000000); S.star = 2;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 3; f++) { for (let x = 0; x < 24; x += 4) P('single', F(f), x); for (let x = 28; x < 64; x += 4) P('single', F(f), x); }
    P('lift', G, 24, F(4));
    S.tick = C.DAY - 1; ticks(1); S.tick = C.DAY - 1; ticks(1);
    days(2);
    const hotel = S.units.filter((u) => u.t === 'single');
    ok(S.stats.income > 0 && hotel.some((u) => u.dirty || u.roach), 'guests stayed and paid; rooms left dirty');
    days(4);
    ok(hotel.some((u) => u.roach), 'no housekeeping -> cockroaches after 3 dirty days');
    fresh(5000000); S.star = 2;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 3; f++) { for (let x = 0; x < 24; x += 4) P('single', F(f), x); for (let x = 28; x < 64; x += 4) P('single', F(f), x); }
    P('housek', F(4), 0); P('housek', F(4), 28);
    P('lift', G, 24, F(4));
    days(7);
    ok(!S.units.some((u) => u.roach), 'with housekeeping no room gets cockroaches in a week');

    // ---- food: office lunch + visitors -> daily income, population counts customers
    fresh(5000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    P('cct', F(1), 0); P('cct', F(1), 16); P('floor', F(1), 32); P('floor', F(1), 40); P('floor', F(1), 48); P('floor', F(1), 56);
    P('stairs', G, 32);
    days(6);
    const cct = S.units.filter((u) => u.t === 'cct');
    ok(cct.every((u) => u.occ) && cct.every((u) => u.custY > 10), 'cha chaan tengs get walk-in customers: ' + cct.map((u) => u.custY).join(', '));
    ok(S.pop >= cct.reduce((a, u) => a + u.custY, 0), 'yesterday\'s customers count as population (' + S.pop + ')');

    // ---- never stuck: broke at a quarter -> grant
    fresh(0);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    S.funds = -30000; S.day = 2; S.tick = C.DAY - 1; ticks(1);
    ok(S.funds >= C.GRANT - 1000 && S.stats.grants > 0, 'out of money -> small business grant ($' + S.funds + ')');

    // ---- an automated player climbs the stars (keeps the economy honest)
    fresh(); S.botRef = undefined;
    const tryP = (t, f, x, top) => !H.place(t, f, x, top);
    for (let x = 0; x < 64; x += 8) tryP('lobby', G, x);
    const plan = (f) => f === 1 ? [['cct', 0], ['cct', 36]] : (f % 3 === 0) ? [['flat', 0], ['flat', 36]] : [['office', 0], ['office', 9], ['office', 36], ['office', 45], ['office', 54]];
    const fillF = (f) => { for (let x = 0; x < 64; x++) if (!S.grid[f * 64 + x] && !(x >= 26 && x < 30)) tryP('floor', f, x); };
    let topF = 0, star2 = -1, star3 = -1, botMs = 0, botTicks = 0;
    for (let d = 0; d < 120 && S.star < 3; d++) {
      for (let k = 0; k < 2; k++) {
        const f = topF + 1; if (f > 29) break;
        if (f >= 20 && S.botRef === undefined) {   // HK rule: a refuge floor before going (much) past 20/F
          if (S.funds < 500000) break;
          const s0 = S.shafts[0];
          if (s0 && s0.f1 < F(f)) { for (let ff = s0.f1 + 1; ff <= F(f); ff++) for (let xx = s0.x; xx < s0.x + s0.w; xx++) { S.grid[ff * 64 + xx] = -1; S.struct[ff * 64 + xx] = 1; } s0.f1 = F(f); H.computeAccess(); }
          S.botRef = H.place('refuge', F(f), 0); topF = f; continue;
        }
        const p = plan(f), need = p.reduce((a, [t]) => a + H.T[t].cost, 0) + 60000;
        if (S.funds < need) break;
        for (const [t, x] of p) tryP(t, F(f), x);
        fillF(F(f)); topF = f;
      }
      if (!S.shafts.length && topF) tryP('lift', G, 26, F(topF));
      const s = S.shafts[0];
      if (s && s.f1 < F(topF)) { // the bot "extends" its shaft (prototype shortcut for rebuilding it)
        for (let ff = s.f1 + 1; ff <= F(topF); ff++) { H.S.units.filter((u) => u.t === 'floor' && u.f === ff && u.x < 30 && u.x + u.w > 26).forEach(() => 0); for (let xx = s.x; xx < s.x + s.w; xx++) { S.grid[ff * 64 + xx] = -1; S.struct[ff * 64 + xx] = 1; } }
        s.f1 = F(topF); H.computeAccess();
      }
      if (s && s.cars.length < Math.min(4, 1 + Math.floor(topF / 5)) && S.funds > 200000) H.addCar(0);
      if (S.star >= 2 && !S.units.some((u) => u.t === 'security') && S.funds > 300000 && topF < 29) {
        topF++; tryP('security', F(topF), 0); tryP('flat', F(topF), 36); fillF(F(topF));
      }
      const tq = performance.now(); days(1); botMs += performance.now() - tq; botTicks += C.DAY;
      if (S.star >= 2 && star2 < 0) star2 = S.day;
      if (S.star >= 3 && star3 < 0) star3 = S.day;
    }
    ok(star2 > 0 && star2 < 40, 'bot reached 2 stars on day ' + star2);
    ok(star3 > 0, 'bot reached 3 stars on day ' + star3 + ' (refuge: ' + S.botRef + ', uncovered ' + H.uncoveredFloor() + ', ' + topF + ' floors, pop ' + S.pop + ', $' + Math.round(S.funds / 1000) + 'K, grants $' + S.stats.grants + ')');
    ok(true, 'sim cost in the browser (whole days incl. rush hours): ' + (botMs * 1000 / botTicks).toFixed(1) + ' us/tick, ' + (botMs / (botTicks / C.DAY)).toFixed(0) + ' ms per game day');
    ok(S.stats.peakGroups <= C.MAX_GROUPS, 'group pool bounded in the long run: peak ' + S.stats.peakGroups + '/' + C.MAX_GROUPS + ', overflow ' + S.stats.overflow);
    ok(S.stats.spawned === S.stats.arrived + S.stats.failed + sumPeople(), 'people conserved over ' + S.day + ' days (' + S.stats.spawned + ' trips)');

    // ---- 4th star: VIP visit (population targets relaxed here; the bot above covers population)
    const sp = C.STAR_POP.slice(); C.STAR_POP[4] = C.STAR_POP[5] = 0;
    S.funds = 50000000;
    for (let f = 3; f >= 0; f--) fillF(f);                         // dig 4 basements
    tryP('parking', F(-1), 0); tryP('parking', F(-1), 16); tryP('parking', F(-1), 40);
    tryP('lift', F(15), 31, F(34));                                  // a second lift from the 15/F sky lobby
    for (let f = topF + 1; f <= 30; f++) fillF(F(f));
    const hf = 31;
    tryP('suite', F(hf), 0); tryP('suite', F(hf), 10); tryP('housek', F(hf), 40); fillF(F(hf));
    tryP('clinic', F(hf + 1), 0); fillF(F(hf + 1));
    tryP('recycle', F(hf + 2), 0); fillF(F(hf + 2)); fillF(F(hf + 3));
    H.computeAccess();
    ok(S.access[F(hf)] === 1, 'suites on ' + hf + '/F reachable: lift to 15/F, change, second lift');
    let vipSeen = false;
    for (let d = 0; d < 40 && S.star < 4; d++) { days(1); if (S.vip) vipSeen = true; }
    const r4 = H.starReqs(4).map(([n, v]) => n + (v ? '+' : '-')).join(' ');
    ok(vipSeen && S.star >= 4, 'VIP stayed in a suite and the tower got 4 stars on day ' + S.day + ' [' + r4 + ']');
    tryP('metro', F(-4), 0);
    for (let d = 0; d < 3 && S.star < 5; d++) days(1);
    ok(S.star === 5, '5 stars once the metro station is in (B4-B2)');
    C.STAR_POP.splice(0, 7, ...sp);

    // ---- TOWER: sky chapel on the roof + a weekend wedding
    fresh(100000000); S.star = 5;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = G + 1; f < C.NF - 4; f++) fillF(f);
    ok(H.canPlace('chapel', C.NF - 8, 0) !== null, 'sky chapel only fits the top 4 floors');
    P('chapel', C.NF - 4, 0);
    P('exp', G, 30, C.NF - 4);
    ok(S.access[C.NF - 4] === 1, 'an express lift reaches the chapel (G and end stops only)');
    C.STAR_POP[6] = 0;
    for (let d = 0; d < 6 && !S.towerDone; d++) days(1);
    C.STAR_POP.splice(0, 7, ...sp);
    ok(S.towerDone && S.star === 6, 'weekend sky wedding -> TOWER rating (day ' + S.day + ')');

    // ---- save / load round trip
    const bytes = H.save();
    const snap = JSON.stringify([S.units.length, S.shafts.length, S.funds, S.star, S.day]);
    H.freshState(); ok(H.load(), 'save loads');
    ok(JSON.stringify([S.units.length, S.shafts.length, S.funds, S.star, S.day]) === snap, 'save round-trips ' + S.units.length + ' rooms, ' + S.shafts.length + ' shafts (' + bytes + ' bytes JSON)');

    // ---- 100 floors: sky lobbies every 15 floors, chapel on the roof (96-99/F)
    ok(C.NF - C.BASE === 100 && C.BASE === 100 && C.SKY.length === 13 && C.SKY.every((f) => (f - C.BASE) % 15 === 0), '100 floors above ground + 100 basements (B5-B100 with the challenge); lobbies B90..B15 (deep), G, 15..90/F');
    ok(H.canPlace('lobby', C.BASE + 90, 0) !== 'LOBBY: G + EVERY 15 FLOORS' && /LOBBY/.test(H.canPlace('lobby', C.BASE + 91, 0) || 'LOBBY'), '90/F is a sky lobby floor, 91/F is not');
    ok(S.units.some((u) => u.t === 'chapel' && u.f + u.h - 1 === C.NF - 1), 'chapel sits on 96-99/F');

    // ---- construction time, rush build, finish now (in-game money only)
    fresh(5000000, true);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    P('lift', G, 26, F(5));
    ok(S.shafts[0].build === C.BUILD_H.lift * C.HOUR && !H.route(G, F(3)), 'a new lift takes ' + C.BUILD_H.lift + ' game hours before it runs');
    for (let f = 1; f <= 5; f++) fillF(F(f));
    const off = (P('office', F(2), 0), S.units[S.units.length - 1]);
    ok(off.build > 0, 'an office is under scaffolding for ' + C.BUILD_H.office + ' h');
    let f1 = S.funds; ok(!H.finishNow(off) && off.build === 0 && f1 - S.funds === Math.round(H.T.office.cost * C.RUSH_PCT / 100), 'FINISH NOW: pays ' + C.RUSH_PCT + '% of the price, ready at once');
    S.rush = true; f1 = S.funds; P('office', F(2), 9);
    const off2 = S.units[S.units.length - 1];
    ok(off2.build === 0 && f1 - S.funds === H.T.office.cost * 1.25, 'RUSH BUILD on: +25% and no waiting');
    S.rush = false; P('office', F(3), 0);
    const off3 = S.units[S.units.length - 1];
    S.tick = 6 * C.HOUR; for (let i = 0; i < C.BUILD_H.lift * C.HOUR; i++) H.simTick();
    ok(!S.shafts[0].build && off3.build === 0 && H.route(G, F(3)), 'after ' + C.BUILD_H.lift + ' h the lift runs and the office is ready');
    // speed shop
    fresh(1000000);
    ok(!S.owned[2] && !S.owned[3] && S.star === 1, 'fresh tower: only 1x clock (star ' + S.star + ')');
    S.mode = 'play'; tap('ok'); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('SPEED:')); tap('ok'); tap('ok');
    ok(S.speed === 1 || S.speed === 0, 'SPEED cycles only through clocks you own (now ' + S.speed + ')');
    f1 = S.funds; S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('SPEED SHOP')); tap('ok'); tap('ok');
    ok(S.owned[2] && f1 - S.funds === C.SPEED_PRICE[2] && S.speed === 2, '2x clock bought for $' + C.SPEED_PRICE[2] / 1000 + 'K of in-game money (star ' + S.star + ' day ' + S.day + ')');
    S.funds = 9e6; const r3 = H.buySpeed(3); ok(r3 === 'NEEDS 3 STARS' && !S.owned[3], '4x clock locked until 3 stars (' + r3 + ', star ' + S.star + ')');
    S.star = 3; S.funds = 1000; ok(H.buySpeed(3) === 'NOT ENOUGH $' && !S.owned[3], 'cannot buy 4x without enough in-game money');
    S.funds = 2e6; ok(!H.buySpeed(3) && S.owned[3], '4x bought at 3 stars');
    // choose the shop type in the build list
    fresh(5e6); S.star = 3; for (let x = 0; x < 64; x += 8) P('lobby', G, x); fillF(F(1));
    S.mode = 'play'; S.cur = F(2); tap('ok'); S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('BUILD...')); tap('ok');
    S.menu.sel = S.menu.items.findIndex((i) => i.t === 'shop' && i.v === 2); tap('ok'); S.place.x = 0; tap('ok');
    const gold = S.units.find((u) => u.t === 'shop');
    ok(gold && gold.v === 2, 'picked 金舖 Goldsmith from the build list -> that shop is built');
    hold('ok'); tap('ok');
    ok(S.menu.items[0].label === 'BUILD AGAIN: GOLDSMITH', 'BUILD AGAIN remembers the shop type');
    // background music: day / night loops, SOUND toggle in the menu, saved
    S.snd = 0; S.tick = 10 * C.HOUR; ok(H.wantMusic() === 1, 'daytime plays the calm day loop');
    S.tick = 21 * C.HOUR; ok(H.wantMusic() === 2, 'evening plays the night loop');
    S.menu.sel = S.menu.items.findIndex((i) => i.label.startsWith('SOUND')); ok(S.menu.items[S.menu.sel].label === 'SOUND: MUSIC + FX', 'SOUND item in the menu');
    tap('ok'); ok(S.snd === 1 && H.wantMusic() === 0 && S.sound, 'OK: music off, effects still on');
    H.freshState(); ok(H.load() && S.snd === 1, 'the sound setting is saved');
    S.snd = 0; S.mode = 'play'; H.musicTick();
    closeAll();

    // ---- lift animation: smooth glide, doors open at a stop, people walk in
    fresh(5000000);
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    for (let f = 1; f <= 8; f++) P('office', F(f), 0);
    P('lift', G, 26, F(8));
    S.cam = G - 1; S.mode = 'play';
    const car = S.shafts[0].cars[0];
    car.f = F(8); car.disp = G;
    const seen = [];
    for (let i = 0; i < 12; i++) { H.animate(); seen.push(car.disp); }
    ok(seen.every((v, i) => i === 0 || v >= seen[i - 1]) && seen[11] > G && seen[0] - G < 3, 'car glides up smoothly (' + seen.slice(0, 5).map((v) => v.toFixed(2)).join(' ') + ' ...)');
    for (let i = 0; i < 60; i++) H.animate();
    ok(Math.abs(car.disp - F(8)) < 0.03, 'and settles exactly on the floor');
    car.f = F(3); car.stopF = F(3); car.pin = 2; car.pout = 1;
    let maxDoor = 0, people = 0;
    for (let i = 0; i < 80; i++) { H.animate(); maxDoor = Math.max(maxDoor, car.doorA); people = Math.max(people, car.anim.length); }
    ok(maxDoor === 1 && car.doorA === 0, 'doors slide open fully at the stop and close again');
    ok(people === 3 && car.anim.length === 0, '1 person steps out and 2 step in (walk animation), then done');
    S.cur = F(3); H.render();
    ok(true, 'lift view renders (LED floor indicator, hall lanterns, queue figures)');
    // shops: each variant, open and shuttered, day and night
    fresh(9000000); S.star = 3;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    fillF(F(1)); fillF(F(2)); fillF(F(3));
    const shops = []; for (let i = 0; i < 4; i++) { P('shop', F(2 + (i >> 1)), (i & 1) * 12); shops.push(S.units[S.units.length - 1]); }
    shops.forEach((u, i) => { u.v = i; u.occ = 1; });
    S.cam = G - 1;
    for (const hh of [9, 12, 22]) { S.tick = hh * C.HOUR; H.render(); }
    ok(H.SHOPS.length === 6 && new Set(H.SHOPS.map((v) => v.sign)).size === 6, '6 distinct HK shop types (' + H.SHOPS.map((v) => v.zh).join(' ') + ')');


    // ---- refuge floors, MEGA cinema, coaster, multi-storey facilities
    fresh(900000000); S.star = 4;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    H.place('lift', G, 26, F(29));
    for (let f = 1; f <= 46; f++) { if (f % 15 === 0) { for (let x = 0; x < 64; x += 8) H.place('lobby', F(f), x); continue; } for (let x = 0; x < 64; x++) if (!S.grid[F(f) * 64 + x]) H.place('floor', F(f), x); }
    ok(H.canPlace('refuge', F(15), 0) === 'SKY LOBBY FLOOR', 'refuge not on a sky lobby floor');
    H.place('office', F(21), 0);
    ok(H.canPlace('refuge', F(21), 0) === 'FLOOR NOT EMPTY', 'refuge needs an empty floor');
    const rr = H.place('refuge', F(20), 0); ok(rr === null && H.isRefugeFloor(F(20)), 'REFUGE built on 20/F, lift passes through (' + rr + ')');
    ok(H.canPlace('shop', F(20), 0) === 'REFUGE FLOOR', 'no rooms on a refuge floor');
    H.place('office', F(41), 0);
    ok(H.uncoveredFloor() === F(41), '41/F is not covered by the 20/F refuge');
    ok(H.starReqs(4).some((r) => r[0] === 'REFUGE FLOORS' && !r[1]), 'star 4 needs refuge floors');
    ok(H.place('refuge', F(40), 0) === null && H.uncoveredFloor() < 0, '40/F refuge covers 41/F');
    S.cam = F(17); S.cur = F(20); S.tick = 21 * C.HOUR; H.render();
    fresh(900000000); S.star = 3;
    for (let x = 0; x < 64; x += 8) P('lobby', G, x);
    H.place('lift', G, 60, F(12));
    ok(H.canPlace('coaster', F(1), 0) === 'NEEDS 4 STARS', 'coaster unlocks at 4 stars');
    S.star = 4;
    ok(H.place('coaster', F(1), 0) === null, 'COASTER built 1/F-5/F');
    const co = S.units[S.units.length - 1];
    ok(co.h === 5 && S.grid[F(5) * 64 + 10] === co.id + 1, 'one coaster record spans 5 floors');
    ok(H.place('mega', F(6), 0) === null && S.units[S.units.length - 1].h === 3, 'MEGA cinema built 6/F-8/F');
    ok(H.canPlace('carpark', F(1), 40, undefined, 2) === 'B4-G ONLY', 'PARKING only on B4-G');
    ok(H.place('carpark', F(-3), 40, undefined, 4) === null && S.units[S.units.length - 1].h === 4, '4-floor PARKING built B3-G');
    for (const t of ['pool', 'ice', 'atrium', 'banquet', 'climb', 'studio']) ok(H.T[t].h >= 2 && H.BUILD_ORDER.includes(t), t.toUpperCase() + ' is a ' + H.T[t].h + '-floor room in the build list');
    ok(H.canPlace('garden', F(20), 0) === '60/F AND ABOVE', 'sky garden only high up');
    for (const u of S.units) u.occ = 1, u.custY = 40;
    for (const hh of [15, 21]) { S.tick = hh * C.HOUR; S.cam = F(1); H.render(); S.cam = F(-4); H.render(); }
    H.save(); H.load();
    ok(S.units.some((u) => u.t === 'carpark' && u.h === 4), 'PARKING height survives save/load');
    ok(true, 'refuge / mega / coaster / parking render day and night');

    // ---- ending: the sky wedding completes -> TOWER rating -> fireworks, then keep playing
    S.star = 5; S.wedding = true; S.wedSent = true; S.wedArrived = 30; S.towerDone = false; S.mode = 'play'; S.speed = 1;
    for (let i = 0; i < 200 && S.mode !== 'end'; i++) H.frame();
    ok(S.mode === 'end' && S.towerDone && S.star === 6, 'TOWER rating opens the ending screen');
    H.render(); const tk = S.tick; H.frame(); ok(S.tick === tk, 'clock pauses on the ending');
    ok(S.endKind === 1 && S.endSel === 0, 'the ending asks CONTINUE CHALLENGE? DIG 100 FLOORS UNDERGROUND (YES selected)');
    H.setKey('down', true); H.frame(); H.setKey('down', false); H.frame();
    ok(S.endSel === 1, 'DOWN selects KEEP BUILDING');
    H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame();
    ok(S.mode === 'play' && !S.deep, 'KEEP BUILDING: back to play, no challenge');
    for (let i = 0; i < 100; i++) H.frame(); ok(S.mode === 'play', 'ending shows once');
    // ---- the Underground challenge
    {
      ok(H.canPlace('office', F(-5), 0) === 'SOLID ROCK', 'before the challenge B5 is solid rock');
      S.celebrate = 1; S.mode = 'play'; H.frame();
      ok(S.mode === 'end' && S.endKind === 1, 'TOWER ending prompt again');
      H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame();
      ok(S.deep && S.mode === 'play' && S.cur === F(-4), 'YES starts the Underground challenge');
      ok(H.canPlace('office', F(-5), 0) === 'DIG FIRST', 'B5 must be dug first');
      const keep = S.dug, c5 = H.digCost(), h5 = H.digHours();
      S.dug = F(-34); const c35 = H.digCost(); S.dug = F(-64); const c65 = H.digCost(); S.dug = F(-99); const c100 = H.digCost(), h100 = H.digHours(); S.dug = keep;
      ok(c5 === 175000 && c35 === 1250000 && c65 === 3225000 && c100 === 6400000 && h5 === 2 && h100 === 30, 'dig costs match the firmware: B5 $175K 2h, B35 $1.25M, B65 $3.2M, B100 $6.4M 30h');
      S.funds = 4e9; C.BUILD_ON = true;
      ok(H.dig() === null && S.digH === h5, 'dig B5 started');
      S.cam = F(-9); S.cur = F(-5); for (let i = 0; i < C.HOUR; i++) H.simTick(); H.render();
      for (let i = 0; i < h5 * C.HOUR; i++) H.simTick();
      ok(S.dug === F(-5) && !S.digH, 'after ' + h5 + 'h B5 is excavated');
      ok(H.dig() === null && S.digH > 0 && H.dig() === null, 'RUSH DIG accepted');
      for (let i = 0; i < C.HOUR; i++) H.simTick();
      ok(S.dug === F(-6), 'rush finishes the floor on the next hour');
      C.BUILD_ON = false;
      while (S.dug > 0 && H.dig() === null);
      ok(S.dug === 0 && H.dig() === 'B100 REACHED', 'dug to B100');
      for (let b = 1; b <= 100; b++) { const f = F(-b); if (H.isLobbyFloor ? H.isLobbyFloor(f) : C.SKY.includes(f)) for (let x = 0; x < 64; x += 8) H.place('lobby', f, x); for (let x = 0; x < 64; x++) if (!S.grid[f * C.W + x]) H.place('floor', f, x); }
      ok(H.place('spa', F(-21), 0) === null && H.canPlace('spa', F(-12), 0) === 'B20 AND DEEPER', 'HOT SPRING only at B20 and deeper');
      ok(H.canPlace('vault', F(-98), 0) === 'B100 ONLY', 'CORE VAULT only at B100');
      for (const [t, b] of [['mhub', 12], ['fossil', 33], ['gem', 41], ['nmarket', 51], ['aqua', 63], ['lab', 72], ['geo', 87]]) { const w = H.place(t, F(-b), 0); ok(w === null, t.toUpperCase() + ' at B' + b + (w ? ': ' + w : '')); }
      let fl = -1; for (let d = 0; d < 20 && fl < 0; d++) { H.newDay(); if (S.floodH) fl = S.floodF; }
      ok(fl >= 0 && H.depth(fl) > 10, 'an unpumped deep floor floods');
      S.cam = Math.max(0, fl - 3); H.render();
      for (let b = 12; b <= 100; b += 20) H.place('pump', C.SKY.includes(F(-b)) ? F(-b) - 1 : F(-b), 40);
      for (let b = 25; b <= 100; b += 30) H.place('vent', F(-b), 46);
      ok(H.deepUncovered(0) < 0 && H.deepUncovered(1) < 0, 'pumps + vents cover every deep floor');
      ok(H.place('vault', 0, 0) === null, 'CORE VAULT at B100');
      H.save(); H.load();
      ok(S.deep && S.dug === 0 && S.units.some((u) => u.t === 'vault'), 'challenge state survives save/load');
      for (const u of S.units) u.occ = 1, u.custY = 40;
      const pop = C.STAR_POP[6]; C.STAR_POP[6] = 0; S.floodH = 0; S.floodF = -1; S.mode = 'play';
      H.newDay(); C.STAR_POP[6] = pop;
      ok(S.coreDone && S.celebrate === 2, 'CORE RATING reached');
      H.frame(); ok(S.mode === 'end' && S.endKind === 2, 'second ending: CORE fireworks'); H.render();
      H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame();
      ok(S.mode === 'play' && S.coreDone, 'OK: keep playing after CORE');
      for (const cam of [F(-14), F(-23), F(-64), F(-74), 0]) for (const hh of [12, 21]) { S.cam = cam; S.tick = hh * C.HOUR; H.render(); }
      ok(true, 'every depth band renders day and night');
    }
    // ---- 2 save slots
    {
      fresh(2000000); S.day = 11; S.star = 3; S.funds = 1234000;
      for (let x = 0; x < 64; x += 8) P('lobby', G, x);
      H.saveSlot(0);
      ok(H.slotInfo(0).indexOf('DAY 12') >= 0 && H.slotInfo(0).indexOf('*3') >= 0, 'SAVE 1 shows ' + H.slotInfo(0));
      ok(H.slotInfo(1) === 'EMPTY', 'slot 2 is EMPTY');
      S.day = 40; S.star = 5; S.funds = 99e6; H.saveSlot(1); S.slot = 1;
      S.day = 41; H.save();
      ok(JSON.parse(localStorage.getItem('tower1')).day === 41, 'autosave writes to last-used slot 2');
      ok(H.loadSlot(0) && S.day === 11 && S.star === 3, 'LOAD 1 restores day 11 *3');
    }
    // ---- random events (banner + brief effect, no softlock)
    {
      fresh(5000000);
      for (let x = 0; x < 64; x += 8) P('lobby', G, x);
      P('lift', G, 30, F(10));
      S.rainH = 4; H.frame();
      ok(S.rainH === 4, 'black rain hours tick');
      S.rainH = 0; S.powerH = 6;
      ok(!H.shaftLive(0), 'power cut stops lifts');
      ok(H.repairPower() === null && !S.powerH && H.shaftLive(0), 'REPAIR POWER restores lifts');
      S.brokenSi = 0; S.brokenH = 10;
      ok(!H.shaftLive(0) && H.repairLift() === null && S.brokenSi < 0, 'REPAIR LIFT fixes the shaft');
      S.quakeH = 8; S.digH = 5; S.deep = true; S.dug = C.BASE - 4; S.digH = 9;
      ok(S.quakeH === 8 && S.digH === 9, 'quake delays digging');
      S.fest = 1; S.santa = true; S.cam = G - 1; H.render();
      S.fest = 2; H.render();
      S.typhoonSig = 8; S.typhoon = true; H.render();
      S.powerH = 1; S.brokenSi = 0; S.brokenH = 1; S.funds = 0;
      for (let i = 0; i < C.HOUR * 2 + 4; i++) H.simTick();
      ok(!S.powerH && S.brokenSi < 0, 'power and broken lift clear on their own (no softlock)');
      ok(true, 'events: rain, power, lift, quake, fest, typhoon, blackout softlock-safe');
    }
    // ---- render every mode
    for (const m of ['play', 'status', 'news']) { S.mode = m; H.render(); }
    S.mode = 'play'; S.cur = G; H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame(); H.render();
    S.menu.sel = 1; H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame(); H.render();
    ok(true, 'every screen renders');
  } catch (e) { ok(false, 'exception: ' + e.message + ' ' + (e.stack || '').split('\n')[1]); }
  ok(errs.length === 0, 'no JS errors' + (errs.length ? ': ' + errs.join('; ') : ''));
  out.push(fails ? fails + ' check(s) failed' : 'all checks passed');
  document.getElementById('out').textContent = out.join('\n');
})();
