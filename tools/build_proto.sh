#!/usr/bin/env bash
# Concatenates prototype/head.html + p1..p5.js into prototype/index.html
set -euo pipefail
cd "$(dirname "$0")/../prototype"
{ cat head.html p1.js p2.js p3.js p4.js p5.js; printf '</script>\n</body></html>\n'; } > index.html
echo "built prototype/index.html"
