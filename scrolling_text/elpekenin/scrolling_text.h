// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * API to draw scrolling text on QP screens.
 */

// -- barrier --

#pragma once

#include <quantum/quantum.h>

/**
 * How many scrolling texts can be drawn at the same time.
 */
#ifndef CONCURRENT_SCROLLING_TEXTS
#    define CONCURRENT_SCROLLING_TEXTS 15
#endif

/**
 * Information about a scrolling text.
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
     * Position (offset) of first char to be drawn.
     */
    uint8_t char_number;

    /**
     * Spaces drawn before repetitions of the text.
     */
    uint8_t spaces;

    /**
     * Pixel width of current text, used to clean before next draw.
     */
    int16_t width;

    /**
     * Foreground color.
     */
    qp_pixel_t fg;

    /**
     * Background color.
     */
    qp_pixel_t bg;

    /**
     * Identifier of the task drawing this scrolling text.
     */
    deferred_token defer_token;
} scrolling_text_state_t;

/**
 * Start a scrolling text.
 *
 * Args:
 *     device: Display in which to draw
 *     x, y: Coordinates where to draw
 *     font: Font to be used
 *     str: Text to be written
 *     n_chars: How many chars to be written at any time
 *     delay: Time between animation steps (in ms)
 *
 * Return: Token of the deferred executor taking care of drawing
 */
deferred_token draw_scrolling_text(painter_device_t device, uint16_t x, uint16_t y, painter_font_handle_t font, const char *str, uint8_t n_chars, uint32_t delay);

/**
 * Start a scrolling text with custom colors.
 *
 * Args:
 *     device: Display in which to draw
 *     x, y: Coordinates where to draw
 *     font: Font to be used
 *     str: Text to be written
 *     n_chars: How many chars to be written at any time
 *     delay: Time between animation steps (in ms)
 *     hue_fg, sat_fg, val_fg: Foreground (font) color
 *     hue_bg, sat_bg, val_bg: Background color
 *
 * Return: Token of the deferred executor taking care of drawing
 */
deferred_token draw_scrolling_text_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_handle_t font, const char *str, uint8_t n_chars, uint32_t delay, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg);

/**
 * Stop a scrolling text.
 *
 * Args:
 *     scrolling_token: Deferred token returned by the function that started this drawing.
 */
void stop_scrolling_text(deferred_token scrolling_token);

void extend_scrolling_text(deferred_token scrolling_token, const char *str);
