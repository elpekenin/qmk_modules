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

/**
 * How big the array to store backtraces will be.
 */
#ifndef UNWIND_DEPTH
#    define UNWIND_DEPTH 100
#endif

/**
 * Get the information about last execution.
 *
 * Args:
 *     depth: Pointer to your variable which will be set to how deep the backtrace is (0 if no crash).
 *     msg: Pointer to your variable which will be set to the crash's cause (NULL if no crash).
 *
 * Return:
 *     Call stack that crashed the program (NULL if no crash)
 */
backtrace_t *get_crash_call_stack(uint8_t *depth, const char **msg);
