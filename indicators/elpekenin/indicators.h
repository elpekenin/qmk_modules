// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Declaratively define RGB indicators.
 *
 * Your configuration would look like:
 *
 * .. code-block:: c
 *
 *     #include "elpekenin/indicators.h"
 *
 *     const indicator_t PROGMEM indicators[] = {
 *         LAYER_INDICATOR(UTILS, RGB_COLOR(RGB_OFF)),
 *         KEYCODE_IN_LAYER_INDICATOR(QK_BOOT, UTILS, HSV_COLOR(HSV_RED)),
 *         CAPS_LOCK_INDICATOR(HUE(HUE_BLUE)),
 *     };
 *
 * Since indicators are checked (and applied) in the same order that you define them, this config is:
 *   * On the layer ``UTILS``, all LEDs will be off except for ``QK_BOOT`` which will be red
 *   * No indicator to be drawn on other layers (running effect left unchanged)
 */

// -- barrier --

#pragma once

#if !defined(RGB_MATRIX_ENABLE)
#    error RGB matrix must be enabled to use indicators
#endif

#include <errno.h>

#include "quantum.h"

/**
 * .. hint::
 *    In general, you will use the :ref:`convenience macros <indicator-macros>`.
 *    However, you might need to manually instantiate these data structures to define custom conditions.
 */

/**
 * State about the LED being checked.
 *
 * Used to tell whether the indicator's color should be applied.
 */
typedef struct {
    /**
     * Index of the LED.
     */
    uint8_t led_index;
    /**
     * Highest active layer.
     */
    uint8_t layer;
    /**
     * Keycode currently mapped to the key where LED belongs.
     */
    uint16_t keycode;
    /**
     * Active modifiers (bitmask).
     */
    uint8_t mods;
    /**
     * Active host leds.
     */
    uint8_t host_leds;
} indicator_args_t;

/**
 * Different conditions to be checked
 */
typedef struct {
    /**
     * Highest active layer is ``<X>``.
     */
    bool layer : 1;
    /**
     * Keycode is exactly ``<X>``.
     */
    bool keycode : 1;
    /**
     * Modifiers ``<X>`` are active (not an exact match, others can be active too).
     */
    bool mods : 1;
    /**
     * Keycode is greater than ``<X>``.
     */
    bool kc_gt_than : 1;
    /**
     * Host leds ``<x>`` are active (not an exact match, others can be active too).
     */
    bool host_leds : 1;
} indicator_checks_t;

typedef enum {
    NUM_LOCK_MASK    = 1 << 0,
    CAPS_LOCK_MASK   = 1 << 1,
    SCROLL_LOCK_MASK = 1 << 2,
    COMPOSE_MASK     = 1 << 3,
    KANA_MASK        = 1 << 4,
} host_led_mask_t;

typedef enum {
    COLOR_TYPE_NONE,
    COLOR_TYPE_RGB,
    COLOR_TYPE_HSV,
    COLOR_TYPE_HUE,
} color_type_t;

/**
 * Represent colors in different ways.
 */
typedef struct {
    /**
     * How this color is specified: rgb, hsv, just hue, ...
     */
    color_type_t type;

    /**
     * Inner value.
     */
    union {
        rgb_t rgb;
        hsv_t hsv;
    };
} color_t;

/**
 * Create a :c:type:`color_t` instance from a RGB triplet.
 */
#define RGB_COLOR(_rgb)                        \
    (color_t) {                                \
        .type = COLOR_TYPE_RGB, .rgb = {_rgb}, \
    }

/**
 * Create a :c:type:`color_t` instance from a HSV triplet.
 */
#define HSV_COLOR(_hsv)                        \
    (color_t) {                                \
        .type = COLOR_TYPE_HSV, .hsv = {_hsv}, \
    }

#define ___FIRST(x, ...) (x)
#define __FIRST(x) ___FIRST(x)

/**
 * Hue values for QMK builtin colors.
 */
enum qmk_hues {
    /** */
    HUE_AZURE = __FIRST(HSV_AZURE),
    /** */
    HUE_BLACK = __FIRST(HSV_BLACK),
    /** */
    HUE_BLUE = __FIRST(HSV_BLUE),
    /** */
    HUE_CHARTREUSE = __FIRST(HSV_CHARTREUSE),
    /** */
    HUE_CORAL = __FIRST(HSV_CORAL),
    /** */
    HUE_CYAN = __FIRST(HSV_CYAN),
    /** */
    HUE_GOLD = __FIRST(HSV_GOLD),
    /** */
    HUE_GOLDENROD = __FIRST(HSV_GOLDENROD),
    /** */
    HUE_GREEN = __FIRST(HSV_GREEN),
    /** */
    HUE_MAGENTA = __FIRST(HSV_MAGENTA),
    /** */
    HUE_ORANGE = __FIRST(HSV_ORANGE),
    /** */
    HUE_PINK = __FIRST(HSV_PINK),
    /** */
    HUE_PURPLE = __FIRST(HSV_PURPLE),
    /** */
    HUE_RED = __FIRST(HSV_RED),
    /** */
    HUE_SPRINGGREEN = __FIRST(HSV_SPRINGGREEN),
    /** */
    HUE_TEAL = __FIRST(HSV_TEAL),
    /** */
    HUE_TURQUOISE = __FIRST(HSV_TURQUOISE),
    /** */
    HUE_WHITE = __FIRST(HSV_WHITE),
    /** */
    HUE_YELLOW = __FIRST(HSV_YELLOW),
};

/**
 * Create a :c:type:`color_t` instance from a hue value.
 *
 * .. note::
 *   Sat and val will follow RGB's global settings.
 */
