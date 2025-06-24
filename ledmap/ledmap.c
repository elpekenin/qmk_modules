// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ledmap.h"

#include "quantum.h"

// clang-format off
static const uint8_t color_to_hue[LEDMAP_SPECIAL_SEPARATOR] = {
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
    const layer_state_t layers_stack = layer_state | default_layer_state;
    const layer_state_t layer_mask   = ((layer_state_t)1) << layer;

    // out of range or inactive layer
    if (layer >= ledmap_layer_count() || !(layers_stack & layer_mask)) {
        return Err(rgb_t, int, -EINVAL);
    }

    ledmap_color_t color = color_at_ledmap_location(layer, row, col);

    hsv_t hsv = (hsv_t){
        .s = rgb_matrix_get_sat(),
        .v = rgb_matrix_get_val(),
    };

    if (color < LEDMAP_SPECIAL_SEPARATOR) {
        hsv.h = color_to_hue[color];
    } else {
        switch (color) {
            case TRNS:
                if (layer == 0) {
                    return Err(rgb_t, int, -ENODATA);
                }

                // look up further down (only on active layers)
                for (uint8_t i = layer - 1; i > 0; --i) {
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

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 1, 0);

bool rgb_matrix_indicators_advanced_ledmap(uint8_t led_min, uint8_t led_max) {
    layer_state_t layer = get_highest_layer(layer_state | default_layer_state);

    // iterate all keys
    for (size_t row = 0; row < MATRIX_ROWS; ++row) {
        for (size_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];

            if (index < led_min || index >= led_max) {
                continue;
            }

            Result(rgb_t, int) rgb_result = rgb_at_ledmap_location(layer, row, col);
            if (rgb_result.is_ok) {
                rgb_t rgb = unwrap(rgb_result);
                rgb_matrix_set_color(index, rgb.r, rgb.g, rgb.b);
            }
        }
    }

    return true;
}
