
/* ---------------------------------------------------------------- drawing (320x240) */
const cv = document.getElementById('c'), X = cv.getContext('2d');
X.imageSmoothingEnabled = false;
let blink = 0;
function rect(x, y, w, h, c) { X.fillStyle = c; X.fillRect(x, y, w, h); }
function txt(s, x, y, c, al) { X.fillStyle = c || '#fff'; X.textAlign = al || 'left'; X.fillText(s, x, y); }
function skyCol() {
  const h = hour();
  if (h < 5 || h >= 20) return ['#0a0e2a', '#141a40'];
  if (h < 7) return ['#40305a', '#e09060'];
  if (h >= 18) return ['#30285a', '#d07050'];
  return S.typhoon ? ['#4a5058', '#707880'] : ['#4aa0e0', '#a8d8f0'];
}
const night = () => hour() < 6 || hour() >= 19;
function fy(f) { return C.TY + (C.VIS - 1 - (f - S.cam)) * C.FH; }   // top y of floor row
const SHOPS = [   // six HK shop types, told apart by storefront art (no Chinese on signs); word = tiny English sign
  { en: 'CONVENIENCE', word: '24H', sign: '#1060c0', band: '#ff9020', neon: '#ffb040', inside: '#f4f8ff', goods: ['#e04040', '#40a0e0', '#f0c020'] },
  { en: 'PHARMACY', word: 'PHARM', sign: '#10a050', band: '#ffffff', neon: '#60ff90', inside: '#eefaf0', goods: ['#ffffff', '#f06080', '#60a0f0'] },
  { en: 'GOLDSMITH', word: 'GOLD', sign: '#a01010', band: '#ffd700', neon: '#ffe060', inside: '#5a1010', goods: ['#ffd700', '#ffcc33', '#fff0a0'] },
  { en: 'BAKERY', word: 'BREAD', sign: '#8a4a20', band: '#fff0d0', neon: '#ffa050', inside: '#fff0d8', goods: ['#d8902c', '#e8b060', '#c07020'] },
  { en: 'ELECTRONICS', word: 'TV', sign: '#202838', band: '#30e0ff', neon: '#30e0ff', inside: '#283040', goods: ['#2050ff', '#30e0ff', '#ff60a0'] },
  { en: 'FASHION', word: 'STYLE', sign: '#d02080', band: '#ffffff', neon: '#ff60c0', inside: '#fff0f8', goods: ['#ff80c0', '#80c0ff', '#ffe060'] },
];
function glyph(s, x, y, c, px) { X.font = 'bold ' + (px || 7) + 'px sans-serif'; txt(s, x, y, c, 'center'); X.font = '8px monospace'; }
function glow(x, y, w, h, c) { X.globalAlpha = 0.25 + 0.1 * Math.sin(blink / 5); rect(x - 2, y - 2, w + 4, h + 4, c); X.globalAlpha = 1; }
function person(x, y, c) { rect(x, y, 2, 2, '#f0c8a0'); rect(x, y + 2, 2, 4, c); }
function shutter(x, y, w, h, paper) {
  rect(x, y, w, h, '#8a8c90');
  for (let i = 1; i < h; i += 2) rect(x, y + i, w, 1, '#6a6c70');
  if (paper) { rect(x + w / 2 - 6, y + 1, 12, h - 2, '#f4f0e0'); glyph(paper, x + w / 2, y + h - 1, '#c03030', 6); }
}
function signIcon(k, x, y) {
  const W = '#fff';
  if (k === 0) { rect(x, y + 1, 5, 6, W); rect(x + 1, y + 2, 3, 2, '#80d0ff'); }
  if (k === 1) { rect(x + 2, y, 2, 7, W); rect(x, y + 2, 6, 2, W); }
  if (k === 2) { rect(x, y + 3, 6, 3, '#ffd700'); rect(x + 1, y + 1, 4, 2, '#ffd700'); }
  if (k === 3) { rect(x, y + 3, 6, 3, '#e0a050'); rect(x + 1, y + 2, 4, 1, '#e0a050'); }
  if (k === 4) { rect(x, y + 1, 6, 5, '#30e0ff'); rect(x + 1, y + 2, 4, 3, '#2050ff'); }
  if (k === 5) { rect(x + 1, y, 4, 2, W); rect(x, y + 2, 6, 2, W); rect(x + 1, y + 4, 4, 3, W); }
}
function drawShop(u, x, y, w) {
  const k = (u.v || 0) % 6, v = SHOPS[k], h = hour(), open = u.occ && isOpen(u, h) && !u.build, nt = night() || h >= 18;
  if (nt && u.occ) glow(x + 1, y + 1, w - 2, 8, v.neon);
  rect(x + 1, y + 1, w - 2, 8, v.sign); rect(x + 1, y + 8, w - 2, 1, v.band);
  signIcon(k, x + 3, y + 1);
  X.font = '7px monospace'; txt(v.word, x + 11, y + 7, nt && u.occ ? v.neon : v.band); X.font = '8px monospace';
  const lx = x + w - 6;
  rect(lx, y + 1, 4, 19, nt && u.occ ? v.neon : v.sign); for (let i = 0; i < 4; i++) rect(lx + 1, y + 3 + i * 4, 2, 2, u.occ ? v.band : '#bbb');
  if (nt && u.occ) rect(lx + 1, y + 2 + ((blink >> 2) % 13), 2, 1, '#fff');
  if (!open) { shutter(x + 1, y + 9, w - 8, 14, u.occ ? null : 'TO LET'); return; }
  const iw = w - 8, ix = x + 1; let iy = y + 9;
  for (let i = 0; i < iw; i += 2) rect(ix + i, iy, 2, 2, (i / 2) % 2 ? '#fff' : v.sign);
  iy += 2;
  rect(ix, iy, iw, 13, nt && k !== 2 && k !== 4 ? '#fff8d8' : v.inside);
  for (let i = 0; i < iw; i += 4) rect(ix + i, iy + 11, 2, 2, '#d0c8b8');   // floor tiles
  iy += 2;
  const gx = ix + 10, gw = iw - 11, G3 = v.goods;
  if (k === 0) for (let f = 0; f + 9 <= gw; f += 10) {   // fridges
    rect(gx + f, iy, 9, 8, '#c0c8d0'); rect(gx + f + 1, iy + 1, 7, 6, '#d8f4ff');
    for (let r = 0; r < 3; r++) for (let b = 0; b < 3; b++) rect(gx + f + 2 + b * 2, iy + 1 + r * 2, 1, 2, G3[(r + b + f) % 3]);
    rect(gx + f + 4, iy, 1, 8, '#a0a8b0');
  }
  if (k === 1) {   // green cross + shelves
    rect(gx + 1, iy + 1, 9, 7, '#fff'); rect(gx + 4, iy + 1, 3, 7, '#10b050'); rect(gx + 2, iy + 3, 7, 3, '#10b050');
    for (let r = 0; r < 2; r++) { rect(gx + 12, iy + 3 + r * 4, gw - 12, 1, '#c0c0c0'); for (let i = 0; i < gw - 13; i += 3) rect(gx + 12 + i, iy + 1 + r * 4, 2, 2, G3[(i / 3 + r) % 3]); }
  }
  if (k === 2) {   // glittering gold window
    rect(gx, iy + 1, gw, 7, '#301008'); rect(gx, iy + 1, gw, 1, '#ffd700');
    for (let i = 1; i < gw - 2; i += 3) rect(gx + i, iy + 4 + (i & 1), 2, 2, G3[i % 3]);
    for (let i = 0; i < 3; i++) { const sx = gx + 2 + ((i * 11 + (blink >> 1) * 7) % (gw - 4)), sy = iy + 2 + (i * 2 + (blink >> 2)) % 5;
      rect(sx, sy, 1, 1, '#fff'); if ((blink >> 1) & 1) { rect(sx - 1, sy, 3, 1, '#fff0a0'); rect(sx, sy - 1, 1, 3, '#fff0a0'); } }
  }
  if (k === 3) {   // loaves + layer cake
    rect(gx, iy + 5, gw, 1, '#b08050');
    for (let i = 0; i + 4 <= gw - 9; i += 5) { rect(gx + i, iy + 3, 4, 2, G3[(i / 5) % 3]); rect(gx + i + 1, iy + 2, 2, 1, G3[(i / 5) % 3]); }
    for (let i = 0; i + 3 <= gw - 9; i += 4) rect(gx + i, iy + 6, 3, 2, '#e8b060');
    const cx = gx + gw - 8; rect(cx, iy + 4, 7, 4, '#ffc0d0'); rect(cx + 1, iy + 2, 5, 2, '#fff'); rect(cx + 2, iy + 1, 3, 1, '#ff4060'); rect(cx, iy + 5, 7, 1, '#ff80a0');
  }
  if (k === 4) for (let r = 0; r < 2; r++) for (let i = 0; i + 6 <= gw; i += 7) {   // TV wall
    rect(gx + i, iy + r * 4, 6, 4, '#101010'); rect(gx + i + 1, iy + r * 4 + 1, 4, 2, G3[(i / 7 + r + (blink >> 3)) % 3]);
  }
  if (k === 5) for (let i = 1; i + 5 <= gw; i += 7) {   // mannequins
    rect(gx + i + 1, iy, 2, 2, '#e8d8c8'); rect(gx + i, iy + 2, 4, 4, G3[(i / 7 | 0) % 3]); rect(gx + i - 1, iy + 5, 6, 2, G3[(i / 7 | 0) % 3]);
    rect(gx + i + 1, iy + 7, 1, 1, '#808080'); rect(gx + i + 2, iy + 7, 1, 1, '#808080');
  }
  rect(ix + 1, iy + 4, 7, 4, '#7a5a3a'); person(ix + 3, iy, v.sign);
  const n = Math.min(2, Math.floor((u.cust || u.custY) / 15));
  for (let i = 0; i < n; i++) person(gx + 2 + ((i * 13 + (blink >> 3)) % (gw - 4)), iy + 1, ['#3050a0', '#a03050'][i]);
}
function drawFood(u, x, y, w) {
  const h = hour(), open = u.occ && isOpen(u, h) && !u.build, nt = night() || h >= 18, ds = u.t === 'dimsum';
  const sign = ds ? '#b01010' : '#e83830', band = ds ? '#ffd040' : '#ffe060', neon = ds ? '#ff4040' : '#ffe060';
  if (nt && u.occ) glow(x + 1, y + 1, w - 2, 8, neon);
  rect(x + 1, y + 1, w - 2, 8, sign); rect(x + 1, y + 1, w - 2, 1, band); rect(x + 1, y + 8, w - 2, 1, band);
  glyph(ds ? 'DIM SUM' : 'CAFE', x + w / 2, y + 8, u.occ ? band : '#999');
  const lx = x + w - 5; rect(lx, y + 1, 4, 15, nt && u.occ ? neon : sign); rect(lx + 1, y + 4, 2, 2, '#fff'); rect(lx + 1, y + 9, 2, 2, '#fff');
  if (!open) { shutter(x + 1, y + 9, w - 7, 10, u.occ ? null : 'TO LET'); return; }
  rect(x + 1, y + 9, w - 7, 10, nt ? '#ffe8c0' : ds ? '#fbe0c0' : '#f8f0e0');
  if (ds) for (let i = 8; i < w - 10; i += 18) { rect(x + i + 4, y + 9, 1, 2, '#c00'); rect(x + i + 3, y + 11, 3, 3, (blink >> 3) & 1 ? '#ff4030' : '#e02020'); }   // lanterns
  for (let i = 4; i < w - 12; i += ds ? 14 : 10) {
    rect(x + i, y + 15, ds ? 9 : 6, 1, ds ? '#c02020' : '#8a5a2a'); rect(x + i + (ds ? 4 : 2), y + 16, 1, 3, '#555');
    rect(x + i + 1, y + 13, 2, 2, '#c88040'); rect(x + i + 4, y + 13, 2, 2, '#fff');      // milk tea, rice bowl
    if (((i + u.id) % 3) || (u.cust || u.custY) > 30) person(x + i + (ds ? 10 : 7), y + 12, ['#3050a0', '#a03050', '#309050'][i % 3]);
  }
}
function drawCinema(u, x, y, w, h) {
  const nt = night() || hour() >= 18;
  rect(x, y, w, h - 1, '#3a1c50');
  rect(x + 2, y + 2, w - 4, 9, '#200c30');
  for (let i = 0; i < w - 4; i += 3) rect(x + 2 + i, y + 2, 1, 1, ((i / 3 + (blink >> 2)) % 3 === 0) ? '#fff6a0' : '#806020');   // chasing bulbs
  glyph('CINEMA', x + w / 2, y + 10, nt ? '#ffe060' : '#ffd040', 8);
  for (let i = 0; i < 4; i++) { rect(x + 8 + i * 28, y + 15, 14, 14, ['#c03030', '#3050c0', '#30a060', '#d0a020'][i]); rect(x + 10 + i * 28, y + 17, 10, 4, '#fff'); }
  if (!u.occ || u.build) shutter(x + 2, y + 18, w - 4, 10, u.occ ? null : 'TO LET');
}