#define HUE(_hue)                              \
    (color_t) {                                \
        .type = COLOR_TYPE_HUE, .hsv.h = _hue, \
    }

/**
 * An indicator's specification:
 */
typedef struct PACKED {
    /**
     * Color to be applied if conditions are fulfilled.
     */
    color_t color;
    /**
     * Which conditions have to be checked.
     */
    indicator_checks_t checks;
    /**
     * Values used to check (the ``<X>``\s above).
     */
    indicator_args_t args;
} indicator_t;

/**
 * .. _indicator-macros:
 *
 * Indicator on any key mapped to the given keycode.
 *
 * Args:
 *     _keycode: Value of the keycode.
 *     _color: Color to be applied.
 */
#define KEYCODE_INDICATOR(_keycode, _color) \
    (indicator_t) {                         \
        .color = (_color),                  \
        .checks =                           \
            {                               \
                .keycode = true,            \
            },                              \
        .args = {                           \
            .keycode = (_keycode),          \
        },                                  \
    }

/**
 * Indicator for all LEDs in the given layer.
 *
 * Args:
 *     _layer: Where the indicator should fire.
 *     _color: Color to be applied.
 */
#define LAYER_INDICATOR(_layer, _color) \
    (indicator_t) {                     \
        .color = (_color),              \
        .checks =                       \
            {                           \
                .layer = true,          \
            },                          \
        .args = {                       \
            .layer = (_layer),          \
        },                              \
    }

/**
 * Indicator on any key mapped to the given keycode in the given layer.
 *
 * Args:
 *     _keycode: Value of the keycode.
 *     _layer: Where the indicator should fire.
 *     _color: Color to be applied.
 */
#define KEYCODE_IN_LAYER_INDICATOR(_keycode, _layer, _color) \
    (indicator_t) {                                          \
        .color = (_color),                                   \
        .checks =                                            \
            {                                                \
                .keycode = true,                             \
                .layer   = true,                             \
            },                                               \
        .args = {                                            \
            .keycode = (_keycode),                           \
            .layer   = (_layer),                             \
        },                                                   \
    }

/**
 * Indicator on any key that has been mapped in the given layer (ie: is not ``KC_NO`` nor ``KC_TRNS``) .
 *
 * Args:
 *     _layer: Where the indicator should fire.
 *     _color: Color to be applied.
 */
#define ASSIGNED_KEYCODE_IN_LAYER_INDICATOR(_layer, _color) \
    (indicator_t) {                                         \
        .color = (_color),                                  \
        .checks =                                           \
            {                                               \
                .layer      = true,                         \
                .kc_gt_than = true,                         \
            },                                              \
        .args = {                                           \
            .keycode = KC_TRNS,                             \
            .layer   = (_layer),                            \
        },                                                  \
    }

/**
 * Indicator on any key mapped to the given keycode while mods are active.
 *
 * Args:
 *     _keycode: Value of the keycode.
 *     mod_mask: Bitmask of the modifiers that must be active.
 *     _color: Color to be applied.
 */
#define KEYCODE_WITH_MOD_INDICATOR(_keycode, mod_mask, _color) \
    (indicator_t) {                                            \
        .color = (_color),                                     \
        .checks =                                              \
            {                                                  \
                .keycode = true,                               \
                .mods    = true,                               \
            },                                                 \
        .args = {                                              \
            .keycode = (_keycode),                             \
            .mods    = (mod_mask),                             \
        },                                                     \
    }

/**
 * Indicator on any key mapped to a custom keycode in the given layer.
 *
 * Args:
 *     _layer: Where the indicator should fire.
 *     _color: Color to be applied.
 */
#define CUSTOM_KEYCODE_IN_LAYER_INDICATOR(_layer, _color) \
    (indicator_t) {                                       \
        .color = (_color),                                \
        .checks =                                         \
            {                                             \
                .layer      = true,                       \
                .kc_gt_than = true,                       \
            },                                            \
        .args = {                                         \
            .keycode = QK_USER,                           \
            .layer   = (_layer),                          \
        },                                                \
    }

/**
 * Indicator on any key mapped to the given keycode while host LEDs are active.
 *
 * Args:
 *     _keycode: Value of the keycode.
 *     host_mask: Bitmask of the host LEDs that must be active.
 *     _color: Color to be applied.
 */
#define KEYCODE_WITH_HOST_LED_INDICATOR(_keycode, host_mask, _color) \
    (indicator_t) {                                                  \
        .color = (_color),                                           \
        .checks =                                                    \
            {                                                        \
                .keycode   = true,                                   \
                .host_leds = true,                                   \
            },                                                       \
        .args = {                                                    \
            .keycode   = (_keycode),                                 \
            .host_leds = (host_mask),                                \
        },                                                           \
    }

/**
 * Indicator for KC_CAPS key(s) while caps lock is active.
 *
 * Args:
 *     _color: Color to be applied.
 */
#define CAPS_LOCK_INDICATOR(_color) KEYCODE_WITH_HOST_LED_INDICATOR(KC_CAPS, CAPS_LOCK_MASK, (_color))

/**
 * Indicator for KC_NUM key(s) while num lock is active.
 *
 * Args:
 *     _color: Color to be applied.
 */
#define NUM_LOCK_INDICATOR(_color) KEYCODE_WITH_HOST_LED_INDICATOR(KC_NUM, NUM_LOCK_MASK, (_color))

// not intended to be used by users -> no docstring
size_t      indicators_count(void);
indicator_t get_indicator(size_t index);
