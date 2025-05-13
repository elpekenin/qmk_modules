// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * API to draw scrolling text on QP screens.
 */

// -- barrier --

#pragma once

#include <quantum/quantum.h>
#include <quantum/util.h>

#if !defined(QUANTUM_PAINTER_ENABLE)
#    error Quantum painter must be enabled to use scrolling_text
#endif

/**
 * How many scrolling texts can be drawn at the same time.
 */
#ifndef CONCURRENT_SCROLLING_TEXTS
#    define CONCURRENT_SCROLLING_TEXTS 15
#endif

/**
 * Configuration for how to draw a scrolling text.
 */
typedef struct PACKED {
    /**
     * Screen where to draw.
     */
    painter_device_t device;

    /**
     * X coordinate where to draw.
     */
    uint16_t x;

    /**
     * Y coordinate where to draw.
     */
    uint16_t y;

    /**
     * Font to draw with.
     */
    painter_font_handle_t font;

    /**
     * Full text.
     */
    char *str;

    /**
     * Amount of chars being drawn each time.
     */
    uint8_t n_chars;

    /**
     * Time between drawing steps.
     */
    uint32_t delay;

    /**
     * Spaces drawn before repetitions of the text.
     */
    uint8_t spaces;

    /**
     * Foreground color.
     */
    qp_pixel_t fg;

    /**
     * Background color.
     */
    qp_pixel_t bg;
} scrolling_text_config_t;

/**
 * Start a scrolling text.
 *
 * Return: Token of the deferred executor taking care of drawing
 */
deferred_token scrolling_text_start(const scrolling_text_config_t *config);

/**
 * Stop a scrolling text.
 *
 * Args:
 *     scrolling_token: Deferred token returned by the function that started this drawing.
 */
void scrolling_text_stop(deferred_token scrolling_token);

void scrolling_text_extend(deferred_token scrolling_token, const char *str);
