#!/usr/bin/env bash
# Runs tools/proto_test.js against prototype/index.html in headless Chrome
# (fresh profile, so nothing saved before). Fails if a check fails.
#
# The report is POSTed to a tiny local HTTP server instead of relying on
# --dump-dom: the test can run for minutes of real CPU (long simulations),
# and dump-dom can fire before the synchronous script finishes.
set -euo pipefail
cd "$(dirname "$0")/.."
CHROME=${CHROME:-}
for c in "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" google-chrome chromium chromium-browser; do
    [ -n "$CHROME" ] && break
    if command -v "$c" >/dev/null 2>&1 || [ -x "$c" ]; then CHROME=$c; fi
done
[ -n "$CHROME" ] || { echo "no Chrome found (set CHROME=...)" >&2; exit 1; }
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"; [ -n "${SRV:-}" ] && kill "$SRV" 2>/dev/null || true' EXIT
sed 's#</body>#<script src="proto_test.js"></script>\
<script>setTimeout(function(){fetch("http://127.0.0.1:8777/report",{method:"POST",body:document.getElementById("out").textContent})},100)</script></body>#' prototype/index.html > "$TMP/t.html"
cp tools/proto_test.js "$TMP/"
cat > "$TMP/server.py" <<'PY'
from http.server import BaseHTTPRequestHandler, HTTPServer
import sys
class H(BaseHTTPRequestHandler):
    def do_POST(self):
        n = int(self.headers.get('content-length', 0))
        open(sys.argv[1], 'w').write(self.rfile.read(n).decode('utf-8'))
        sys.exit(0)
    def log_message(self, *a): pass
HTTPServer(('127.0.0.1', 8777), H).serve_forever()
PY
python3 "$TMP/server.py" "$TMP/report.txt" 2>/dev/null &
SRV=$!
sleep 1
# --headless=old: --virtual-time-budget hangs the x64-under-Rosetta build on this
# machine; plain old headless works and the report arrives via the POST above.
# Chrome never exits by itself, so poll for the report, then kill it.
timeout 900 "$CHROME" --headless=old --disable-gpu --no-sandbox --user-data-dir="$TMP/profile" \
    --allow-file-access-from-files --no-first-run --no-default-browser-check "file://$TMP/t.html" >/dev/null 2>&1 &
CHROME_PID=$!
for _ in $(seq 1 900); do [ -s "$TMP/report.txt" ] && break; sleep 1; done
kill "$CHROME_PID" "$SRV" 2>/dev/null || true
[ -f "$TMP/report.txt" ] || { echo "no report" >&2; exit 1; }
cat "$TMP/report.txt"
[ "$(tail -c 40 "$TMP/report.txt" | tr -d '[:space:]')" = "allcheckspassed" ]