/* ---- refuge floor, MEGA cinema, coaster and the other multi-storey facilities (y = top of the room) */
function drawRefuge(u, x, y, w) {
  const FH = C.FH, nt = night();
  rect(x, y, w, FH - 1, '#6a706c');
  for (let i = 0; i < w; i += 8) rect(x + i, y + FH - 5, 4, 2, '#d8c040');
  rect(x, y + 1, w, 2, '#c03030'); rect(x, y + 4, w, 1, '#9098a0');
  if (u.x === 0) rect(x, y + 6, 3, FH - 10, nt ? '#203050' : '#88c0f0');
  if (u.x + u.w === C.W) rect(x + w - 3, y + 6, 3, FH - 10, nt ? '#203050' : '#88c0f0');
  const first = u.x === 0;
  if (first && w >= 30) { rect(x + 5, y + 7, 22, FH - 10, '#50565c'); rect(x + 8, y + 11, 7, 7, '#2a70b0'); rect(x + 18, y + 11, 7, 7, '#2a70b0'); }
  for (let i = first && w >= 30 ? 46 : 6; i + 8 < w; i += 40) { rect(x + i, y + 7, 7, 7, '#c02020'); rect(x + i + 2, y + 9, 3, 3, '#f0d0d0'); }
  for (let i = 18; i + 26 < w; i += 40) { rect(x + i, y + 6, 26, 8, '#10a040'); glyph('EXIT', x + i + 13, y + 13, '#e0ffe0', 7); }
  if (w >= 50) glyph('REFUGE', x + w / 2 + 8, y + 21, '#f0e060', 7);
  if (nt) { X.globalAlpha = 0.12; rect(x, y, w, FH - 1, '#60ff90'); X.globalAlpha = 1; }
}
function drawMega(u, x, y, w, h) {
  const on = u.occ && !S.typhoon;
  rect(x, y, w, h - 1, '#180c28'); rect(x + 2, y + 2, w - 4, 14, '#301040');
  for (let i = 0; i < (w - 4) / 4; i++) { const c = (i + (blink >> 2)) % 3 ? '#ffe070' : '#705020'; rect(x + 3 + i * 4, y + 2, 2, 2, c); rect(x + 3 + i * 4, y + 14, 2, 2, c); }
  glyph('MEGA', x + w / 2, y + 14, '#ff3070', 12);
  const sx = x + w / 2 - 6, sw = w / 2 - 2, sy = y + 19, sh = h - 26;
  rect(sx - 2, sy - 2, sw + 4, sh + 4, '#000');
  if (on) {
    rect(sx, sy, sw, sh, (blink >> 1) & 1 ? '#b0c8f0' : '#a0b8e0');
    const px = sx + ((blink >> 1) % (sw - 14)); rect(px, sy + sh - 22, 12, 16, '#405080'); rect(px + 3, sy + sh - 28, 6, 6, '#405080');
    if ((blink * 7) % 23 === 0) { X.globalAlpha = 0.3; rect(sx, sy, sw, sh, '#fff'); X.globalAlpha = 1; }
  } else rect(sx, sy, sw, sh, '#303040');
  for (let t = 0; t < 4; t++) {
    const ty = y + 26 + t * 11, tx = x + 6 + t * 4, tw = sx - 8 - tx;
    rect(tx, ty + 6, tw, 4, '#701830');
    if (on) for (let i = 0; i < (u.custY / 6 + t * 2) % (tw / 5 + 1) + 2 && i * 5 + 4 < tw; i++) { rect(tx + 2 + i * 5, ty + 2, 3, 4, '#302838'); rect(tx + 2 + i * 5, ty, 3, 2, '#d0a080'); }
  }
  const by = y + 2 * C.FH;
  rect(x + 3, by + 4, 22, C.FH - 6, '#d0b060'); rect(x + 6, by + 7, 16, 6, '#302010'); glyph('TIX', x + 14, by + 13, '#ffe080', 6);
  person(x + 12, by + 16, '#803030');
}
let TRK = null;
function coasterTrack(W0, H0) {
  if (TRK && TRK.w === W0) return TRK;
  const xs = [], ys = [];
  for (let i = 0; i < 128; i++) {
    let px, py;
    if (i < 16) { px = 10 + i * 4; py = H0 - 14 - (i >= 6 ? (i - 6) * (H0 - 26) / 9 : 0); }
    else if (i < 32) { const k = i - 16; px = 74 + k * 2; py = 12 + (H0 - 26) * (1 - Math.sin(2 * Math.PI * (16 + k * 2) / 64)) / 2; }
    else if (i < 64) { const k = i - 32, r = 26; px = 104 + k / 2 + r * Math.sin(2 * Math.PI * k * 2 / 64); py = H0 - 14 - r + r * Math.sin(2 * Math.PI * (16 + k * 2) / 64); }
    else if (i < 96) { const k = i - 64; px = 122 + k * 2; py = H0 - 34 - 22 * Math.sin(2 * Math.PI * k * 4 / 64); }
    else { const k = i - 96; px = 186 - k * 176 / 31; py = H0 - 8; }
    xs.push(Math.round(px * W0 / 192)); ys.push(Math.round(py));
  }
  return (TRK = { w: W0, xs, ys });
}
function drawCoaster(u, x, y, w, h) {
  const on = u.occ && isOpen(u, hour()), tr = coasterTrack(w, h);
  rect(x, y, w, h - 1, night() || on ? '#101830' : '#283048');
  for (let i = 0; i < 24; i++) rect(x + 4 + (i * 37) % (w - 8), y + 4 + (i * 53) % (h - 30), 1, 1, '#8080c0');
  for (let i = 0; i < 128; i += 6) rect(x + tr.xs[i], y + tr.ys[i], 1, h - 2 - tr.ys[i], '#404860');
  X.strokeStyle = '#e04050'; X.lineWidth = 2; X.beginPath();
  for (let i = 0; i <= 128; i++) { const k = i & 127; i ? X.lineTo(x + tr.xs[k] + 1, y + tr.ys[k] + 1) : X.moveTo(x + tr.xs[k] + 1, y + tr.ys[k] + 1); }
  X.stroke(); X.lineWidth = 1;
  rect(x + 4, y + h - 20, 60, 2, '#808898');
  const head = on ? (blink * (3 + ((blink >> 5) & 1))) & 127 : 8;
  for (let c = 0; c < 4; c++) { const p = (head - c * 3) & 127, cx = x + tr.xs[p] - 2, cy = y + tr.ys[p] - 4; rect(cx, cy, 6, 4, c ? '#f0c020' : '#20b0f0'); if (on) rect(cx + 1, cy - 2, 2, 2, '#f0c0a0'); }
  rect(x + 4, y + 4, 52, 13, '#200828'); X.strokeStyle = '#40f0ff'; X.strokeRect(x + 4.5, y + 4.5, 51, 12);
  glyph('COASTER', x + 30, y + 14, (blink >> 4) & 7 ? '#40f0ff' : '#108090', 8);
  const by = y + 4 * C.FH;
  rect(x + w - 30, by + 4, 26, C.FH - 6, '#303850'); glyph('RIDE', x + w - 17, by + 12, '#ffd040', 7);
  for (let i = 0; i < Math.min(6, u.custY / 8); i++) person(x + w - 36 - i * 5, by + 16, '#5050a0');
}
function drawCarpark(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#4a4e56');
  for (let k = 0; k < u.h; k++) {
    const dy = y + k * C.FH;
    rect(x, dy + C.FH - 3, w, 2, '#7a7e86');
    for (let i = 14; i < w - 22; i += 10) rect(x + i, dy + C.FH - 9, 1, 6, '#f0f0f0');
    for (let j = 0; j < 12; j++) rect((k & 1 ? x + 3 : x + w - 15) + j, dy + C.FH - 3 - (k & 1 ? j : 11 - j) * (C.FH - 4) / 12, 1, 2, '#d0c040');
    for (let i = 0; i < 4; i++) { const cx = x + 16 + i * 10; if (cx + 8 > x + w - 18 || (i + k + u.x) % 3 === 2) continue; rect(cx, dy + C.FH - 8, 8, 4, ['#d03030', '#3060d0', '#e0e0e0', '#303030'][(i + k) & 3]); rect(cx + 2, dy + C.FH - 10, 4, 2, '#90b0d0'); }
    const p = ((blink >> 1) + k * 23) % (w - 8); rect(x + 4 + p, dy + C.FH - 8, 8, 4, '#f0a020'); rect(x + 6 + p, dy + C.FH - 10, 4, 2, '#b0d0f0');
  }
  const gy = y + h - 13; rect(x + w - 6, gy, 3, 10, '#f0d040');
  if (((blink >> 5) & 3) === 0) rect(x + w - 6, gy - 10, 2, 10, '#f04040'); else rect(x + w - 22, gy, 16, 2, '#f04040');
  rect(x + 2, y + 2, 12, 9, '#2050c0'); glyph('P', x + 8, y + 10, '#fff', 8);
}
function drawPool(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#d8e8f0');
  for (let i = 0; i < w - 4; i += 8) rect(x + 2 + i, y + 2, 6, 4, '#9fd8f8');
  const py = y + 22, ph = h - 25;
  rect(x + 6, py, w - 12, ph, '#2080d0');
  for (let i = 0; i < (w - 12) / 6; i++) rect(x + 7 + i * 6 + ((blink >> 2) + i * 3) % 5, py + 2 + (i * 5 + (blink >> 3)) % (ph - 4), 3, 1, '#b0e8ff');
  rect(x + w - 26, py - 3, 18, 2, '#f0f0f0'); rect(x + w - 10, py - 3, 2, ph + 3, '#909090');
  if (u.occ && isOpen(u, hour())) { const j = (blink >> 1) % 16; person(x + w - 22, py - 8 + (j < 8 ? -j / 2 : -4 + (j - 8)), '#e05050'); }
}
function drawIce(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#7090b0');
  for (let i = 0; i < w - 4; i += 10) rect(x + 4 + i, y + 3, 4, 2, '#fff0c0');
  rect(x + 4, y + 20, w - 8, h - 23, '#e8f6ff'); rect(x + 4, y + 18, w - 8, 3, '#f0f0f0'); rect(x + w / 2, y + 21, 1, h - 25, '#d04040');
  const n = u.occ && isOpen(u, hour()) ? 2 + Math.min(6, u.custY / 8 | 0) : 0;
  for (let i = 0; i < n; i++) { const a = ((blink >> 1) + i * 8) / 64 * 2 * Math.PI; person(Math.round(x + w / 2 + Math.cos(a) * (w - 24) / 2), Math.round(y + 30 + Math.sin(a) * 6 + (i & 1) * 4), i & 1 ? '#d03060' : '#3050c0'); }
}
function drawAtrium(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#f0e8d8'); rect(x + w / 2 - 14, y, 28, 6, '#a0d8f0');
  for (let k = 0; k < 4; k++) {
    const dy = y + k * C.FH;
    rect(x, dy + C.FH - 3, 14, 3, '#b0a080'); rect(x + w - 14, dy + C.FH - 3, 14, 3, '#b0a080');
    rect(x + 2, dy + 8, 10, C.FH - 12, k & 1 ? '#f080a0' : '#80c0f0'); rect(x + w - 12, dy + 8, 10, C.FH - 12, k & 1 ? '#90d080' : '#f0c060');
  }
  X.strokeStyle = '#707880';
  for (let k = 0; k < 3; k++) {
    const dy = y + (k + 1) * C.FH, x0 = x + 17, len = w - 34;
    X.beginPath(); if (k & 1) { X.moveTo(x0, dy - 3); X.lineTo(x0 + len, dy - 3 - C.FH); } else { X.moveTo(x0, dy - 3 - C.FH); X.lineTo(x0 + len, dy - 3); } X.stroke();
    const p = ((blink >> 1) + k * 9) % len, yy = k & 1 ? dy - 3 - p * C.FH / len : dy - 3 - (len - p) * C.FH / len;
    person(x0 + p, yy - 7, '#4060a0');
  }
  rect(x + w / 2 - 6, y + h - 9, 12, 6, '#40a060'); rect(x + w / 2 - 1, y + h - 15, 2, 6, '#80e0ff');
}
function drawBanquet(u, x, y, w, h) {
  const on = u.occ && isOpen(u, hour()) && hour() >= 18;
  rect(x, y, w, h - 1, on ? '#9a1c24' : '#6a1820');
  rect(x + w / 2 - 22, y + 12, 44, 22, '#d8a830'); rect(x + w / 2 - 20, y + 14, 40, 18, '#b01818');
  for (let i = 0; i < 9; i++) rect(x + w / 2 - 17 + i * 2, y + 20 + (i * 3) % 5, 2, 2, '#30a040');
  for (let i = 0; i < 7; i++) rect(x + w / 2 + 3 + i * 2, y + 18 + (i * 2) % 4, 2, 2, '#f0d040');
  for (let i = 0; i < 3; i++) { const cx = x + 14 + i * (w - 28) / 2; rect(cx, y, 1, 4, '#d0b060'); rect(cx - 4, y + 5, 9, 4, '#f8e080'); }
  for (let i = 0; i < 5; i++) { const tx = x + 6 + i * (w - 12) / 5; if (tx > x + w / 2 - 26 && tx < x + w / 2 + 22) continue; rect(tx, y + h - 8, 10, 3, '#f0f0e8'); if (on) { person(tx - 2, y + h - 8, '#302030'); person(tx + 9, y + h - 8, '#302030'); } }
}
function drawClimb(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#e0d8c8'); rect(x + 4, y + 4, w - 8, h - 8, '#8a6a4a');
  for (let i = 0; i < 22; i++) rect(x + 6 + (i * 13) % (w - 13), y + 6 + (i * 29) % (h - 14), 3, 2, ['#f04040', '#40c040', '#f0d020', '#4080f0'][i & 3]);
  if (u.occ && isOpen(u, hour())) for (let i = 0; i < 1 + (u.custY > 20); i++) { const up = ((blink >> 3) + i * 17) % (h - 18), cx = x + 10 + i * (w - 22) / 2, cy = y + h - 16 - up; rect(cx + 1, y + 3, 1, cy - y - 3, '#f0f0f0'); person(cx, cy, i ? '#40a0f0' : '#f06040'); }
  rect(x + 2, y + h - 4, w - 4, 2, '#3050a0');
}
function drawGarden(u, x, y, w, h) {
  const nt = night();
  rect(x, y, w, h - 1, nt ? '#18283a' : '#a8d8f0'); rect(x, y + h - 6, w, 5, '#5a9a40');
  for (let i = 0; i < 4; i++) { const tx = x + 8 + i * (w - 30) / 3; rect(tx + 3, y + 24, 2, h - 30, '#6a4a2a'); X.fillStyle = i & 1 ? '#3a8a3a' : '#4aa04a'; X.beginPath(); X.ellipse(tx + 4, y + 20, 6, 7, 0, 0, 7); X.fill(); }
  const wx = x + w - 16; rect(wx, y + 4, 10, 6, '#808890');
  for (let j = 0; j < h - 16; j += 3) rect(wx + 2 + (j + (blink >> 1)) % 3, y + 10 + j, 6, 2, (j / 3 + (blink >> 1)) & 1 ? '#a0e0ff' : '#60b0f0');
  rect(wx - 6, y + h - 8, 18, 4, '#3080c0');
  if (nt) for (let i = 0; i < 6; i++) if (((blink >> 3) + i) % 3) rect(x + 6 + i * 7, y + 8 + (i * 5) % 14, 1, 1, '#f0f080');
}
function drawStudio(u, x, y, w, h) {
  rect(x, y, w, h - 1, '#18181e');
  for (let i = 0; i < w - 8; i += 3) rect(x + 4 + i, y + 3, 2, 3, '#404048');
  for (let i = 0; i < 4; i++) { const lx = x + 12 + i * (w - 24) / 3; rect(lx - 2, y + 6, 5, 4, '#707078'); if (((blink >> 3) + i) % 4) { X.globalAlpha = 0.16; rect(lx - 6 - (i - 2) * 4, y + 10, 12, h - 24, '#fff0b0'); X.globalAlpha = 1; } }
  rect(x + w / 2 - 20, y + 20, 40, 26, '#2a40a0');
  rect(x + w / 2 - 14, y + h - 10, 28, 6, '#c0a060'); person(x + w / 2 - 6, y + h - 14, '#e04040'); person(x + w / 2 + 4, y + h - 14, '#303080');
  for (let i = 0; i < 2; i++) { const cx = i ? x + w - 22 : x + 8; rect(cx, y + h - 18, 12, 7, '#404048'); rect(cx + 5, y + h - 11, 2, 9, '#606068'); }
  rect(x + 3, y + 10, 26, 9, '#e02020'); glyph('ON AIR', x + 16, y + 17, (blink >> 4) & 1 ? '#fff' : '#ffa0a0', 6);
  if (S.celeb) glyph('STAR!', x + w / 2, y + 17, '#ffe040', 7);
}

