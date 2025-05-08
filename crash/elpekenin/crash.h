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
#include <stdint.h>

#if !defined(COMMUNITY_MODULE_TYPES_ENABLE)
#    error 'elpekenin/types' must be enabled
#endif

#include "elpekenin/types.h"

/**
 * How big the array to store backtraces will be.
 */
#ifndef CRASH_UNWIND_DEPTH
#    define CRASH_UNWIND_DEPTH 100
#endif

/**
 * How big the array to store a message will be.
 */
#ifndef CRASH_MESSAGE_LEN
#    define CRASH_MESSAGE_LEN 200
#endif

typedef struct {
    uint8_t     stack_depth;
    backtrace_t call_stack[CRASH_UNWIND_DEPTH];
    char        msg[CRASH_MESSAGE_LEN];
} crash_info_t;

OptionImpl(crash_info_t);

/**
 * Get information about last execution.
 *
 * Return:
 *     Optional stack trace.
 *        * Some(trace): Call stack that crashed the program. Use :c:func:`unwrap` to get the value.
 *        * None: Previous execution did not crash.
 */
Option(crash_info_t) get_crash(void);

/**
 * Crash the program because of given reason.
 */
_Noreturn void exception(const char *reason);
