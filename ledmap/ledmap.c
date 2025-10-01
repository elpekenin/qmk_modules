// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ledmap.h"

#include "quantum.h"

#ifndef __warn_unused
#    define __warn_unused __attribute__((__warn_unused_result__))
#endif

__warn_unused static int rgb_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col, rgb_t *rgb) {
    const layer_state_t layers_stack = layer_state | default_layer_state;
    const layer_state_t layer_mask   = ((layer_state_t)1) << layer;

    // out of range or inactive layer
    if (layer >= ledmap_layer_count() || !(layers_stack & layer_mask)) {
        return -EINVAL;
    }

    const color_t color = color_at_ledmap_location(layer, row, col);
    if (color.type == COLOR_TYPE_TRNS) {
        // transparency on layer 0 -> nothing to do
        if (layer == 0) {
            return -ENODATA;
        }

        // look further down (only on active layers)
        for (uint8_t i = layer - 1; i > 0; --i) {
            if (layer_state & (1 << i)) {
                return rgb_at_ledmap_location(i, row, col, rgb);
            }
        }

        // ran out of loop and found nothing
        return -EINVAL;
    }

    // non-trans color handled by just "unwrapping" the RGB in it
    return to_rgb(color, rgb);
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

            rgb_t rgb;
            if (rgb_at_ledmap_location(layer, row, col, &rgb) == 0) {
                rgb_matrix_set_color(index, rgb.r, rgb.g, rgb.b);
            }
        }
    }

    return true;
}
