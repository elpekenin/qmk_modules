// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ledmap.h"

#include <errno.h>
#include <quantum/quantum.h>

// clang-format off
static const uint8_t color_to_hue[__LEDMAP_SEPARATOR__] = {
    [RED]        = 0,
    [ORANGE]     = 21,
    [YELLOW]     = 43,
    [CHARTREUSE] = 64,
    [GREEN]      = 85,
    [SPRING]     = 106,
    [CYAN]       = 127,
    [AZURE]      = 148,
    [BLUE]       = 169,
    [VIOLET]     = 180,
    [MAGENTA]    = 201,
    [ROSE]       = 222,
};
// clang-format on

Result(rgb_t, int) rgb_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col) {
    // clang-format off
    if (
        layer >= ledmap_layer_count() // out of range
        || !(layer_state & (1 << layer)) // layer is not active
    ) {
        // clang-format on
        return Err(rgb_t, int, -EINVAL);
    }

    ledmap_color_t color = color_at_ledmap_location(layer, row, col);

    hsv_t hsv = (hsv_t){
        .s = rgb_matrix_get_sat(),
        .v = rgb_matrix_get_val(),
    };

    if (color < __LEDMAP_SEPARATOR__) {
        hsv.h = color_to_hue[color];
    } else {
        switch (color) {
            case TRNS:
                if (layer == 0) {
                    return Err(rgb_t, int, -ENODATA);
                }

                // look up further down (only on active layers)
                for (int8_t i = layer - 1; i > 0; --i) {
                    if (layer_state & (1 << i)) {
                        return rgb_at_ledmap_location(i, row, col);
                    }
                }

                return Err(rgb_t, int, -EINVAL);

            case WHITE:
                hsv.h = 0;
                hsv.s = 0;
                break;

            case BLACK:
                hsv = (hsv_t){
                    .h = 0,
                    .s = 0,
                    .v = 0,
                };
                break;

            default:
                return Err(rgb_t, int, -ENOTSUP);
        }
    }

    return Ok(rgb_t, int, hsv_to_rgb(hsv));
}

void draw_ledmap(uint8_t led_min, uint8_t led_max) {
    layer_state_t layer = get_highest_layer(layer_state | default_layer_state);

    // iterate all keys
    for (int8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (int8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];

            if (index < led_min || index >= led_max) {
                continue;
            }

            Result(rgb_t, int) result = rgb_at_ledmap_location(layer, row, col);
            if (result.is_ok) {
                rgb_t rgb = unwrap(result);
                rgb_matrix_set_color(index, rgb.r, rgb.g, rgb.b);
            }
        }
    }
}
