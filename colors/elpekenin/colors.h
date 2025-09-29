// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Custom type to define colors in different ways.
 */

#pragma once

#include <errno.h>

#include "quantum.h"

/**
 * Different ways in which a color can be specified.
 */
typedef enum {
    COLOR_TYPE_NONE,
    /** */
    COLOR_TYPE_RGB,
    /** */
    COLOR_TYPE_HSV,
    /** */
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

#define __RGB(_r, _g, _b) \
    {                     \
        .r = (_r),        \
        .g = (_g),        \
        .b = (_b),        \
    }

/**
 * Create a :c:type:`color_t` instance from a RGB triplet.
 */
#define RGB_COLOR(_rgb...)      \
    {                           \
        .type = COLOR_TYPE_RGB, \
        .rgb  = __RGB(_rgb),    \
    }

#define __HSV(_h, _s, _v) \
    {                     \
        .h = (_h),        \
        .s = (_s),        \
        .v = (_v),        \
    }

/**
 * Create a :c:type:`color_t` instance from a HSV triplet.
 */
#define HSV_COLOR(_hsv...)      \
    {                           \
        .type = COLOR_TYPE_HSV, \
        .hsv  = __HSV(_hsv),    \
    }

#define ___FIRST(x, ...) (x)
#define __FIRST(...) ___FIRST(__VA_ARGS__)

/**
 * Hue values for QMK builtin colors.
 */
enum qmk_hues {
    /** */
    HUE_AZURE = __FIRST(HSV_AZURE),
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
    HUE_YELLOW = __FIRST(HSV_YELLOW),
};

/**
 * Create a :c:type:`color_t` instance from a hue value.
 *
 * .. note::
 *   Sat and val will follow RGB's global settings.
 */
#define HUE(_hue)               \
    {                           \
        .type = COLOR_TYPE_HUE, \
        .hsv =                  \
            {                   \
                .h = (_hue),    \
            },                  \
    }

int get_rgb(color_t color, rgb_t *rgb);
