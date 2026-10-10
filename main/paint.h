// Dirty-region painting, shared by the device loop and the host harness.
// Each frame: per floor row, per lift shaft window, HUD+hint, minimap and
// overlay get a signature; only regions whose signature changed are
// composed (in <= 320x20 bands) and sent.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef void (*paint_send_t)(int x, int y, int w, const uint16_t *buf, int rows);
// Returns the number of pixels sent this frame.
int paint_frame(paint_send_t send, bool full);
