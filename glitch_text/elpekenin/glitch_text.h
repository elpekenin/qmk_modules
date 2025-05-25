// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * API to draw glitch text on QP screens.
 */

// -- barrier --

#pragma once

#include <stdbool.h>
#include <stdint.h>

#if !defined(QUANTUM_PAINTER_ENABLE)
#    error Quantum painter must be enabled to use glitch_text
#endif

/**
 * How many glitch texts can be drawn at the same time.
 */
#ifndef CONFIG_GLITCH_TEXT_N_WORKERS
#    define CONFIG_GLITCH_TEXT_N_WORKERS 15
#endif

/**
 * State of a glitch text.
 */
typedef enum {
    /** */
    NOT_RUNNING,
    /** */
    FILLING,
    /** */
    COPYING,
    /** */
    DONE,
} anim_phase_t;

/**
 * Callback function for each step of the animation.
 */
typedef void (*callback_fn_t)(const char *, bool);

#define MAX_TEXT_SIZE 64

/**
 * Information about a glitch text.
 */
typedef struct {
    /**
     * Current animation phase.
     */
    anim_phase_t phase;
    /**
     * Target text: what to draw after animation is complete.
     */
    char dest[MAX_TEXT_SIZE + 1]; // u64 mask + '\0'
    /**
     * Text to display at the moment.
     */
    char curr[MAX_TEXT_SIZE + 1]; // u64 mask + '\0'
    /**
     * Bitmask used internally to control chars to change.
     */
    uint64_t mask;
    /**
     * Length of the string.
     */
    uint8_t len;
    /**
     * User-provided function used to render text.
     */
    callback_fn_t callback;
} glitch_text_state_t;

/**
 * Start glitch animation targeting the given text
 * for each frame, callback gets invoked with the text to be rendered
 *
 * Args:
 *     text: Target string (will be copied).
 *     callback: Function executed each frame to render the new string.
 *
 * .. attention::
 *    Text can be at most 64 chars long.
 *
 * Return: Error code
 *    * ``0``: Color was found, and assigned into pointer.
 *    * ``-EINVAL``: Invalid input.
 */
int glitch_text_start(const char *text, callback_fn_t callback);
