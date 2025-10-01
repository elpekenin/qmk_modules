// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "compiler_support.h"
#include "progmem.h"

#define NUM_LEDMAP_LAYERS_RAW ARRAY_SIZE(ledmap)

STATIC_ASSERT(NUM_KEYMAP_LAYERS_RAW == NUM_LEDMAP_LAYERS_RAW, "Number of ledmap layers doesn't match the number of keymap layers");

uint8_t ledmap_layer_count(void) {
    return NUM_LEDMAP_LAYERS_RAW;
}

color_t color_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col) {
    color_t value;
    memcpy_P(&value, &ledmap[layer][row][col], sizeof(color_t));
    return value;
}