/* underground challenge art (y = top of the box, h = box height) */
function drawPump(u, x, y, w, h) { rect(x + 1, y + 3, w - 2, h - 4, '#3c5a82'); X.fillStyle = '#2a70b0'; X.beginPath(); X.ellipse(x + 8, y + 13, 5, 5, 0, 0, 7); X.fill(); rect(x + 13, y + 11, w - 15, 3, '#80c0f0'); rect(x + 6 + ((blink >> 1) & 3), y + 12, 3, 2, '#e0f0ff'); txt('PUMP', x + 2, y + 22, '#fff'); }
function drawVent(u, x, y, w, h) { rect(x + 1, y, w - 2, h, '#6e7878'); for (let j = 2; j < h - 2; j += 3) rect(x + 2, y + j, w - 4, 1, '#404848'); rect(x + w / 2 - 4 + ((blink >> 1) & 3) * 2, y + 11, 2, 2, '#fff'); }
function drawMhub(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#d8d0c0'); rect(x + 2, y + 2, w - 4, 10, '#c83c32'); txt('METRO HUB', x + w / 2, y + 10, '#fff', 'center');
  for (let k = 0; k < 2; k++) { const py = y + 16 + k * 26; rect(x + 2, py + 16, w - 4, 3, '#808080');
    let tx = (blink * (k ? 2 : 3)) % (w + 80) - 60; if (k) tx = w - tx - 60;
    for (let c = 0; c < 3; c++) { const l = Math.max(x + 2, x + tx + c * 20), r = Math.min(x + w - 2, x + tx + c * 20 + 18); if (r > l) { rect(l, py + 4, r - l, 12, k ? '#40a040' : '#e0e0e0'); rect(l, py + 7, r - l, 3, '#203040'); } } }
}
function drawSpa(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#7a5040'); for (let i = 0; i < 6; i++) rect(x + 4 + i * (w - 8) / 6, y + 3, 6, 3, '#9a6a50');
  rect(x + 6, y + h - C.FH + 6, w - 12, C.FH - 8, '#60b8c8');
  for (let i = 0; i < 5; i++) rect(x + 10 + i * (w - 20) / 5 + ((blink >> 3) + i) % 3, y + 20 - ((blink >> 2) + i * 5) % 14, 2, 3, '#f0f0f0');
  txt('SPA', x + 3, y + 14, '#ffe0c0');
}
function drawFossil(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#c8b890'); const bx = x + w / 2 - 30, by = y + 22, b = '#f8f0e0';
  rect(bx, by, 14, 8, b); rect(bx + 12, by + 6, 24, 3, b); for (let i = 0; i < 6; i++) rect(bx + 14 + i * 4, by + 9, 1, 8, b);
  rect(bx + 36, by + 7, 18, 2, b); rect(bx + 16, by + 17, 2, 18, b); rect(bx + 28, by + 17, 2, 18, b); rect(bx - 4, y + h - 6, 64, 3, '#806040');
  txt('FOSSILS', x + 3, y + 10, '#604020');
}
function drawGem(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#3a2a4a'); const G = ['#40f0ff', '#ff40a0', '#80ff60', '#ffe040'];
  for (let i = 0; i < 12; i++) rect(x + 10 + (i * 23) % (w - 20), y + 4 + (i * 13) % (h - 12), 2, 2, ((blink >> 2) + i) % 5 ? G[i & 3] : '#fff');
  rect(x + w / 2 - 8, y + h - 9, 16, 5, '#707070');
}
function drawNmarket(u, x, y, w, h) {
  rect(x, y + 1, w, h - 2, '#281820'); const AW = ['#e03030', '#f0c020', '#30a0e0', '#40c060'];
  for (let i = 0; i < Math.floor(w / 20); i++) { const sx = x + 2 + i * 20; rect(sx, y + 3, 18, 4, AW[i & 3]); rect(sx + 2, y + 14, 14, 3, '#c08040');
    for (let j = 0; j < 4; j++) rect(sx + 2 + j * 4, y + 2, 1, 1, ((blink >> 3) + i + j) & 1 ? '#ffe080' : '#806020'); }
}
function drawAqua(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#0c2850'); rect(x + 4, y + 4, w - 8, h - 14, '#1c5ab0');
  for (let i = 0; i < 7; i++) { const fx = x + 6 + ((blink >> 1) * (1 + (i & 1)) + i * 37) % (w - 18), fyy = y + 8 + (i * 11) % (h - 26); rect(fx, fyy, 5, 3, ['#ffa020', '#f0f040', '#f06090'][i % 3]); }
  const sx = x + 4 + (blink >> 2) % (w - 30); rect(sx, y + 30, 22, 7, '#8090a8');
}
function drawLab(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#1e2a2a');
  for (let i = 0; i < 3; i++) { const tx = x + 8 + i * 22; rect(tx, y + 8, 10, h - 14, '#204040'); rect(tx + 2, y + 10, 6, h - 18, ((blink >> 3) + i) & 1 ? '#40ff90' : '#30c070'); }
  rect(x + 3, y + 3, 36, 8, '#f0d020'); txt('LAB 51', x + 4, y + 10, '#101010');
}
function drawGeo(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#3a1a12');
  for (let i = 0; i < 3; i++) { const cx = x + 16 + i * (w - 32) / 2, a = (blink / 8 + i) % 6.28; X.fillStyle = '#808890'; X.beginPath(); X.ellipse(cx, y + 18, 9, 9, 0, 0, 7); X.fill(); rect(cx + 7 * Math.sin(a) - 1, y + 17 + 7 * Math.cos(a), 3, 3, '#202020'); }
  for (let i = 0; i < w - 4; i += 5) rect(x + 2 + i, y + h - 6, 4, 4, ((blink >> 2) + i / 5) & 1 ? '#ff8020' : '#d03010');
}
function drawVault(u, x, y, w, h) {
  rect(x + 1, y + 1, w - 2, h - 2, '#2a2010'); const cx = x + w / 2, cy = y + h / 2;
  X.fillStyle = '#d8b040'; X.beginPath(); X.ellipse(cx, cy, 20, 20, 0, 0, 7); X.fill(); X.fillStyle = '#a08020'; X.beginPath(); X.ellipse(cx, cy, 16, 16, 0, 0, 7); X.fill();
  for (let i = 0; i < 8; i++) { const a = i * 0.785 + blink / 40; rect(cx + 12 * Math.sin(a) - 1, cy + 12 * Math.cos(a) - 1, 3, 3, '#fff0a0'); }
  for (let i = 0; i < 6; i++) { rect(x + 6 + i * 4, y + h - 8 - (i & 1) * 3, 3, 5, '#ffd040'); rect(x + w - 30 + i * 4, y + h - 8 - (i & 1) * 3, 3, 5, '#ffd040'); }
  txt('CORE VAULT', x + 4, y + 10, '#ffe080');
}
const BIG = { pump: drawPump, vent: drawVent, mhub: drawMhub, spa: drawSpa, fossil: drawFossil, gem: drawGem, nmarket: drawNmarket, aqua: drawAqua, lab: drawLab, geo: drawGeo, vault: drawVault, refuge: drawRefuge, mega: drawMega, coaster: drawCoaster, carpark: drawCarpark, pool: drawPool, ice: drawIce, atrium: drawAtrium, banquet: drawBanquet, climb: drawClimb, garden: drawGarden, studio: drawStudio };
function drawUnit(u) {
  const d = T[u.t], x = C.TX + u.x * C.SEG, w = u.w * C.SEG, y = fy(u.f + u.h - 1), h = u.h * C.FH;
  if (u.f + u.h - 1 < S.cam || u.f > S.cam + C.VIS - 1) return;
  if (u.t === 'floor' && u.f < C.BASE && !u.build) { for (let i = 0; i < w; i += 16) rect(x + i, y + 2, 1, h - 4, 'rgba(0,0,0,0.35)'); rect(x, y, w, 1, '#707070'); rect(x, y + h - 1, w, 1, '#404048'); return; }   // dug basement: the rock shows through
  const lit = night() && (u.inside > 0 || COMMERCE(u.t) && isOpen(u, hour()));
  if (u.build) {   // scaffolding + progress bar
    rect(x, y, w, h - 1, '#5a5048');
    for (let i = 0; i < w; i += 6) rect(x + i, y, 1, h - 1, '#c09040');
    for (let j = 3; j < h; j += 5) rect(x, y + j, w, 1, '#c09040');
    const p = 1 - u.build / (u.build0 || 1);
    rect(x + 2, y + h - 4, w - 4, 2, '#222'); rect(x + 2, y + h - 4, Math.max(1, (w - 4) * p), 2, '#ffd040');
    return;
  }
  if (u.t === 'shop') { rect(x, y, w, h, '#2a2a30'); drawShop(u, x, y, w); rect(x, y + h - 1, w, 1, '#222'); return; }
  if (FOOD(u.t)) { rect(x, y, w, h, '#2a2a30'); drawFood(u, x, y, w); rect(x, y + h - 1, w, 1, '#222'); return; }
  if (u.t === 'cinema') { drawCinema(u, x, y, w, h); return; }
  if (BIG[u.t]) { BIG[u.t](u, x, y, w, h); rect(x, y + h - 1, w, 1, '#222'); if (u.fire && blink & 8) rect(x + 2, y + h - C.FH + 2, w - 4, C.FH - 4, '#ff6020'); return; }
  let col = d.col;
  if (TENANT(u.t) && !u.occ) col = '#3a3a44';
  rect(x, y, w, h - 1, col);
  rect(x, y + h - 1, w, 1, '#222');
  switch (u.t) {
    case 'office': for (let i = 0; i < 4; i++) { rect(x + 3 + i * 8, y + 4, 5, 10, lit ? '#fff2a0' : u.occ ? '#2a4a78' : '#555'); if (u.inside && !night() && i < u.inside) person(x + 4 + i * 8, y + 11, '#203050'); } break;
    case 'flat': for (let i = 0; i < 6; i++) rect(x + 4 + i * 10, y + 4, 5, 9, lit ? '#ffd070' : u.occ ? '#7a5a30' : '#555'); rect(x + w - 6, y + 4, 4, 15, '#8a4a2a'); for (let i = 0; i < 3; i++) rect(x + 6 + i * 18, y + 15, 6, 1, '#e0e0e0'); break;
    case 'single': case 'twin': case 'suite': rect(x + 2, y + 12, w - 4, 5, u.roach ? '#4a2a10' : u.dirty ? '#b0a070' : '#fff'); rect(x + 2, y + 10, 3, 2, '#ddd'); if (lit) rect(x + w - 5, y + 2, 3, 3, '#ffd070'); break;
    case 'lobby': for (let i = 0; i < u.w; i += 4) rect(x + i * C.SEG + 2, y + 1, 2, 18, '#a08860'); rect(x, y + h - 3, w, 2, '#b8a070'); break;
    case 'stairs': X.strokeStyle = '#e0d0a0'; X.beginPath(); X.moveTo(x + 2, y + h - 2); X.lineTo(x + w - 2, y - C.FH + 2); X.stroke(); break;
    case 'housek': glyph('HOUSEKEEP', x + w / 2, y + 11, '#223', 7); break;
    case 'security': glyph('GUARD', x + w / 2, y + 11, '#fff', 7); break;
    case 'clinic': rect(x + w / 2 - 1, y + 3, 3, 10, '#e02020'); rect(x + w / 2 - 5, y + 7, 11, 2, '#e02020'); break;
    case 'recycle': glyph('RECYCLE', x + w / 2, y + 20, '#dfd', 8); break;
    case 'parking': for (let i = 0; i < 4; i++) rect(x + 2 + i * 16, y + 8, 10, 6, ['#d03030', '#3050d0', '#e0e0e0', '#30a050'][i]); break;
    case 'metro': rect(x, y + h - 12, w, 7, '#ddd'); rect(x + 10, y + h - 20, 60, 8, '#e04040'); glyph('METRO', x + w / 2, y + 14, '#fff', 9); break;
    case 'chapel': X.fillStyle = '#ffd860'; X.beginPath(); X.moveTo(x + w / 2, y - 6); X.lineTo(x + w / 2 - 24, y + 14); X.lineTo(x + w / 2 + 24, y + 14); X.fill(); glyph('CHAPEL', x + w / 2, y + 34, '#a05020', 9); break;
  }
  if (TENANT(u.t) && u.occ && u.eval < 40) rect(x + w - 3, y + 1, 2, 2, u.eval < 20 ? '#ff2020' : '#ffb020');
  if (u.fire && blink & 8) { rect(x + 2, y + 2, w - 4, h - 4, '#ff6020'); rect(x + 4, y + 4, w - 8, h - 8, '#ffd040'); }
  if (TENANT(u.t) && !u.occ && S.access[u.f]) glyph(u.t === 'flat' ? 'SALE' : 'TO LET', x + w / 2, y + 12, '#aaa', 7);
}
/* ---- lifts: smooth cars, sliding doors, people in and out, LED floor indicator, hall lanterns */
function animate() {
  for (const s of S.shafts) for (const c of s.cars) {
    if (c.stopF >= 0) {                     // a stop happened in the sim since the last frame
      c.holdF = c.stopF; c.hold = 16;
      const vis = c.holdF >= S.cam && c.holdF < S.cam + C.VIS;
      if (vis) {
        for (let i = 0; i < Math.min(3, c.pout); i++) c.anim.push({ out: 1, t: -i * 3 });
        for (let i = 0; i < Math.min(3, c.pin); i++) c.anim.push({ out: 0, t: -i * 3 });
        if (c.anim.length > 6) c.anim.splice(0, c.anim.length - 6);
      }
      c.pin = c.pout = 0; c.stopF = -1;
    }
    const tgt = c.hold > 0 ? c.holdF : c.f, diff = tgt - c.disp;
    if (Math.abs(diff) > 0.02 && c.doorA > 0) c.doorA = Math.max(0, c.doorA - 0.25);   // close the doors, then go
    else if (Math.abs(diff) > 0.02) c.disp += Math.sign(diff) * Math.min(Math.abs(diff), Math.max(0.1, Math.abs(diff) * 0.22));
    else if (Math.abs(diff) <= 0.02) {
      c.disp = tgt;
      if (c.hold > 0) { c.hold--; c.doorA = c.hold > 4 ? Math.min(1, c.doorA + 0.25) : Math.max(0, c.doorA - 0.25); }
    }
    if (c.hold <= 0 && c.doorA > 0) c.doorA = Math.max(0, c.doorA - 0.25);
    for (const a of c.anim) a.t++;
    c.anim = c.anim.filter((a) => a.t < 14);
  }
}
function fyf(fl) { return C.TY + (C.VIS - 1 - (fl - S.cam)) * C.FH; }
function drawShaft(s, si) {
  const x = C.TX + s.x * C.SEG, w = s.w * C.SEG, ex = s.t === 'exp';
  for (let f = Math.max(s.f0, S.cam); f <= Math.min(s.f1, S.cam + C.VIS - 1); f++) {
    const y = fy(f);
    rect(x, y, w, C.FH, ex ? '#1a1e26' : '#262a34');
    rect(x + 1, y, 1, C.FH, '#3c4250'); rect(x + w - 2, y, 1, C.FH, '#3c4250');   // rails
    if (s.build) { for (let j = 0; j < C.FH; j += 4) rect(x, y + j, w, 1, '#c09040'); continue; }
    if (serves(s, f)) {
      rect(x - 1, y + 4, 1, 15, '#888');   // door frame
      const [n, wt] = waiting(s, f);
      let up = false, dn = false;
      for (const g of s.q[f]) { if (g.legs[g.leg].to > f) up = true; else dn = true; }
      X.fillStyle = up ? '#ffd040' : '#444'; X.beginPath(); X.moveTo(x - 4, y + 3); X.lineTo(x - 6, y + 5); X.lineTo(x - 2, y + 5); X.fill();   // hall lanterns
      X.fillStyle = dn ? '#ffd040' : '#444'; X.beginPath(); X.moveTo(x - 4, y + 9); X.lineTo(x - 6, y + 7); X.lineTo(x - 2, y + 7); X.fill();
      for (let i = 0; i < Math.min(n, 5); i++) person(x - 10 - i * 4, y + 13, wt > 3 * C.MIN ? '#ff3030' : wt > 1.5 * C.MIN ? '#ff90c0' : '#202020');
      if (n > 5) txt('+' + (n - 5), x - 30, y + 14, '#fff', 'right');
    }
  }
  if (s.build) return;
  const ys = s.cars.map((c) => Math.round(fyf(c.disp)));
  s.cars.forEach((c, k) => {
    if (c.disp < S.cam - 1 || c.disp > S.cam + C.VIS) return;
    const y = ys[k];
    // cars at the same height share the shaft in lanes; a lone car uses the full width; one indicator per group
    let n = 0, lane = 0, lead = k;
    ys.forEach((yy, j) => { if (Math.abs(yy - y) < C.FH) { n++; if (j < k) { lane++; lead = Math.min(lead, j); } } });
    const lw = Math.floor((w - 2) / n), cx = x + 1 + lane * lw, cw = lane === n - 1 ? w - 2 - lane * lw : lw;
    rect(cx, y + 1, cw, C.FH - 2, '#c8d0e0');
    rect(cx + 1, y + 5, cw - 2, C.FH - 7, '#fff4c0');
    for (let i = 0; i < Math.min(c.load, (cw - 2) / 3 | 0); i++) person(cx + 2 + i * 3, y + 14, '#304060');
    const half = (cw - 2) / 2, op = Math.round(half * c.doorA);
    rect(cx + 1, y + 5, half - op, C.FH - 7, '#9aa4b8'); rect(cx + 1 + half + op, y + 5, half - op, C.FH - 7, '#9aa4b8');
    if (lead === k) {
      rect(x + 1, y - 4, w - 2, 5, '#101010');
      X.font = '5px monospace'; txt(floorName(Math.round(c.disp)) + (n > 1 ? ' x' + n : ''), x + w / 2, y, '#ff8030', 'center'); X.font = '8px monospace';
    }
    for (const a of c.anim) if (a.t >= 0) {                          // boarding / alighting
      const k = a.t / 13, py = Math.round(fyf(c.holdF)) + 16;
      const px = a.out ? x + w / 2 - k * 18 : x - 18 + k * 18 + w / 2;
      person(Math.round(px), py, a.out ? '#2a7a40' : '#304060');
    }
  });
}
function render() {
  blink++;
  X.font = '8px monospace'; X.textBaseline = 'alphabetic';
  if (S.mode === 'title' || (S.mode === 'menu' && S.menu && S.menu.prev === 'title')) { drawTitle(); if (S.mode === 'menu') drawMenu(); return; }
  const [a, b] = skyCol();
  const gr = X.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, a); gr.addColorStop(1, b);
  X.fillStyle = gr; X.fillRect(0, 0, 320, 240);
  // ground and basements
  if (S.cam <= C.BASE) {   // underground: soil, rock, granite, magma
    const yg = fy(C.BASE) + C.FH;
    for (let f = S.cam; f < Math.min(C.BASE, S.cam + C.VIS); f++) {
      const y = fy(f), d = depth(f), b = d <= 30 ? 0 : d <= 60 ? 1 : d <= 85 ? 2 : 3, dug = f >= S.dug;
      const base = ['#5a4030', '#56545a', '#342f3c', '#3a1410'][b];
      rect(0, y, 320, C.FH, base); if (dug) { X.globalAlpha = 0.35; rect(0, y, 320, C.FH, '#8a8478'); X.globalAlpha = 1; }
      for (let i = 0; i < 14; i++) rect((i * 71 + f * 37) % 320, y + 3 + (i * 7 + f * 5) % (C.FH - 6), 3 + (i & 1) * 2, 2, b === 3 ? '#a04020' : '#7a6a58');
      if (!dug) rect(0, y + (f * 7) % (C.FH - 4) + 2, 320, 1, 'rgba(0,0,0,0.4)');
      if (b === 3) for (let i = 0; i < 5; i++) { const cx = (i * 61 + f * 13) % 300, g = ((blink >> 3) + i + f) & 1 ? '#ff7020' : '#c03010'; rect(cx, y + C.FH - 4, 14, 1, g); rect(cx + 5, y + C.FH - 7, 1, 3, g); }
      if (!dug && f === S.dug - 1 && S.digH) {   // the drill
        const p = 1 - S.digH / (S.digH0 || 1), dx = C.TX + 8 + (C.W * C.SEG - 40) * p;
        X.globalAlpha = 0.6; rect(C.TX, y + 4, dx - C.TX, C.FH - 8, '#8a8478'); X.globalAlpha = 1;
        rect(dx, y + 6, 12, C.FH - 12, '#f0c020'); rect(dx + 12, y + 9, 6, 6, '#909090'); rect(dx + 18 + ((blink >> 1) & 1), y + 10, 3, 4, '#d0d0d0');
        txt('DIGGING', Math.max(C.TX + 2, dx - 46), y + 14, '#ffe080');
      }
    }
    if (S.cam < C.BASE) rect(0, fy(S.cam) + C.FH, 320, 240 - fy(S.cam) - C.FH, S.cam === 0 ? '#2a0c08' : '#1a1612');   // below the view: rock (B100: the hot core)
    rect(0, yg, 320, 2, '#6a8a3a');
  }
  if (S.typhoon || S.rainH || S.typhoonSig >= 3)
    for (let i = 0; i < (S.rainH ? 50 : 40); i++) rect((i * 37 + blink * (S.rainH ? 5 : 3)) % 320, (i * 53 + blink * 9) % 220 + 14, 1, S.rainH ? 6 : 5, S.rainH ? '#6080a0' : '#cde');
  if (S.powerH) { X.globalAlpha = 0.35; rect(C.TX, C.TY, C.W * C.SEG, C.VIS * C.FH, '#000018'); X.globalAlpha = 1; txt('POWER CUT', C.TX + 4, C.TY + 10, '#ff8040'); }
  if (S.fest) {
    for (let f = S.cam; f < S.cam + C.VIS; f++) {
      const y = fy(f);
      if (S.fest === 1) for (let i = 0; i < 8; i++) rect(C.TX + 4 + i * 28, y + 2, 2, 2, ((blink >> 2) + i + f) & 1 ? '#ff4040' : '#40ff40');
      else for (let i = 0; i < 4; i++) { const lx = C.TX + 20 + i * 48; rect(lx, y + 2, 6, 8, '#ff6020'); rect(lx + 2, y, 2, 2, '#ffd040'); }
    }
  }
  if (S.santa && !S.powerH) { const sf = S.cam + ((blink >> 3) % C.VIS), y = fy(sf); rect(C.TX + C.W * C.SEG / 2 - 4, y + 4, 8, 10, '#e02020'); rect(C.TX + C.W * C.SEG / 2 - 4, y + 2, 8, 3, '#fff'); }
  // slabs
  for (let f = S.cam; f < S.cam + C.VIS; f++) for (let x = 0; x < C.W; x++) if (S.struct[f * C.W + x] && !S.grid[f * C.W + x]) rect(C.TX + x * C.SEG, fy(f), C.SEG, C.FH, '#56565e');
  for (const u of S.units) drawUnit(u);
  if (night() || hour() >= 18.5) {   // dusk/night: darken the facade, then relight light sources only
    X.globalAlpha = night() ? 0.55 : 0.3; rect(C.TX, C.TY, C.W * C.SEG, C.VIS * C.FH, '#000018'); X.globalAlpha = 1;
    for (const u of S.units) if (!u.build && u.occ && (COMMERCE(u.t) || u.inside > 0 || u.t === 'lobby')) drawUnit(u);
  }
  S.shafts.forEach(drawShaft);
  if (S.floodH && S.floodF >= S.cam && S.floodF < S.cam + C.VIS) {   // flood: water across the floor
    const y = fy(S.floodF), lvl = 6 + (6 - S.floodH) * 2;
    X.globalAlpha = 0.6; rect(C.TX, y + C.FH - lvl, C.W * C.SEG, lvl, '#2060c0'); X.globalAlpha = 1;
    for (let i = 0; i < 12; i++) rect(C.TX + (i * 23 + (blink >> 1)) % (C.W * C.SEG - 4), y + C.FH - lvl, 4, 1, '#a0d0ff');
    txt('FLOOD', C.TX + 4, y + 10, '#80c0ff');
  }
  // floor labels and cursor
  for (let f = S.cam; f < S.cam + C.VIS; f++) {
    const y = fy(f), on = f === S.cur;
    rect(0, y, C.TX - 1, C.FH - 1, on ? '#ffd040' : isLobbyFloor(f) ? '#5a4a30' : '#20202a');
    txt(floorName(f), C.TX - 3, y + 9, on ? '#000' : S.access[f] ? '#ddd' : '#777', 'right');
  }
  const yc = fy(S.cur);
  X.strokeStyle = '#ffd040'; X.lineWidth = 1; X.strokeRect(C.TX - 0.5, yc - 0.5, C.W * C.SEG + 1, C.FH);
  if (S.mode === 'place') drawGhost();
  if (S.mode === 'inspect') drawInspect();
  drawMinimap();
  drawHud();
  if (S.mode === 'menu') drawMenu();
  if (S.mode === 'status') drawStatus();
  if (S.mode === 'news') drawNews();
  if (S.mode === 'end') drawEnd();
}
function drawGhost() {
  const p = S.place, d = T[p.t], x = C.TX + p.x * C.SEG, w = d.w * C.SEG, dh = p.t === 'carpark' ? parkH(p.v) : d.h;
  if (p.stage === 'x') {
    const why = canPlace(p.t, S.cur, p.x, undefined, p.v);
    const y = fy(S.cur + dh - 1);
    X.globalAlpha = 0.7; rect(x, y, w, dh * C.FH, why ? '#ff3030' : d.col); X.globalAlpha = 1;
    X.strokeStyle = '#fff'; X.strokeRect(x + 0.5, y + 0.5, w - 1, dh * C.FH - 1);
  } else {
    const f0 = Math.min(p.f0, p.top), f1 = Math.max(p.f0, p.top), why = canPlace(p.t, f0, p.x, f1);
    for (let f = Math.max(f0, S.cam); f <= Math.min(f1, S.cam + C.VIS - 1); f++) { X.globalAlpha = 0.7; rect(x, fy(f), w, C.FH, why ? '#ff3030' : '#a0c0ff'); X.globalAlpha = 1; }
  }
}
function drawInspect() {
  const it = S.insp.list[S.insp.i];
  const x = C.TX + (it.u ? it.u.x : S.shafts[it.s].x) * C.SEG, w = (it.u ? it.u.w : S.shafts[it.s].w) * C.SEG;
  if (blink & 4) { X.strokeStyle = '#fff'; X.strokeRect(x + 0.5, fy(S.cur) + 0.5, w - 1, C.FH - 1); }
}
function drawMinimap() {
  const x0 = 290, y0 = C.TY, rh = C.VIS * C.FH / C.NF;
  rect(x0 - 2, y0 - 2, 30, C.NF * rh + 4, '#000a');
  for (let f = 0; f < C.NF; f++) {
    let c = null;
    for (const u of S.units) if (f >= u.f && f < u.f + u.h && u.t !== 'stairs') { c = u.occ ? T[u.t].col : '#555'; break; }
    if (!c && S.struct.subarray(f * C.W, f * C.W + C.W).some((v) => v)) c = '#666';
    if (c) rect(x0, y0 + (C.NF - 1 - f) * rh, 26, rh - 1, c);
    if (f === C.BASE) rect(x0 - 2, y0 + (C.NF - 1 - f) * rh + rh - 1, 30, 1, '#6a8a3a');
  }
  X.strokeStyle = '#ffd040'; X.strokeRect(x0 - 1.5, y0 + (C.NF - S.cam - C.VIS) * rh - 0.5, 29, C.VIS * rh);
  rect(x0, y0 + (C.NF - 1 - S.cur) * rh, 26, rh - 1, '#ffd040');
}
function drawHud() {
  rect(0, 0, 320, C.TY - 1, '#000c');
  txt('$' + kfmt(S.funds), 2, 9, S.funds < 0 ? '#ff6060' : '#9f9');
  txt('POP ' + S.pop, 60, 9, '#fff');
  txt(S.coreDone ? 'CORE' : S.star >= 6 ? 'TOWER' : '\u2605'.repeat(S.star) + '\u2606'.repeat(5 - S.star), 118, 9, '#ffd040');
  txt(dateStr() + ' ' + clockStr(), 230, 9, '#fff', 'right');
  txt(['II', '>', '>>', '>>>'][S.speed], 316, 9, '#ffd040', 'right');
  rect(0, 228, 320, 12, '#000c');
  let hint;
  if (S.msgT > 0) { S.msgT--; hint = S.msg; }
  else if (S.mode === 'place') {
    const p = S.place, why = p.stage === 'x' ? canPlace(p.t, S.cur, p.x) : canPlace(p.t, Math.min(p.f0, p.top), p.x, Math.max(p.f0, p.top));
    hint = (p.stage === 'x' ? '\u25b2\u25c0 \u25bc\u25b6 ' : '\u25b2\u25bc TOP FLOOR ') + T[p.t].en + ' $' + kfmt(p.stage === 'x' ? cost(p.t, S.cur) : cost(p.t, Math.min(p.f0, p.top), Math.max(p.f0, p.top))) + (why ? '  ' + why : '  OK BUILD');
  } else if (S.mode === 'inspect') hint = inspectLine(S.insp.list[S.insp.i]);
  else hint = '\u25b2\u25bc FLOOR  OK MENU  HOLD OK ' + (S.speed ? 'PAUSE' : 'RUN');
  txt(hint, 2, 237, '#fff');
}
function inspectLine(it) {
  if (it.s !== undefined) { const s = S.shafts[it.s]; let n = 0; for (let f = s.f0; f <= s.f1; f++) n += waiting(s, f)[0]; return T[s.t].en + ' ' + floorName(s.f0) + '-' + floorName(s.f1) + ' CARS ' + s.cars.length + ' WAITING ' + n; }
  const u = it.u, d = T[u.t];
  if (u.t === 'shop') return 'SHOP: ' + SHOPS[(u.v || 0) % 6].en;
  if (!TENANT(u.t)) return d.zh + ' ' + d.en;
  return d.en + (u.occ ? '' : ' VACANT') + ' EVAL ' + u.eval + ' RENT ' + ['LOW', 'NORM', 'HIGH'][u.rent] + (COMMERCE(u.t) ? ' CUST ' + u.custY : '') + (u.dirty ? ' DIRTY' : '');
}
function panel(x, y, w, h, title) { rect(x, y, w, h, '#101828f0'); X.strokeStyle = '#ffd040'; X.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1); txt(title, x + 6, y + 11, '#ffd040'); }
function drawMenu() {
  const m = S.menu, rows = 14, top = Math.max(0, Math.min(m.sel - 6, m.items.length - rows));
  const h = Math.min(rows, m.items.length) * 12 + 20;
  panel(30, 20, 260, h, m.title);
  for (let i = top; i < Math.min(m.items.length, top + rows); i++) {
    const it = m.items[i], y = 34 + (i - top) * 12;
    if (i === m.sel) rect(32, y - 9, 256, 12, '#ffd04040');
    txt((i === m.sel ? '\u25b6 ' : '  ') + it.label, 36, y, it.dis ? '#777' : '#fff');
    if (it.sub) txt(it.sub, 284, y, it.dis ? '#666' : '#9f9', 'right');
  }
}
function drawStatus() {
  panel(20, 18, 280, 204, 'TOWER STATUS  ' + dateStr());
  let y = 34;
  const line = (s, c) => { txt(s, 28, y, c); y += 11; };
  line('FUNDS $' + kfmt(S.funds) + '   POPULATION ' + S.pop, '#fff');
  line('THIS QUARTER  IN $' + kfmt(S.stats.qIn) + '  OUT $' + kfmt(S.stats.qOut), '#ccc');
  line('LAST OFFICE RENT $' + kfmt(S.stats.rentQ), '#ccc');
  const avg = S.stats.waitN ? S.stats.waitSum / S.stats.waitN / C.MIN : 0;
  line('AVG LIFT WAIT ' + avg.toFixed(1) + ' MIN   MOVING NOW ' + S.groups.length + ' GROUPS', '#ccc');
  y += 4;
  if (S.star < 6) {
    line('NEXT: ' + (S.star === 5 ? 'TOWER' : (S.star + 1) + ' STARS'), '#ffd040');
    for (const [n, ok] of starReqs(S.star + 1)) line((ok ? ' [x] ' : ' [ ] ') + n, ok ? '#9f9' : '#f99');
  } else line('TOWER RATING ACHIEVED!', '#ffd040');
  y += 4;
  const nx = BUILD_ORDER.filter((t) => T[t].star === S.star + 1).map((t) => T[t].en);
  if (nx.length) line('UNLOCKS: ' + nx.join(', ').slice(0, 44), '#adf');
}
function drawNews() {
  panel(10, 18, 300, 204, 'NEWS');
  S.log.slice(0, 16).forEach((l, i) => txt(l.slice(0, 50), 16, 34 + i * 11, i ? '#ccc' : '#fff'));
}
function drawEnd() {
  const FW = ['#ff4060', '#ffd040', '#40e0ff', '#80ff60', '#ff80ff'];
  for (let k = 0; k < 5; k++) {
    const t = (blink >> 1) + k * 13, ph = t % 40, gen = Math.floor(t / 40);
    const cx = 40 + (k * 61 + gen * 37) % 240, cy = 30 + (k * 29 + gen * 17) % 80;
    if (ph < 8) { rect(cx, cy + 60 - ph * 7, 2, 4, '#fff0c0'); continue; }
    const r = (ph - 8) * 1.5; if (r > 34) continue;
    for (let i = 0; i < 16; i++) { const a = i / 16 * 2 * Math.PI; rect(Math.round(cx + r * Math.sin(a)), Math.round(cy + r * Math.cos(a) + r * r / 120), 2, 2, r > 26 ? '#605060' : FW[k]); }
  }
  const core = S.endKind === 2, ask = S.endKind === 1 && !S.deep;
  panel(40, ask ? 108 : 128, 240, ask ? 100 : 80, core ? 'CORE RATING!' : 'TOWER RATING!');
  txt(core ? '\u5730\u5fc3\u5bf6\u5eab  B100 CORE VAULT OPEN' : '\u6469\u5929\u5927\u5ec8  SKYRISE COMPLETE', 52, ask ? 132 : 152, '#ffd040');
  if (ask) {
    txt('DAY ' + (S.day + 1) + '  POP ' + S.pop, 52, 150, '#c0c0d0');
    txt('CONTINUE CHALLENGE?', 52, 164, '#fff'); txt('DIG 100 FLOORS UNDERGROUND', 52, 175, '#ff9050');
    ['YES, DIG!', 'KEEP BUILDING'].forEach((o, i) => { const sel = S.endSel === i; if (sel) rect(50, 181 + i * 11, 220, 10, '#3a3010'); txt((sel ? '> ' : '  ') + o, 54, 189 + i * 11, sel ? '#ffd040' : '#9090a0'); });
    return;
  }
  txt('DAY ' + (S.day + 1) + '  POP ' + S.pop + '  $' + (S.funds >= 1e7 ? Math.floor(S.funds / 1e6) + 'M' : Math.floor(S.funds / 1000) + 'K'), 52, 170, '#c0c0d0');
  txt(core ? 'OK: KEEP PLAYING' : 'OK: KEEP BUILDING', 52, 190, (blink >> 4) & 1 ? '#ffd040' : '#a08020');
}
function drawTitle() {
  const gr = X.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#0a0e2a'); gr.addColorStop(1, '#30285a');
  X.fillStyle = gr; X.fillRect(0, 0, 320, 240);
  for (let i = 0; i < 9; i++) { const h = 40 + ((i * 53) % 90), x = 10 + i * 34; rect(x, 200 - h, 26, h, '#1a1e3a'); for (let k = 0; k < h / 10 - 1; k++) rect(x + 4 + (k % 3) * 7, 205 - h + k * 10, 4, 4, (k + i) % 3 ? '#ffd070' : '#333'); }
  rect(140, 30, 40, 170, '#2a3060'); for (let k = 0; k < 16; k++) rect(144 + (k % 4) * 9, 36 + k * 10, 5, 5, '#ffe090');
  rect(0, 200, 320, 40, '#101020');
  X.font = 'bold 22px sans-serif'; txt('摩天大廈', 160, 70, '#ffd040', 'center');
  X.font = 'bold 12px monospace'; txt('S K Y R I S E', 160, 88, '#fff', 'center');
  X.font = '8px monospace';
  if (!S.menu) titleMenu();
}

