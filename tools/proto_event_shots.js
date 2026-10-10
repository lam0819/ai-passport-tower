(function () {
  const H = window.__TW, C = H.C, S = H.S, F = (n) => C.BASE + n, shots = [];
  const snap = (n) => { H.render(); shots.push(n + ' ' + document.querySelector('canvas').toDataURL('image/png')); };
  H.stop(); C.BUILD_ON = false; C.EVENTS_ON = false; H.seed(7); H.freshState(); S.mode = 'play'; S.funds = 5e6; S.star = 3;
  for (let x = 0; x < 64; x += 8) H.place('lobby', F(0), x);
  for (let f = 1; f <= 6; f++) { H.place('office', F(f), 0); H.place('office', F(f), 36); for (let x = 9; x < 36; x++) if (!S.grid[F(f)*C.W+x]) H.place('floor', F(f), x); for (let x = 45; x < 64; x++) if (!S.grid[F(f)*C.W+x]) H.place('floor', F(f), x); }
  H.place('lift', F(0), 28, F(6));
  S.cam = F(0) - 1; S.cur = F(3); S.tick = 14 * C.HOUR;
  S.rainH = 5; S.msg = 'BLACK RAIN WARNING! STAY INDOORS'; S.msg_t = 150; H.render(); snap('proto_event_black_rain');
  S.rainH = 0; S.powerH = 5; S.msg = 'POWER CUT! LIFTS STOPPED - PAY TO FIX'; H.render(); snap('proto_event_power_cut');
  S.powerH = 0; S.fest = 1; S.santa = true; S.msg = 'MERRY CHRISTMAS! SANTA IS HERE'; S.tick = 20 * C.HOUR; H.render(); snap('proto_event_christmas');
  S.fest = 2; S.santa = false; S.msg = 'MID-AUTUMN! MOONCAKES FOR ALL'; H.render(); snap('proto_event_midautumn');
  S.fest = 0; S.typhoonSig = 8; S.typhoon = true; S.msg = 'TYPHOON SIGNAL 8: OFFICES+SHOPS SHUT'; H.render(); snap('proto_event_typhoon8');
  S.mode = 'title'; S.menu = null; H.titleMenu(); H.render(); snap('proto_title_slots');
  document.getElementById('out').textContent = shots.join('\n');
})();
