#!/usr/bin/env bash
# Host harness: builds the firmware's own tower / ui / draw / paint / gfx code
# for the PC, runs the checks, and writes screenshots to docs/. Needs cc + python3.
set -euo pipefail
cd "$(dirname "$0")/../.."
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -D_POSIX_C_SOURCE=199309L -DGFX_COUNT_PIXELS -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -Itools/host/stub -Imain \
    tools/host/test.c main/tower.c main/ui.c main/draw.c main/paint.c main/gfx.c main/assets.c main/cjk.c -o "$TMP/test"
"$TMP/test" "$TMP"
python3 tools/host/to_png.py "$TMP" docs >/dev/null
echo "screenshots: $(ls docs/*.png | wc -l) in docs/"
