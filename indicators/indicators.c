// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/indicators.h"

#include "quantum.h"

static bool should_draw_indicator(const indicator_t *indicator, const indicator_args_t *args) {
    if (indicator->checks.layer && indicator->args.layer != args->layer) {
        return false;
    }

    if (indicator->checks.keycode && indicator->args.keycode != args->keycode) {
        return false;
    }

    if (indicator->checks.mods && !(indicator->args.mods & args->mods)) {
        return false;
    }

    if (indicator->checks.kc_gt_than && indicator->args.keycode >= args->keycode) {
        return false;
    }

    if (indicator->checks.host_leds && !(indicator->args.host_leds & args->host_leds)) {
        return false;
    }

    return true;
}

static uint8_t apply_magic_config(uint8_t mods) {
    const mod_t input  = {.raw = mods};
    mod_t       output = input;

    if (keymap_config.swap_lalt_lgui) {
        output.left_alt = input.left_gui;
        output.left_gui = input.left_alt;
    }

    if (keymap_config.swap_ralt_rgui) {
        output.right_alt = input.right_gui;
        output.right_gui = input.right_alt;
    }

    if (keymap_config.swap_lctl_lgui) {
        output.left_ctrl = input.left_gui;
        output.left_gui  = input.left_ctrl;
    }

    if (keymap_config.swap_rctl_rgui) {
        output.right_ctrl = input.right_gui;
        output.right_gui  = input.right_ctrl;
    }

    if (keymap_config.no_gui) {
        output.left_gui  = false;
        output.right_gui = false;
    }

    return output.raw;
}

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 1, 0);

bool rgb_matrix_indicators_advanced_indicators(uint8_t led_min, uint8_t led_max) {
    rgb_t   rgb;
    uint8_t mods  = get_mods();
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);

#ifndef NO_ACTION_ONESHOT
    mods |= get_oneshot_mods();
#endif

    indicator_args_t args = {
        .mods      = apply_magic_config(mods),
        .layer     = layer,
        .host_leds = host_keyboard_leds(),
    };

    // iterate all keys
    for (int8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (int8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];

            // early exit if out of range
            if (index < led_min || index >= led_max) {
                continue;
            }

            args.led_index = index;
            args.keycode   = keymap_key_to_keycode(layer, (keypos_t){col, row});

            // iterate all indicators
            for (size_t i = 0; i < indicators_count(); ++i) {
                const indicator_t indicator = get_indicator(i);

                if (should_draw_indicator(&indicator, &args)) {
                    const int ret = get_rgb(indicator.color, &rgb);
                    if (ret < 0) {
                        // something went wrong, do nothing
                        continue;
                    }

                    rgb_matrix_set_color(args.led_index, rgb.r, rgb.g, rgb.b);
                }
            }
        }
    }

    return true;
}
