
/* ---------------------------------------------------------------- the day: who goes where */
// Every game minute each tenant decides, from a few counters, whether a group sets off.
// No per-person state: an office is "6 workers, k inside, j on the way".
function inWin(h, a, b) { return h >= a && h < b; }
function spawnRate(need, h, end) { const mins = Math.max(1, (end - h) * 60); return Math.min(1, need / mins * 1.4); }
function nearestOpen(f, pred) {
  let best = null, bd = 1e9;
  for (const u of S.units) {
    if (!pred(u) || !u.occ || u.fire || u.build) continue;
    const d = Math.abs(u.f - f);
    if (d < bd && S.access[u.f]) { bd = d; best = u; }
  }
  return best;
}
function isOpen(u, h) {
  const d = T[u.t];
  if (S.typhoon) return false;
  if (d.open) return inWin(h, d.open[0], d.open[1]);
  return true;
}
function construction() {
  let done = false;
  for (const u of S.units) if (u.build) { u.build = Math.max(0, u.build - C.MIN); if (!u.build) { done = true; note(T[u.t].en + ' ON ' + floorName(u.f) + '/F IS READY'); } }
  for (const s of S.shafts) if (s.build) { s.build = Math.max(0, s.build - C.MIN); if (!s.build) { done = true; note(T[s.t].en + ' IS RUNNING'); } }
  if (done) computeAccess();
}
function finishNow(o) {   // instant-finish: pay RUSH_PCT of the price, scaled by the work left
  const left = o.build / (o.build0 || 1), price = Math.max(1000, Math.round(T[o.t].cost * C.RUSH_PCT / 100 * left));
  if (price > S.funds) return 'NOT ENOUGH $';
  spend(price); o.build = 0; S.stats.rushed++; computeAccess();
  return null;
}
function buySpeed(i) {
  if (S.owned[i]) return 'ALREADY OWNED';
  if (S.star < C.SPEED_STAR[i]) return 'NEEDS ' + C.SPEED_STAR[i] + ' STARS';
  if (C.SPEED_PRICE[i] > S.funds) return 'NOT ENOUGH $';
  spend(C.SPEED_PRICE[i]); S.owned[i] = 1; S.stats.speedBuys++;
  return null;
}
function minuteTick() {
  construction();
  const h = hour(), we = weekend(), noWork = we || S.typhoon || (S.rainH && rand() < 0.5);
  for (const u of S.units) {
    if (!u.occ || u.fire || u.build) continue;
    switch (u.t) {
      case 'office': {
        if (noWork) break;
        if (inWin(h, 7.5, 9)) {
          const need = C.OFFICE_POP - u.inside - u.transit;
          if (need > 0 && rand() < spawnRate(need, h, 9)) { const n = Math.min(need, 1 + ri(3)); spawn(entryFloor(), u.f, n, u.id, 'in'); }
        } else if (inWin(h, 12, 12.5) && u.inside >= 4 && !u.lunched) {
          u.lunched = 1;
          const food = nearestOpen(u.f, (q) => FOOD(q.t) && isOpen(q, h));
          if (food) { u.inside -= 3; spawn(u.f, food.f, 3, u.id, 'lunch', { dest: food.id, home: u.f }); }
        } else if (inWin(h, 17, 18.5) && u.inside > 0) {
          if (rand() < spawnRate(u.inside, h, 18.5)) { const n = Math.min(u.inside, 1 + ri(3)); u.inside -= n; spawn(u.f, C.BASE, n, u.id, 'out'); }
        }
        break;
      }
      case 'flat': {
        if (!we && inWin(h, 7, 8.5) && u.inside >= 3 && rand() < 0.05) { u.inside -= 2; spawn(u.f, C.BASE, 2, u.id, 'out'); u.away = 2; }
        else if (!we && inWin(h, 17, 20) && u.away && rand() < 0.03) { const n = u.away; u.away = 0; spawn(entryFloor(), u.f, n, u.id, 'back'); }
        else if (we && inWin(h, 10, 16) && u.inside >= 2 && !u.lunched && rand() < 0.01) {
          const sh = nearestOpen(u.f, (q) => COMMERCE(q.t) && q.t !== 'cinema' && isOpen(q, h));
          if (sh) { u.lunched = 1; u.inside -= 1; spawn(u.f, sh.f, 1, u.id, 'resshop', { dest: sh.id, home: u.f }); }
        }
        break;
      }
      case 'single': case 'twin': case 'suite': {
        if (inWin(h, 17, 21) && u.booked && !u.inside && !u.transit) {
          if (rand() < 0.02) { const n = T[u.t].guests; u.booked = 0; spawn(entryFloor(), u.f, n, u.id, 'guest'); u.stay = 1; }
        } else if (inWin(h, 7, 10) && u.inside && u.stay === 2 && rand() < 0.03) {
          const n = u.inside; u.inside = 0; u.stay = 0; spawn(u.f, C.BASE, n, u.id, 'out');
          earn(Math.round(T[u.t].night * [0.6, 1, 1.5][u.rent])); u.inc += T[u.t].night; u.dirty = 1;
        }
        break;
      }
      case 'cct': case 'dimsum': case 'shop': {
        if (!isOpen(u, h)) break;
        const d = T[u.t], len = d.open[1] - d.open[0];
        const target = d.base * (we ? 1.35 : 1) * (0.3 + u.eval / 100) * (S.units.some((m) => m.t === 'metro') ? 1.3 : 1)
          * (S.units.some((a) => a.t === 'atrium' && !a.build && Math.abs(a.f - u.f) <= 4) ? 1.3 : 1) * (S.celeb ? 1.5 : 1) * (S.rainH ? 0.5 : 1) * (S.alarmH ? 0.25 : 1) * (S.fest ? 1.3 : 1) * (S.typhoonSig === 3 ? 0.6 : 1);
        if (rand() < target / (len * 60) / 2.5) { const n = 1 + ri(4); spawn(entryFloor(), u.f, n, -1, 'visit', { dest: u.id, home: C.BASE }); }
        break;
      }
      case 'mega': {   // matinee, prime time (biggest), late show
        if (S.typhoon) break;
        const k = inWin(h, 14, 15) ? 0.6 : inWin(h, 18, 19) ? 1.5 : inWin(h, 20.5, 21.5) ? 0.9 : 0;
        if (k && rand() < T.mega.base * (we ? 1.5 : 1) * (0.3 + u.eval / 100) * k / 60 / 4) spawn(entryFloor(), u.f, 4, -1, 'visit', { dest: u.id, home: C.BASE });
        break;
      }
      case 'coaster': {   // evenings x1.6, weekends x2
        if (!isOpen(u, h)) break;
        const pk = (h >= 18 ? 1.6 : 1) * (we ? 2 : 1);
        if (rand() < T.coaster.base * pk * (0.3 + u.eval / 100) / (13 * 60) / 2.5) spawn(entryFloor(), u.f, 2 + ri(3), -1, 'visit', { dest: u.id, home: C.BASE });
        break;
      }
      case 'mhub': case 'spa': case 'fossil': case 'gem': case 'nmarket': case 'aqua': {   // underground sights
        if (!isOpen(u, h) || (S.floodH && u.f === S.floodF)) break;
        const d = T[u.t], len = d.open[1] - d.open[0];
        const target = d.base * (we ? 1.5 : 1) * (0.3 + u.eval / 100) * (S.celeb ? 1.5 : 1) / (ventOK(u.f) ? 1 : 2);
        if (rand() < target / (len * 60) / 2.5) spawn(entryFloor(), u.f, 1 + ri(4), -1, 'visit', { dest: u.id, home: C.BASE });
        break;
      }
      case 'pool': case 'ice': case 'climb': case 'banquet': {
        if (!isOpen(u, h) || (u.t === 'banquet' && h < 18)) break;
        const d = T[u.t], len = u.t === 'banquet' ? 5 : d.open[1] - d.open[0];
        const target = d.base * (u.t === 'ice' ? (we ? 2.2 : 1) : (we ? 1.4 : 1)) * (0.3 + u.eval / 100) * (S.celeb ? 1.5 : 1) * (S.rainH ? 0.5 : 1) * (S.alarmH ? 0.25 : 1) * (S.fest ? 1.3 : 1) * (S.typhoonSig === 3 ? 0.6 : 1);
        if (rand() < target / (len * 60) / 2.5) spawn(entryFloor(), u.f, u.t === 'banquet' ? 6 + ri(5) : 1 + ri(4), -1, 'visit', { dest: u.id, home: C.BASE });
        break;
      }
      case 'cinema': {
        for (const sh of T.cinema.show) if (inWin(h, sh - 1, sh) && !S.typhoon) {
          const target = T.cinema.base * (we ? 1.5 : 1) * (0.3 + u.eval / 100);
          if (rand() < target / 60 / 4) spawn(entryFloor(), u.f, 4, -1, 'visit', { dest: u.id, home: C.BASE });
        }
        break;
      }
    }
  }
  // VIP and wedding
  if (S.vip && !S.vip.sent && inWin(h, 18, 19)) { S.vip.sent = true; spawn(C.BASE, S.vip.room.f, 1, -1, 'vip'); }
  if (S.vip && S.vip.arrived && !S.vip.left && inWin(h, 10, 11) && S.vip.night) { S.vip.left = true; spawn(S.vip.room.f, C.BASE, 1, -1, 'vipout'); }
  if (S.wedding && !S.wedSent && we && inWin(h, 11, 12)) {
    S.wedSent = true; S.wedArrived = 0; S.wedWait = 0;
    const ch = S.units.find((u) => u.t === 'chapel');
    for (let i = 0; i < 6; i++) spawn(C.BASE, ch.f, 5, -1, 'wed');
    const bq = S.units.find((u) => u.t === 'banquet' && live(u));
    if (bq) { for (let i = 0; i < 4; i++) spawn(C.BASE, bq.f, 8, -1, 'wed'); note('WEDDING BANQUET TONIGHT'); }
  }
  if (S.wedSent && !S.towerDone && S.wedArrived >= 30) {
    S.towerDone = true; S.star = 6; S.celebrate = 1; note('A SKY WEDDING! TOWER RATING!'); sfx('fanfare');
  }
}

