// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Utilities to check why (if) last execution crashed.
 *
 * .. warning::
 *    Only works on Cortex-M microcontrollers, but apparently, not on M4F.
 */

// -- barrier --

#pragma once

#include <backtrace.h>
#include <stdbool.h>
#include <stddef.h>

// How big the array to store a message will be.
#ifndef CRASH_MESSAGE_LENGTH
#    define CRASH_MESSAGE_LENGTH (200)
#endif

// How big the array to store backtraces will be.
#ifndef CRASH_UNWIND_DEPTH
#    define CRASH_UNWIND_DEPTH (100)
#endif

/**
 * Information about a crash.
 */
typedef struct {
    /**
     * How nested the call stack was when program crashed.
     */
    size_t stack_depth;
    /**
     * Buffer storing stack frames (only the first ``stack_depth`` ones are valid).
     */
    backtrace_t call_stack[CRASH_UNWIND_DEPTH];
    /**
     * Reason of the crash, null-terminated.
     */
    char msg[CRASH_MESSAGE_LENGTH];
} crash_info_t;

/**
 * Get information about last execution.
 *
 * Return: Whether previous execution crashed
 *    * ``true``: Call stack that lead to crashing has been copied to ``info``.
 *    * ``false``: ``info`` left untouched.
 */
bool get_crash(crash_info_t *info);
