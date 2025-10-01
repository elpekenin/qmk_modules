// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Define a static, per-key, RGB matrix design.
 *
 * It works similar to keymaps, mapping a color to each key and allowing transparency.
 *
 * Hackery is needed (re-defining ``XXX``) so that ``LAYOUT`` can correctly fill
 * un-used matrix spots with values of type ``color_t``. See example:
 *
 * .. code-block:: c
 *
 *     #include "elpekenin/ledmap.h"
 *
 *     // short aliases
 *     #define BLACK RGB_COLOR(RGB_BLACK)
 *     #define CYAN HUE_COLOR(HUE_CYAN)
 *     #define RED HSV_COLOR(HSV_RED)
 *     #define TRNS TRNS_COLOR
 *     #define WHITE WHITE_COLOR
 *
 *     // make LAYOUT work
 *     #undef XXX
 *     #define XXX NONE_COLOR
 *
 *     const color_t PROGMEM ledmap[][MATRIX_ROWS][MATRIX_COLS] = {
 *         [QWERTY] = LAYOUT(
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,  RED,  RED,     RED,  RED,  RED,  RED,  RED,  RED,
 *             RED,  RED,  RED,  RED,    BLACK,         WHITE,    RED,  TRNS, RED,  RED
 *         ),
 *         [FN] = LAYOUT(
 *             TRNS, TRNS, TRNS, TRNS, TRNS, TRNS,    TRNS, TRNS, TRNS, TRNS, TRNS, TRNS,
 *             CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,    CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,
 *             CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,    CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,
 *             CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,    CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,
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

#include <errno.h>

#include "compiler_support.h"
#include "quantum.h"

#if !defined(RGB_MATRIX_ENABLE)
#    error RGB matrix must be enabled to use ledmap
#endif

#if defined(COMMUNITY_MODULE_COLORS_ENABLE)
#    include "elpekenin/colors.h"
#else
#    error Must enable 'elpekenin/colors'
#endif

#if defined(COMMUNITY_MODULE_INDICATORS_ENABLE)
#    pragma message "Enable indicators after ledmap, otherwise you will overwrite them."
#endif

/**
 * Retrieve the color assigned to a key in the ledmap (transparency gets applied).
 *
 * Args:
 *     layer: Where to look at.
 *     row: Electrical position of the key in the matrix.
 *     col: Electrical position of the key in the matrix.
 *     rgb: Where the value will be written.
 *
 * Return: Result of the operation.
 *     * ``0``: Color was retrieved.
 *     * ``-EINVAL``: Some input was wrong.
 *     * ``-ENODATA``: ``TRNS`` on layer 0 -> Dont overwrite the existing effect.
 *     * Further details on :c:func:`to_rgb`
 */
int rgb_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col, rgb_t *rgb);

// Not intended to be used by users -> no docstring
uint8_t ledmap_layer_count(void);

color_t color_at_ledmap_location(uint8_t layer, uint8_t row, uint8_t col);
