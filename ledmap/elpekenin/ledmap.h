// Copyright 2024 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
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
 *     const uint8_t PROGMEM ledmap[][MATRIX_ROWS][MATRIX_COLS] = {
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
 *
 */

/**
 * ----
 */

// -- barrier --

#pragma once

#include <stdint.h>

#include <quantum/color.h> // rgb_t

/**
 * Available colors
 */
enum colors {
    /** */
    RED        = 0,
    /** */
    ORANGE     = 21,
    /** */
    YELLOW     = 43,
    /** */
    CHARTREUSE = 64,
    /** */
    GREEN      = 85,
    /** */
    SPRING     = 106,
    /** */
    CYAN       = 127,
    /** */
    AZURE      = 148,
    /** */
    BLUE       = 169,
    /** */
    VIOLET     = 180,
    /** */
    MAGENTA    = 201,
    /** */
    ROSE       = 222,

    // special values
    /** */
    TRNS  = 253,
    /** */
    WHITE = 254,
    /** */
    BLACK = 255,

    // has to be equal to the smallest special value
    _MARKER_ = TRNS,
};

// Not intended to be used by users -> no docstring
uint8_t ledmap_layer_count(void);
uint8_t hue_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col);

/**
 * Return the color that has been assigned to a key in the ledmap (applies transparency).
 *
 * Args:
 *     layer: Where to look at.
 *     row: Electrical position of the key in the matrix.
 *     col: Electrical position of the key in the matrix.
 *     rgb: Where the value will be written.
 *
 * Return: Error code.
 *    * ``0``: Color was found, and assigned into pointer.
 *    * ``-EINVAL``: Some input was wrong.
 *    * ``-ENODATA``: ``TRNS`` on layer 0 -> Dont overwrite the existing effect.
 *
 */
int get_ledmap_color(uint8_t layer, uint8_t row, uint8_t col, rgb_t *rgb);

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
 *
 */
void draw_ledmap(uint8_t led_min, uint8_t led_max);
