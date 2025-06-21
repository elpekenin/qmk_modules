// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * API to draw scrolling text on QP screens.
 */

// -- barrier --

#pragma once

#include <errno.h>
#include <sys/cdefs.h>

#include "quantum.h"
#include "util.h"

#if !defined(QUANTUM_PAINTER_ENABLE)
#    error Quantum painter must be enabled to use scrolling_text
#endif

#if defined(COMMUNITY_MODULE_ALLOCATOR_ENABLE)
#    define SCROLLING_TEXT_USE_ALLOCATOR 1
#    include "elpekenin/allocator.h"
#else
#    define SCROLLING_TEXT_USE_ALLOCATOR 0
#endif

// How many scrolling texts can be drawn at the same time.
#ifndef SCROLLING_TEXT_N_WORKERS
#    define SCROLLING_TEXT_N_WORKERS 15
#endif

// Time interval between checking works' state (ms)
#ifndef SCROLLING_TEXT_TASK_INTERVAL
#    define SCROLLING_TEXT_TASK_INTERVAL 10
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
     * Amount of chars being drawn each time.
     */
    size_t n_chars;

    /**
     * Time between drawing steps.
     */
    uint32_t delay;

    /**
     * Spaces drawn before repetitions of the text.
     */
    size_t spaces;

    /**
     * Foreground color.
     */
    hsv_t fg;

    /**
     * Background color.
     */
    hsv_t bg;

#if SCROLLING_TEXT_USE_ALLOCATOR || defined(__SPHINX__)
    /**
     * Allocator to be used.
     */
    const allocator_t *allocator;
#endif
} scrolling_text_config_t;

/**
 * Start a scrolling text.
 *
 * Return: Token of the deferred executor taking care of drawing
 */
__result_use_check deferred_token scrolling_text_start(const scrolling_text_config_t *config, const char *str);

/**
 * Stop a scrolling text.
 *
 * Args:
 *     scrolling_token: Deferred token returned by the function that started this drawing.
 */
void scrolling_text_stop(deferred_token scrolling_token);

void scrolling_text_extend(deferred_token scrolling_token, const char *str);