/* ---------------------------------------------------------------- new day, quarter */
function evalUnit(u) {
  if (!TENANT(u.t)) return;
  if (!S.access[u.f]) { u.eval = 0; return; }
  const waitMin = u.sN ? u.sSum / u.sN / C.MIN : (u.lastWait || 0);
  u.lastWait = waitMin;
  let e = 78 - Math.min(70, waitMin * 10) + [10, 0, -15][u.rent];
  // noise: offices dislike shops/food next door; homes and hotels dislike offices and shops
  for (const o of S.units) {
    if (o.f !== u.f || o === u) continue;
    const gap = Math.max(o.x - (u.x + u.w), u.x - (o.x + o.w));
    if (gap > 6) continue;
    if (u.t === 'office' && COMMERCE(o.t)) e -= 15;
    if (T[u.t].quiet && (COMMERCE(o.t) || o.t === 'office')) e -= 15;
  }
  const near = (t, r) => S.units.some((o) => o.t === t && !o.build && Math.abs(o.f - u.f) <= r);
  if (u.t === 'office' && S.units.some((o) => (o.t === 'carpark' || o.t === 'parking') && !o.build)) e += 8;   // car park
  if ((u.t === 'flat' || T[u.t].hotel) && near('pool', 15)) e += 8;                                          // pool
  if (near('garden', 8)) e += 10;                                                                            // sky garden calms
  if (u.f < C.BASE && !ventOK(u.f)) e -= 20;
  if (S.quakeH) e -= 15;                                                                 // stale air down deep
  for (const o of S.units) {   // coaster noise: beside any of its 5 floors, or just above / below
    if (o.t !== 'coaster' || o.build || u.f + 1 < o.f || u.f > o.f + 5) continue;
    const gap = Math.max(o.x - (u.x + u.w), u.x - (o.x + o.w));
    if (gap <= 6) e -= T[u.t].quiet ? 30 : 15;
  }
  if (T[u.t].hotel) {
    if (!housekeepingFor(u)) e -= 25;
    if (u.roach) e = 0;
  }
  if (u.t === 'flat' && S.star >= 3 && !S.units.some((c) => c.t === 'clinic' && Math.abs(c.f - u.f) <= 15)) e -= 10;
  u.eval = Math.max(0, Math.min(100, Math.round(e * 0.5 + u.eval * 0.5)));
  u.sSum = 0; u.sN = 0;
}
function housekeepingFor(u) { return S.units.some((h) => h.t === 'housek' && live(h) && Math.abs(h.f - u.f) <= C.HOUSEKEEP_RANGE); }
function newDay() {
  S.day++;
  S.tick = 0;
  S.celeb = !S.typhoon && S.units.some((u) => u.t === 'studio' && live(u)) && rand() < 1 / 6;
  if (S.celeb) note('CELEBRITY AT THE TV STUDIO! CROWDS!');
  if (S.deep && !S.floodH) {   // seepage: an unpumped deep floor floods now and then
    const ff = deepUncovered(0);
    if (ff >= 0 && rand() < 0.3) { S.floodF = ff; S.floodH = 6; for (const o of S.units) if (o.f <= ff && o.f + o.h - 1 >= ff) o.eval = Math.max(0, o.eval - 20); note('FLOOD ON ' + floorName(ff) + '! BUILD PUMP ROOMS'); }
  }
  if (S.deep && !S.coreDone && S.towerDone && starReqs(7).every((r) => r[1])) { S.coreDone = true; S.celebrate = 2; note('CORE RATING! THE DEEPEST TOWER ON EARTH'); sfx('fanfare'); }
  const q = S.day % 3 === 0;
  S.complain = false; S.typhoonSig = 0; S.typhoon = false;
  if (C.EVENTS_ON && quarter() === 2 && dayKind() < 2) {
    const r = rand();
    if (r < 0.08) { S.typhoonSig = 8; S.typhoon = true; note('TYPHOON SIGNAL 8: OFFICES+SHOPS SHUT'); }
    else if (r < 0.18) { S.typhoonSig = 3; note('TYPHOON SIGNAL 3: STRONG WIND'); }
    else if (r < 0.30) { S.typhoonSig = 1; note('TYPHOON SIGNAL 1: STAND BY'); }
  }
  for (const u of S.units) {
    u.lunched = 0;
    evalUnit(u);
    if (COMMERCE(u.t)) { u.custY = u.cust; if (u.occ) { const inc = u.cust * T[u.t].perCust; earn(inc); u.inc += inc; } u.lowDays = u.occ && u.cust < 8 ? u.lowDays + 1 : 0; if (u.lowDays >= 4) { u.occ = 0; u.lowDays = 0; note(T[u.t].en + ' ON ' + floorName(u.f) + '/F CLOSED: NO CUSTOMERS'); } u.cust = 0; }
    if (T[u.t].hotel) {
      if (u.stay === 1) u.stay = 2;
      if (u.dirty) { u.dirtyDays++; if (u.dirtyDays >= 3 && !u.roach) { u.roach = 1; note('COCKROACHES IN ' + floorName(u.f) + '/F ROOM!'); } }
    }
  }
  housekeeping();
  // tenants move in
  for (const u of S.units) {
    if (!TENANT(u.t) || u.occ || !S.access[u.f] || u.fire || u.build) continue;
    if (T[u.t].hotel) { if (!u.roach) u.occ = 1; continue; }
    const pre = 60 + [10, 0, -15][u.rent];
    if (pre < 40 || rand() > 0.5) continue;
    u.occ = 1; u.eval = Math.max(u.eval, 60);
    if (u.t === 'flat') { const v = Math.round(T.flat.sale * [0.8, 1, 1.2][u.rent]); earn(v); u.inc += v; S.stats.sales += v; note('FLAT SOLD ON ' + floorName(u.f) + '/F +$' + Math.round(v / 1000) + 'K'); }
    if (u.t === 'office') u.inside = 0;
    if (u.t === 'flat') u.inside = 3;
  }
  // hotel bookings for tonight
  for (const u of S.units) if (T[u.t].hotel && u.occ && !u.roach && !u.dirty && !u.inside && !(S.vip && S.vip.room === u)) u.booked = rand() < (u.eval / 100) * (weekend() ? 1 : 0.8) ? 1 : 0;
  if (S.vip && S.vip.sent && !S.vip.arrived) { S.vip.wait = 1e6; vipVerdict(); }
  if (q) newQuarter();
  population();
  checkStars();
  events();
  S.stats.dayIn = S.stats.dayOut = 0;
  save();
}
function housekeeping() {
  for (const h of S.units.filter((x) => x.t === 'housek' && live(x))) {
    let n = C.HOUSEKEEP_ROOMS;
    for (const u of S.units) {
      if (n <= 0) break;
      if (T[u.t].hotel && u.dirty && !u.roach && Math.abs(u.f - h.f) <= C.HOUSEKEEP_RANGE) { u.dirty = 0; u.dirtyDays = 0; n--; }
    }
  }
}
function newQuarter() {
  // rent from offices; unhappy tenants leave first
  let rent = 0;
  for (const u of S.units) {
    if (u.t === 'office' && u.occ) {
      if (u.eval < 35) { u.occ = 0; u.inside = 0; note('OFFICE ON ' + floorName(u.f) + '/F MOVED OUT'); continue; }
      const r = Math.round(T.office.rent * [0.6, 1, 1.5][u.rent]); rent += r; u.inc += r;
    }
    if (u.t === 'flat' && u.occ && u.eval < 25) { u.occ = 0; u.inside = 0; note('FLAT ON ' + floorName(u.f) + '/F IS FOR SALE AGAIN'); }
  }
  for (const u of S.units) if (!u.build) { if (u.t === 'carpark') rent += 40000 * u.h; if (u.t === 'studio') rent += 250000; if (u.t === 'lab') rent += 400000; if (u.t === 'geo') rent += 600000; if (u.t === 'vault') rent += 1000000; }   // season parking, studio lease
  earn(rent); S.stats.rentQ = rent;
  // upkeep
  let m = 0;
  for (const u of S.units) m += T[u.t].maint || 0;
  for (const s of S.shafts) m += T[s.t].maint * s.cars.length;
  if (S.star >= 3) m += 300 * S.units.filter((u) => u.t === 'lobby').length;
  spend(m);
  // never stuck: a small-business grant when the money has run out
  if (S.funds < C.GRANT_BELOW && S.grantQ !== S.day) {
    S.grantQ = S.day; const g = C.GRANT - Math.min(0, S.funds); earn(g); S.stats.grants += g; note('SMALL BUSINESS GRANT +$' + Math.round(g / 1000) + 'K');
  }
  if (quarter() === 0) { const ls = 20000 * S.star; earn(ls); note('KUNG HEI FAT CHOI! LAI SEE +$' + ls / 1000 + 'K'); }
  S.stats.qIn = S.stats.qOut = 0;
}
function population() {
  let p = 0;
  for (const u of S.units) {
    if (!u.occ) continue;
    if (u.t === 'office') p += C.OFFICE_POP;
    else if (u.t === 'flat') p += C.FLAT_POP;
    else if (T[u.t].hotel) p += T[u.t].guests * (u.inside || u.stay ? 1 : 0);
    else if (COMMERCE(u.t)) p += u.custY;
  }
  S.pop = p;
}
function starReqs(n) {
  const has = (t) => S.units.some((u) => u.t === t && live(u));
  const sp = C.STAR_POP[Math.min(6, n)];
  const r = [['POP ' + sp, S.pop >= sp]];
  if (n === 7) {   // CORE rating (Underground challenge)
    const sights = ['mhub', 'spa', 'fossil', 'gem', 'nmarket', 'aqua', 'lab', 'geo'].filter(has).length;
    r.push(['DIG TO B100', S.dug === 0], ['CORE VAULT', has('vault')], ['PUMPS', deepUncovered(0) < 0], ['VENTS', deepUncovered(1) < 0], [sights + '/6 DEEP SIGHTS', sights >= 6]);
  }
  if (n === 3) r.push(['SECURITY', has('security')]);
  if (n >= 3 && n <= 5 || n === 7) r.push(['REFUGE FLOORS', uncoveredFloor() < 0]);
  if (n === 4) r.push(['2 SUITES', S.units.filter((u) => u.t === 'suite' && live(u)).length >= 2], ['CLINIC', has('clinic')],
    ['RECYCLING', has('recycle')], ['CAR PARK', has('parking') || has('carpark')], ['VIP VISIT', S.vipOK]);
  if (n === 5) r.push(['METRO', has('metro')]);
  if (n === 6) r.push(['SKY CHAPEL', has('chapel')], ['WEDDING', S.towerDone]);
  return r;
}
function checkStars() {
  while (S.star < 5 && starReqs(S.star + 1).every((r) => r[1])) {
    S.star++; note('TOWER PROMOTED TO ' + S.star + ' STARS!'); sfx('fanfare');
  }
  if (S.star === 5 && !S.wedding && S.pop >= C.STAR_POP[6] && S.units.some((u) => u.t === 'chapel' && live(u))) { S.wedding = true; S.wedSent = false; note('A WEDDING IS BOOKED FOR THIS WEEKEND'); }
}
function events() {
  // VIP: when everything else for 4 stars is in place
  if (S.star === 3 && !S.vipOK && !S.vip && starReqs(4).filter((r) => r[0] !== 'VIP VISIT').every((r) => r[1]) && rand() < 0.35) {
    const room = S.units.filter((u) => u.t === 'suite' && live(u) && u.occ && !u.roach && !u.dirty && !u.inside).sort((a, b) => b.eval - a.eval)[0];
    if (room) { S.vip = { room, wait: 0, sent: false, arrived: false, left: false, night: false }; room.booked = 0; note('A VIP IS COMING TONIGHT!'); }
  } else if (S.vip && S.vip.arrived) S.vip.night = true;
  // a small fire (security nearby puts it out quickly)
  if (C.EVENTS_ON && !S.fire && S.units.length > 20 && rand() < 0.01) {
    const cands = S.units.filter((u) => TENANT(u.t) && u.f > C.BASE);
    if (cands.length) {
      const u = cands[ri(cands.length)];
      const guard = S.units.some((s) => s.t === 'security' && live(s) && Math.abs(s.f - u.f) <= C.SECURITY_RANGE);
      const safe = refugeCovered(u.f);
      u.fire = (guard ? 2 : 8) * (safe ? 1 : 2);   // hours
      S.fire = u.id; note('FIRE ON ' + floorName(u.f) + '/F! ' + (!safe ? 'NO REFUGE FLOOR' : guard ? 'SECURITY IS ON IT' : 'BUILD SECURITY NEXT TIME'));
    }
  }

  if (!C.EVENTS_ON) return;
  /* kid-friendly HK-flavoured random events */
  const yd = S.day % 36;
  S.fest = 0; S.santa = false;
  if (yd === 20) { S.fest = 2; note('MID-AUTUMN! MOONCAKES FOR ALL'); }
  else if (yd === 33) { S.fest = 1; S.santa = true; note('MERRY CHRISTMAS! SANTA IS HERE'); }
  else if (yd === 34 && rand() < 0.5) { S.santa = true; note('SANTA CLAUS IS RIDING THE LIFTS!'); }
  if (!S.typhoon && !S.rainH && !S.powerH && rand() < 0.08) { S.rainH = 4 + ri(4); note('BLACK RAIN WARNING! STAY INDOORS'); }
  if (!S.powerH && S.shafts.length && rand() < 0.04) { S.powerH = 6 + ri(6); note('POWER CUT! LIFTS STOPPED - PAY TO FIX'); }
  if (S.brokenSi < 0 && S.shafts.length && rand() < 0.05) {
    const si = ri(S.shafts.length); if (!S.shafts[si].build) { S.brokenSi = si; S.brokenH = 12 + ri(12); note('LIFT BROKEN - PAY TO FIX'); }
  }
  if (!S.celeb && !S.typhoon && rand() < 0.04) { S.celeb = true; note('SURPRISE! A STAR IS IN THE TOWER'); }
  if (!S.quakeH && rand() < 0.03) {
    S.quakeH = 8 + ri(8);
    for (const u of S.units) if (u.eval > 10) u.eval -= 10;
    if (S.digH) S.digH += 3 + ri(4);
    note('EARTHQUAKE! EVERYONE IS SHAKEN');
  }
  if (!S.alarmH && S.units.some((u) => u.t === 'security' && live(u)) && rand() < 0.04) { S.alarmH = 2 + ri(2); note('FALSE ALARM! SECURITY IS CHECKING'); }
  if (!S.complain && S.vipOK && S.star >= 4 && rand() < 0.05) {
    S.complain = true;
    for (const u of S.units) if (u.t === 'suite' && live(u) && u.eval > 5) u.eval -= 5;
    note('VIP COMPLAINT: TOO SLOW LIFTS!');
  }

}
function vipVerdict() {
  const mins = S.vip.wait / C.MIN, room = S.vip.room;
  const good = mins < 4 && S.vip.arrived && room.eval >= 55 && !room.roach;
  S.vipOK = good; note(good ? 'THE VIP LOVED IT!' : 'THE VIP WAS NOT IMPRESSED (' + Math.round(mins) + ' MIN WAITING)');
  S.vip = null; checkStars();
}
function hourTick() {
  if (S.digH && --S.digH === 0) {
    S.dug--; note(floorName(S.dug) + ' EXCAVATED');
    if (rand() < 0.18) { const t = 50000 + ri(450000); earn(t); note('TREASURE FOUND! +$' + Math.round(t / 1000) + 'K'); }
  }
  if (S.powerH && --S.powerH === 0) note('POWER IS BACK');
  if (S.rainH && --S.rainH === 0) note('THE BLACK RAIN HAS STOPPED');
  if (S.quakeH && --S.quakeH === 0) note('THE SHAKING HAS STOPPED');
  if (S.alarmH && --S.alarmH === 0) note('FALSE ALARM - ALL CLEAR');
  if (S.brokenH && --S.brokenH === 0 && S.brokenSi >= 0) { S.brokenSi = -1; note('THE LIFT FIXED ITSELF'); }
  if (S.floodH && --S.floodH === 0) { S.floodF = -1; note('THE WATER IS PUMPED OUT'); }
  if (S.fire) {
    const u = unitById(S.fire);
    if (!u) S.fire = null;
    else if (--u.fire <= 0) {
      S.fire = null; u.eval = Math.max(0, u.eval - 30);
      const guard = S.units.some((s) => s.t === 'security' && live(s) && Math.abs(s.f - u.f) <= C.SECURITY_RANGE);
      if (!refugeCovered(u.f)) { for (const o of S.units) if (o.f === u.f && TENANT(o.t) && o.occ) { o.occ = 0; o.inside = 0; o.eval = Math.max(0, o.eval - 50); } S.stats.firesBig = (S.stats.firesBig || 0) + 1; note('NO REFUGE FLOOR: FIRE GUTTED THE FLOOR'); }
      else if (!guard) { u.occ = 0; u.inside = 0; note('THE FIRE GUTTED A ' + T[u.t].en); if (!S.powerH) { S.powerH = 4; note('BLACKOUT AFTER THE FIRE!'); } } else note('FIRE OUT');
    }
  }
}

/* ---------------------------------------------------------------- one tick */
function simTick() {
  S.tick++;
  stepPeople();
  if (S.tick % C.MIN === 0) { stepPending(); minuteTick(); }
  if (S.tick % C.HOUR === 0) { hourTick(); population(); }
  if (S.tick >= C.DAY) newDay();
}
