// Copyright 2024 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <errno.h>

#include <quantum/action_layer.h>
#include <quantum/rgb_matrix/rgb_matrix.h>

#include "elpekenin/ledmap.h"

static inline bool is_special_color(uint8_t hue) {
    return hue >= _MARKER_;
};

int get_ledmap_color(uint8_t layer, uint8_t row, uint8_t col, rgb_t *rgb) {
    if (
        layer >= ledmap_layer_count()  // out of range
        || !(layer_state & (1 << layer))  // layer is not active
        || rgb == NULL  // invalid pointer
    ) {
        return -EINVAL;
    }

    uint8_t hue = hue_at_ledmap_location(layer, row, col);
    uint8_t sat = rgb_matrix_get_sat();
    uint8_t val = rgb_matrix_get_val();
    hsv_t   hsv = {hue, sat, val};

    // not "regular" colors (hue), but ones with special handling
    if (is_special_color(hue)) {
        switch (hue) {
            case TRNS:
                if (layer == 0) {
                    return -ENODATA;
                }

                // look up further down (only on active layers)
                for (int8_t i = layer - 1; i > 0; --i) {
                    if (layer_state & (1 << i)) {
                        return get_ledmap_color(i, row, col, rgb);
                    }
                }

                return -EINVAL;

            case WHITE:
                hsv = (hsv_t){0, 0, val};
                break;

            case BLACK:
                hsv = (hsv_t){0, 0, 0};
                break;
        }
    }

    *rgb = hsv_to_rgb(hsv);
    return 0;
}

void draw_ledmap(uint8_t led_min, uint8_t led_max) {
    layer_state_t layer = get_highest_layer(layer_state | default_layer_state);
    rgb_t rgb = (rgb_t){RGB_OFF};

    // iterate all keys
    for (int8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (int8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];

            if (index < led_min || index >= led_max) {
                continue;
            }

            if (get_ledmap_color(layer, row, col, &rgb) == 0) {
                rgb_matrix_set_color(index, rgb.r, rgb.g, rgb.b);
            }
        }
    }
}
