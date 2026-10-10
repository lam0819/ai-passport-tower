#!/usr/bin/env bash
# Renders prototype screenshots of the Underground challenge into docs/proto_*.png
set -euo pipefail
cd "$(dirname "$0")/.."
CHROME=${CHROME:-}
for c in "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" google-chrome chromium chromium-browser; do
    [ -n "$CHROME" ] && break
    if command -v "$c" >/dev/null 2>&1 || [ -x "$c" ]; then CHROME=$c; fi
done
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cp tools/proto_shots.js "$TMP/"
sed 's#</body>#<script src="proto_shots.js"></script></body>#' prototype/index.html > "$TMP/t.html"
"$CHROME" --headless=new --disable-gpu --no-sandbox --user-data-dir="$TMP/profile" --allow-file-access-from-files \
    --virtual-time-budget=20000 --dump-dom "file://$TMP/t.html" 2>/dev/null | python3 -c '
import sys, re, html, base64
d = sys.stdin.read(); m = re.findall(r"<div id=\"out\">(.*?)</div>", d, re.S)
lines = [l for l in html.unescape(m[-1] if m else "").split("\n") if " data:image/png;base64," in l]
for l in lines:
    n, u = l.split(" ", 1); open("docs/" + n + ".png", "wb").write(base64.b64decode(u.split(",", 1)[1])); print("docs/" + n + ".png")
print(html.unescape(m[-1])[:300] if m and not lines else ""); sys.exit(0 if lines else 1)'
