
/* ---------------------------------------------------------------- people: groups, not persons */
// A group is 1..6 people with one trip (from, to). Pool is fixed; when it is full the trip is
// counted as "overflow" and the people simply arrive (no stress), so nothing ever blocks.
function spawn(from, to, n, u, kind, ex) {
  S.stats.spawned += n;
  const r = route(from, to);
  if (!r) { S.stats.failed += n; failTrip(u, n, kind); if (kind === 'in' || kind === 'guest' || kind === 'back') { const un = u >= 0 ? unitById(u) : null; if (un) un.inside += n; } return false; }
  if (S.groups.length >= C.MAX_GROUPS) { S.stats.overflow += n; arrive(Object.assign({ from, to, n, u, kind, wait: 0 }, ex)); return true; }
  const g = Object.assign({ from, to, n, u, kind, legs: r.legs, leg: 0, wait: 0, walk: r.walk * C.WALK_PER_FLOOR, st: r.walk ? 'walk' : 'q', f: from }, ex);
  S.groups.push(g);
  if (S.groups.length > S.stats.peakGroups) S.stats.peakGroups = S.groups.length;
  const un = u >= 0 ? unitById(u) : null;
  if (un && (kind === 'in' || kind === 'back')) un.transit += n;
  if (!r.walk) enqueue(g);
  return true;
}
function enqueue(g) {
  const L = g.legs[g.leg];
  g.st = 'q'; g.f = L.from;
  S.shafts[L.s].q[L.from].push(g);
}
function failTrip(uid, n, kind) {
  const u = uid >= 0 ? unitById(uid) : null;
  if (u) { u.sSum += 60 * C.MIN * n; u.sN += n; }   // no way there: counts as an hour of waiting
}
function arrive(g) {
  S.stats.arrived += g.n;
  if (g.legs && g.legs.length) { S.stats.waitSum += g.wait * g.n; S.stats.waitN += g.n; if (g.wait > S.stats.maxWait) S.stats.maxWait = g.wait; }
  const u = g.u >= 0 ? unitById(g.u) : null;
  if (u) { u.sSum += g.wait * g.n; u.sN += g.n; }
  switch (g.kind) {
    case 'in': case 'back': if (u) { u.transit = Math.max(0, u.transit - g.n); u.inside += g.n; } break;
    case 'out': break;
    case 'lunch': case 'visit': case 'resshop': {
      const dest = g.dest >= 0 ? unitById(g.dest) : null;
      if (dest) dest.cust += g.n;
      const stay = g.kind === 'lunch' ? 30 : 45 + ri(45);
      later(S.tick + stay * C.MIN, g.to, g.home, g.n, g.u, g.kind === 'visit' ? 'leave' : 'back');
      break;
    }
    case 'guest': if (u) { u.inside += g.n; u.transit = Math.max(0, u.transit - g.n); } break;
    case 'vip': S.vip.arrived = true; S.vip.wait += g.wait; break;
    case 'vipout': S.vip.wait += g.wait; vipVerdict(); break;
    case 'wed': S.wedArrived += g.n; S.wedWait += g.wait * g.n; break;
  }
}
function later(at, from, to, n, u, kind) {
  if (S.pend.length >= C.MAX_PEND) { spawn(from, to, n, u, kind); return; }
  S.pend.push({ at, from, to, n, u, kind });
}
function stepPending() {
  for (let i = S.pend.length - 1; i >= 0; i--) {
    const p = S.pend[i];
    if (p.at <= S.tick || S.tick < 60) { S.pend.splice(i, 1); spawn(p.from, p.to, p.n, p.u, p.kind === 'leave' ? 'out' : p.kind); }
  }
}

/* ---------------------------------------------------------------- lifts: collective control */
function wantsStop(s, c, f) {
  for (const g of c.g) if (g.legs[g.leg].to === f) return true;
  const q = s.q[f];
  if (!q.length || !serves(s, f) || c.load >= C.CAP[s.t]) return false;
  for (const g of q) { const up = g.legs[g.leg].to > f; if (c.dir === 0 || (up ? 1 : -1) === c.dir) return true; }
  return false;
}
function callsAhead(s, c, dir) {
  for (const g of c.g) if ((g.legs[g.leg].to - c.f) * dir > 0) return true;
  for (let f = c.f + dir; f >= s.f0 && f <= s.f1; f += dir) if (s.q[f].length) return true;
  return false;
}
function stepCar(s, c) {
  if (c.door > 0) { c.door--; return; }
  const f = c.f;
  let moved = false;
  // unload
  for (let i = c.g.length - 1; i >= 0; i--) {
    const g = c.g[i];
    if (g.legs[g.leg].to !== f) continue;
    c.g.splice(i, 1); c.load -= g.n; moved = true;
    g.leg++;
    if (g.leg < g.legs.length) enqueue(g); else { g.st = 'done'; arrive(g); }
    c.pout += g.n; c.stopF = f;
  }
  // decide direction
  if (c.dir === 0 || !callsAhead(s, c, c.dir)) {
    const d0 = c.dir || 1;
    c.dir = callsAhead(s, c, d0) ? d0 : callsAhead(s, c, -d0) ? -d0 : 0;
    if (c.dir === 0 && s.q[f].length) { const g = s.q[f][0]; c.dir = g.legs[g.leg].to > f ? 1 : -1; }
  }
  // load the ones going our way
  const q = s.q[f];
  if (serves(s, f) && !(c.load && (c.dwell || 0) >= 2)) {   // doors close after 2 loads with riders aboard
    for (let i = 0; i < q.length && c.dir !== 0; i++) {
      const g = q[i], up = g.legs[g.leg].to > f;
      if ((up ? 1 : -1) !== c.dir || c.load + g.n > C.CAP[s.t]) continue;
      q.splice(i, 1); i--;
      c.g.push(g); c.load += g.n; g.st = 'ride'; moved = true; c.pin += g.n; c.stopF = f;
    }
  }
  if (moved) { c.door = C.DOOR; c.dwell = (c.dwell || 0) + 1; return; }
  if (c.dir === 0) return;
  c.dwell = 0;
  // move one floor (express: up to 3), stopping where wanted
  const sp = s.t === 'exp' ? C.EXP_SPEED : 1;
  for (let k = 0; k < sp; k++) {
    const nf = c.f + c.dir;
    if (nf < s.f0 || nf > s.f1) { c.dir = 0; break; }
    c.f = nf;
    if (wantsStop(s, c, nf)) break;
  }
}
function stepPeople() {
  for (let si = 0; si < S.shafts.length; si++) {
    const s = S.shafts[si];
    if (s.build || !shaftLive(si)) continue;
    for (const c of s.cars) stepCar(s, c);
    for (let f = s.f0; f <= s.f1; f++) for (const g of s.q[f]) g.wait++;
  }
  for (const g of S.groups) if (g.st === 'walk' && --g.walk <= 0) {
    if (g.legs.length) { const L = g.legs[0]; g.legs[0] = { s: L.s, from: L.from, to: L.to }; enqueue(g); }
    else { g.st = 'done'; arrive(g); }
  }
  if (S.groups.some((g) => g.st === 'done')) S.groups = S.groups.filter((g) => g.st !== 'done');
}
/* People queued at a floor of a shaft, for drawing */
function waiting(s, f) { let n = 0, w = 0; for (const g of s.q[f]) { n += g.n; w = Math.max(w, g.wait); } return [n, w]; }
