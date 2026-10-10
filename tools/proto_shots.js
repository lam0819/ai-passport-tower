// Underground-challenge screenshots from the prototype (run by tools/proto_shots.sh)
window.onerror = (m, s, l) => { document.getElementById("out").textContent = "ERR " + m + " line " + l; };
(function () {
  const H = window.__TW, C = H.C, S = H.S, F = (n) => C.BASE + n, shots = [];
  const snap = (n) => { H.render(); shots.push(n + ' ' + document.querySelector('canvas').toDataURL('image/png')); };
  H.stop(); C.BUILD_ON = false; H.seed(7); H.freshState(); S.mode = 'play'; S.funds = 4e9; S.star = 6; S.towerDone = true;
  for (let x = 0; x < 64; x += 8) H.place('lobby', F(0), x);
  for (let b = 1; b <= 4; b++) for (let x = 0; x < 64; x++) if (!S.grid[F(-b) * C.W + x]) H.place('floor', F(-b), x);
  S.celebrate = 1; H.frame(); for (let i = 0; i < 30; i++) H.render(); snap('proto_ending_prompt');
  H.setKey('ok', true); H.frame(); H.setKey('ok', false); H.frame();
  C.BUILD_ON = true; H.dig(); for (let i = 0; i < C.HOUR; i++) H.simTick(); S.cam = F(-9); S.cur = F(-5); snap('proto_underground_digging');
  C.BUILD_ON = false; S.digH = 0; while (S.dug > 0 && H.dig() === null);
  for (let b = 5; b <= 100; b++) { const f = F(-b); if (C.SKY.includes(f)) for (let x = 0; x < 64; x += 8) H.place('lobby', f, x); for (let x = 0; x < 64; x++) if (!S.grid[f * C.W + x]) H.place('floor', f, x); }
  for (const [t, b] of [['mhub', 12], ['spa', 21], ['fossil', 33], ['gem', 41], ['nmarket', 51], ['aqua', 63], ['lab', 72], ['geo', 87]]) H.place(t, F(-b), 0);
  for (let b = 12; b <= 100; b += 20) H.place('pump', C.SKY.includes(F(-b)) ? F(-b) - 1 : F(-b), 40);
  for (let b = 25; b <= 100; b += 30) H.place('vent', F(-b), 46);
  H.place('vault', 0, 0); H.place('exp', F(-90), 58, F(0));
  for (const u of S.units) u.occ = 1, u.custY = 40, u.build = 0;
  for (const sh of S.shafts) sh.build = 0;
  S.tick = 20 * C.HOUR;
  for (const [n, cam, cur] of [['metro_soil', F(-14), F(-11)], ['spa_fossil', F(-23), F(-20)], ['aquarium_rock', F(-64), F(-62)], ['core_magma', 0, F(-99)]]) { S.cam = cam; S.cur = cur; for (let i = 0; i < 9; i++) H.frame(); S.mode = 'play'; snap('proto_underground_' + n); }
  S.floodF = F(-41); S.floodH = 4; S.cam = F(-45); S.cur = F(-41); snap('proto_underground_flood');
  S.floodH = 0; S.coreDone = true; S.celebrate = 2; S.cam = 0; H.frame(); for (let i = 0; i < 30; i++) H.render(); snap('proto_ending_core');
  document.getElementById('out').textContent = shots.join('\n');
})();