/* ---------------------------------------------------------------- save / load (localStorage tower0/tower1 = NVS s0/s1) */
function slotKey(i) { return i ? 'tower1' : 'tower0'; }
function slotInfo(i) {
  try {
    const raw = localStorage.getItem(slotKey(i)); if (!raw) return 'EMPTY';
    const d = JSON.parse(raw); if (!d || !d.v) return 'EMPTY';
    const money = d.funds >= 1e6 ? Math.floor(d.funds / 1e6) + 'M' : Math.floor(d.funds / 1000) + 'K';
    return 'DAY ' + (d.day + 1) + ' *' + d.star + ' $' + money;
  } catch (e) { return 'EMPTY'; }
}
function packSave() {
  return { v: 3, deep: S.deep, coreDone: S.coreDone, dug: S.dug, digH: S.digH, digH0: S.digH0,
    powerH: S.powerH, rainH: S.rainH, quakeH: S.quakeH, alarmH: S.alarmH, brokenSi: S.brokenSi, brokenH: S.brokenH, fest: S.fest, typhoonSig: S.typhoonSig,
    day: S.day, funds: S.funds, star: S.star, vipOK: S.vipOK, towerDone: S.towerDone, wedding: S.wedding, nextId: S.nextId, last: S.last, lastV: S.lastV, speed: S.speed, cur: S.cur, cam: S.cam, snd: S.snd, sound: S.sound, swap: S.swap, rush: S.rush, owned: S.owned,
    units: S.units.map((u) => [TIDX[u.t], u.f, u.x, u.occ, u.eval, u.rent, u.dirty, u.dirtyDays, u.roach, u.custY, u.id, u.w, u.build || 0, u.build0 || 0, u.h]),
    shafts: S.shafts.map((s) => [s.t, s.x, s.f0, s.f1, s.cars.length, s.build || 0, s.build0 || 0]),
    struct: Array.from(S.struct).join('').replace(/(.)\1*/g, (m, c) => c + m.length + ',') };
}
function saveSlot(i) {
  try {
    const d = packSave(); localStorage.setItem(slotKey(i), JSON.stringify(d));
    S.slot = i; localStorage.setItem('tower_slot', String(i));
    return JSON.stringify(d).length;
  } catch (e) { return 0; }
}
function save() { return saveSlot(S.slot || 0); }   // autosave -> last-used
function applySave(d) {
  freshState();
  Object.assign(S, { day: d.day, funds: d.funds, star: d.star, vipOK: d.vipOK, towerDone: d.towerDone, wedding: d.wedding, nextId: d.nextId, last: d.last, lastV: d.lastV, speed: d.speed, cur: d.cur, cam: d.cam, snd: d.snd || 0, sound: d.snd === undefined ? d.sound : d.snd < 2, swap: d.swap, rush: !!d.rush, owned: d.owned || [1, 1, 0, 0], tick: 0, mode: 'play',
    deep: !!d.deep, coreDone: !!d.coreDone, dug: d.dug === undefined ? C.BASE - 4 : d.dug, digH: d.digH || 0, digH0: d.digH0 || 0,
    powerH: d.powerH || 0, rainH: d.rainH || 0, quakeH: d.quakeH || 0, alarmH: d.alarmH || 0, brokenSi: d.brokenSi === undefined ? -1 : d.brokenSi, brokenH: d.brokenH || 0, fest: d.fest || 0, typhoonSig: d.typhoonSig || 0, typhoon: (d.typhoonSig || 0) >= 8 });
  let i = 0; for (const m of d.struct.split(',')) { if (!m) continue; const c = +m[0], n = +m.slice(1); S.struct.fill(c, i, i + n); i += n; }
  for (const a of d.units) {
    const t = TKEYS[a[0]], dd = T[t];
    const u = { id: a[10], t, f: a[1], x: a[2], w: a[11] || dd.w, h: a[14] || dd.h, occ: a[3], eval: a[4], rent: a[5], dirty: a[6], dirtyDays: a[7], roach: a[8], custY: a[9],
      build: a[12] || 0, build0: a[13] || 0, v: a[10] % 6, sSum: 0, sN: 0, inside: t === 'flat' && a[3] ? 3 : 0, transit: 0, cust: 0, inc: 0, booked: 0, lowDays: 0, fire: 0 };
    S.units.push(u);
    for (let ff = u.f; ff < u.f + u.h; ff++) for (let xx = u.x; xx < u.x + u.w; xx++) S.grid[ff * C.W + xx] = u.id + 1;
  }
  for (const a of d.shafts) {
    const sh = { t: a[0], x: a[1], f0: a[2], f1: a[3], w: T[a[0]].w, build: a[5] || 0, build0: a[6] || 0, cars: [], q: [] };
    for (let ff = 0; ff < C.NF; ff++) sh.q.push([]);
    for (let c = 0; c < a[4]; c++) sh.cars.push(newCar(sh.f0));
    S.shafts.push(sh);
  }
  computeAccess(); population();
  return true;
}
function loadSlot(i) {
  let d;
  try { d = JSON.parse(localStorage.getItem(slotKey(i))); } catch (e) { return false; }
  if (!d || (d.v !== 2 && d.v !== 3)) return false;
  S.slot = i; localStorage.setItem('tower_slot', String(i));
  return applySave(d);
}
function load() {
  const slot = +(localStorage.getItem('tower_slot') || 0);
  if (loadSlot(slot)) return true;
  /* migrate legacy single 'tower' key */
  try {
    const raw = localStorage.getItem('tower'); if (!raw) return false;
    localStorage.setItem('tower0', raw); localStorage.removeItem('tower');
    return loadSlot(0);
  } catch (e) { return false; }
}
function newGame() { freshState(); S.mode = 'play'; computeAccess(); note('BUILD A LOBBY ON G/F TO START'); }

