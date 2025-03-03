// Copyright 2024 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>

#include <platforms/progmem.h>

#define NUM_LEDMAP_LAYERS_RAW (sizeof(ledmap) / (MATRIX_ROWS * MATRIX_COLS))

_Static_assert(NUM_KEYMAP_LAYERS_RAW == NUM_LEDMAP_LAYERS_RAW, "Number of ledmap layers doesn't match the number of keymap layers");

uint8_t ledmap_layer_count(void) {
    return NUM_LEDMAP_LAYERS_RAW;
}

uint8_t hue_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col) {
    return pgm_read_byte(&(ledmap[layer][row][col]));
}
