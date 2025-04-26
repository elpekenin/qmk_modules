// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Define a static, per-key, RGB matrix design.
 *
 * It works similar to keymaps, mapping a color to each key and allowing transparency.
 *
 * Your configuration would end up looking like:
 *
 * .. code-block:: c
 *
 *     const ledmap_color_t PROGMEM ledmap[][MATRIX_ROWS][MATRIX_COLS] = {
 *         [_QWERTY] = LAYOUT(
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,    BLACK,         WHITE,    RED,  TRNS, RED,  RED
 *         ),
 *         [_FN] = LAYOUT(
 *             TRNS, TRNS, TRNS, TRNS, TRNS, TRNS,    TRNS, TRNS, TRNS, TRNS, TRNS, TRNS,
 *             CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,    CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,
 *             BLUE, BLUE, BLUE, BLUE, BLUE, BLUE,    BLUE, BLUE, BLUE, BLUE, BLUE, BLUE,
 *             ROSE, ROSE, ROSE, ROSE, ROSE, ROSE,    ROSE, ROSE, ROSE, ROSE, ROSE, ROSE,
 *             WHITE,WHITE,BLACK,TRNS,    BLACK,         BLACK,   RED,  TRNS, WHITE,WHITE
 *         ),
 *      };
 *
 * .. warning::
 *    Due to reusing ``LAYOUT`` macro to define the colors, implementation is not too flexible.
 *      * Assumes existence of a LED under every key.
 *      * Does not support assigning colors to LEDs that aren't under a key (eg: indicators or underglow)
 *
 *    It also hasn't been exhaustively tested, there might be some problems.
 */

// -- barrier --

#pragma once

#include <quantum/quantum.h>

#if !defined(RGB_MATRIX_ENABLE)
#    error RGB matrix must be enabled to use ledmap
#endif

#if defined(COMMUNITY_MODULE_TYPES_ENABLE)
#    include "elpekenin/types.h"
#else
#    error Must enable 'elpekenin/types'
#endif

/**
 * Available colors
 */
typedef enum {
    // `LAYOUT` puts `KC_NO` in un-used spots
    // we want to prevent collisions with it
    NONE = KC_NO,

    /** */
    RED,
    /** */
    ORANGE,
    /** */
    YELLOW,
    /** */
    CHARTREUSE,
    /** */
    GREEN,
    /** */
    SPRING,
    /** */
    CYAN,
    /** */
    AZURE,
    /** */
    BLUE,
    /** */
    VIOLET,
    /** */
    MAGENTA,
    /** */
    ROSE,

    __LEDMAP_SEPARATOR__,

    /** */
    TRNS,
    /** */
    WHITE,
    /** */
    BLACK,

    __N_LEDMAP_COLORS__,
} ledmap_color_t;

_Static_assert(__N_LEDMAP_COLORS__ <= UINT8_MAX, "Update code, reading uses `pgm_read_byte`");

ResultImpl(rgb_t, int);

/**
 * Retrieve the color assigned to a key in the ledmap (transparency gets applied).
 *
 * Args:
 *     layer: Where to look at.
 *     row: Electrical position of the key in the matrix.
 *     col: Electrical position of the key in the matrix.
 *     rgb: Where the value will be written.
 *
 * Return:
 *    Result of the operation.
 *       * Ok(rgb): Color was retrieved. Use :c:func:`unwrap` to get the value.
 *       * Err(val): Something went wrong. Use :c:func:`unwrap_err` to get the value. Possible values:
 *          * ``-EINVAL``: Some input was wrong.
 *          * ``-ENODATA``: ``TRNS`` on layer 0 -> Dont overwrite the existing effect.
 *          * ``-ENOTSUP``: Unknown value read (not a value in :c:enum:`ledmap_color_t`).
 */
Result(rgb_t, int) rgb_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col);

/**
 * Assign their color to all leds in the given range.
 *
 * Args:
 *     led_min: First LED to be drawn.
 *     led_max: Last LED to be drawn.
 *
 * .. warning::
 *    Since QMK does not (yet?) allow to hook modules into the RGB system,
 *    you need to call this function from within ``rgb_matrix_indicators_advanced_user``
 */
void draw_ledmap(uint8_t led_min, uint8_t led_max);

// Not intended to be used by users -> no docstring
uint8_t ledmap_layer_count(void);

ledmap_color_t color_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col);