/* ---------------------------------------------------------------- main loop: 30 fps */
let running = true, acc = 0, lastT = 0;
function frame() {
  pollKeys();
  musicTick();
  if (['play', 'place', 'inspect'].includes(S.mode) && S.speed) {
    const n = hour() < 6 ? C.NIGHT_TPF : C.TPF[S.speed];
    for (let i = 0; i < n; i++) simTick();
    if (S.celebrate) { S.endKind = S.celebrate === 2 ? 2 : 1; S.endSel = 0; S.celebrate = false; S.mode = 'end'; save(); }   // ending: fireworks
  }
}
function loop(t) {
  if (!running) return;
  acc += Math.min(200, t - (lastT || t)); lastT = t;
  while (acc >= 1000 / 30) { frame(); animate(); acc -= 1000 / 30; }
  render();
  requestAnimationFrame(loop);
}
const KEYMAP = { ArrowUp: 'up', KeyW: 'up', ArrowDown: 'down', KeyS: 'down', Enter: 'ok', Space: 'ok' };
addEventListener('keydown', (e) => { const k = KEYMAP[e.code]; if (k && !e.repeat) { setKey(k, true); e.preventDefault(); } });
addEventListener('keyup', (e) => { const k = KEYMAP[e.code]; if (k) { setKey(k, false); e.preventDefault(); } });
for (const b of document.querySelectorAll('[data-k]')) {
  const k = b.dataset.k;
  b.addEventListener('pointerdown', (e) => { setKey(k, true); e.preventDefault(); });
  b.addEventListener('pointerup', () => setKey(k, false));
  b.addEventListener('pointerleave', () => { if (K[k]) setKey(k, false); });
}
freshState(); seed(Date.now());
requestAnimationFrame(loop);

window.__TW = { dig, digCost, digHours, depth, deepUncovered, ventOK, saveSlot, loadSlot, slotInfo, repairPower, repairLift, shaftLive, titleMenu, C, S, T, SHOPS, wantMusic, musicTick, BUILD_ORDER, place, canPlace, demolish, addCar, route, simTick, frame, render, setKey, save, load, seed, freshState, newGame,
  isRefugeFloor, uncoveredFloor, newDay, population, finishNow, buySpeed, animate, checkStars, starReqs, computeAccess, unitById, floorThings, stop: () => { running = false; }, K };
