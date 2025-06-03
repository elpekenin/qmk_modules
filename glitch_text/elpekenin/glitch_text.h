// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * API to draw glitch text on QP screens.
 */

// -- barrier --

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

#if !defined(QUANTUM_PAINTER_ENABLE)
#    error Quantum painter must be enabled to use glitch_text
#endif

#if defined(COMMUNITY_MODULE_ALLOCATOR_ENABLE)
#    define GLITCH_TEXT_USE_ALLOCATOR 1
#    include "elpekenin/allocator.h"
#else
#    define GLITCH_TEXT_USE_ALLOCATOR 0
#endif

/**
 * How many glitch texts can be drawn at the same time.
 */
#ifndef GLITCH_TEXT_N_WORKERS
#    define GLITCH_TEXT_N_WORKERS 15
#endif

/**
 * Time interval between checking works' state (ms)
 */
#ifndef GLITCH_TEXT_TASK_INTERVAL
#    define GLITCH_TEXT_TASK_INTERVAL 10
#endif

/**
 * Callback function for each step of the animation.
 */
typedef void (*callback_fn_t)(const char *, bool);

typedef struct PACKED {
    /**
     * Function to render each animation frame.
     */
    callback_fn_t callback;

    /**
     * Time between drawing steps (ms).
     */
    uint32_t delay;

#if GLITCH_TEXT_USE_ALLOCATOR || defined(__SPHINX__)
    /**
     * Allocator to be used.
     */
    const allocator_t *allocator;
#endif
} glitch_text_config_t;

/**
 * Start glitch animation targeting the given text.
 * For each frame, callback gets invoked with the text to be rendered
 *
 * .. attention::
 *    Text can be at most 64 chars long.
 *
 * Return: Error code
 *    * ``0``: Color was found, and assigned into pointer.
 *    * ``-EINVAL``: Invalid input.
 */
__result_use_check int glitch_text_start(const glitch_text_config_t *config, const char *str);
