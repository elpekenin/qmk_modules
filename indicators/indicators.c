// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/indicators.h"

#include <quantum/quantum.h>

static bool should_draw_indicator(const indicator_t *indicator, const indicator_fn_args_t *args) {
    if (indicator->flags & LAYER && indicator->conditions.layer != args->layer) {
        return false;
    }

    if (indicator->flags & KEYCODE && indicator->conditions.keycode != args->keycode) {
        return false;
    }

    if (indicator->flags & MODS && !(indicator->conditions.mods & args->mods)) {
        return false;
    }

    if (indicator->flags & KC_GT_THAN && indicator->conditions.keycode >= args->keycode) {
        return false;
    }

    return true;
}

static uint8_t mod_config_8bit(uint8_t mod) {
    if (keymap_config.swap_lalt_lgui) {
        /** If both modifiers pressed or neither pressed, do nothing
         * Otherwise swap the values
         */
        if (((mod & MOD_BIT(KC_LALT)) && !(mod & MOD_BIT(KC_LGUI))) ||
            (!(mod & MOD_BIT(KC_LALT)) && (mod & MOD_BIT(KC_LGUI)))) {
            mod ^= (MOD_BIT(KC_LALT) | MOD_BIT(KC_LGUI));
        }
    }
    if (keymap_config.swap_ralt_rgui) {
        if (((mod & MOD_BIT(KC_RALT)) && !(mod & MOD_BIT(KC_RGUI))) ||
            (!(mod & MOD_BIT(KC_RALT)) && (mod & MOD_BIT(KC_RGUI)))) {
            mod ^= (MOD_BIT(KC_RALT) | MOD_BIT(KC_RGUI));
        }
    }
    if (keymap_config.swap_lctl_lgui) {
        if (((mod & MOD_BIT(KC_LCTL)) && !(mod & MOD_BIT(KC_LGUI))) ||
            (!(mod & MOD_BIT(KC_LCTL)) && (mod & MOD_BIT(KC_LGUI)))) {
            mod ^= (MOD_BIT(KC_LCTL) | MOD_BIT(KC_LGUI));
        }
    }
    if (keymap_config.swap_rctl_rgui) {
        if (((mod & MOD_BIT(KC_RCTL)) && !(mod & MOD_BIT(KC_RGUI))) ||
            (!(mod & MOD_BIT(KC_RCTL)) && (mod & MOD_BIT(KC_RGUI)))) {
            mod ^= (MOD_BIT(KC_RCTL) | MOD_BIT(KC_RGUI));
        }
    }
    if (keymap_config.no_gui) {
        mod &= ~MOD_BIT(KC_LGUI);
        mod &= ~MOD_BIT(KC_RGUI);
    }

    return mod;
}


bool draw_indicators(uint8_t led_min, uint8_t led_max) {
    uint8_t mods  = get_mods();
    uint8_t layer = get_highest_layer(layer_state);

#ifndef NO_ACTION_ONESHOT
    mods |= get_oneshot_mods();
#endif

    indicator_fn_args_t args = {
        .mods  = mod_config_8bit(mods),
        .layer = layer,
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
            for (int8_t i = 0; i < indicators_count(); ++i) {
                const indicator_t indicator = get_indicator(i);

                if (should_draw_indicator(&indicator, &args)) {
                    rgb_matrix_set_color(args.led_index, indicator.color.r, indicator.color.g, indicator.color.b);
                }
            }
        }
    }

    return false;
}
