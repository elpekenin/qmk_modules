// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Custom logging utilities, inspired by Python's |logging|_ module.
 *
 * .. note::
 *   Under the hood, this is just a wrapper on top of :c:func:`printf`.
 *
 * .. hack for link with inline-code styling
 * .. |logging| replace:: ``logging``
 * .. _logging: https://docs.python.org/3/library/logging.html
 */

// -- barrier --

#pragma once

#include <quantum/compiler_support.h>
#include <stdbool.h>

#include "printf/printf.h" // ATTR_PRINTF

#ifndef CONFIG_LOGGING_FORMAT
/**
 * Default format for logging messages.
 */
#    define CONFIG_LOGGING_FORMAT "[%LS] %M\n"
#endif

/**
 * Different level of severity. Used to filter out messages.
 *
 * .. warning::
 *   If you want to add a new one, it has to be the last element.
 */
typedef enum {
    /** */
    LOG_DEBUG,
    /** */
    LOG_INFO,
    /** */
    LOG_WARN,
    /** */
    LOG_ERROR,
    /** */
    LOG_NONE,
} log_level_t; // ALWAYS ADD AT THE END, FOR ASSERT TO WORK

/**
 * .. hint::
 *   The :c:func:`logging` function will apply an extra transformation to your input, based on a custom format.
 *   Its specifiers being:
 *
 *   * ``%LL``: The message's level (long). Eg: ``DEBUG``.
 *
 *     * These strings are set in ``level_str``.
 *   * ``%LS``: Print only the first char of the previous string. Eg: ``D``.
 *   * ``%M``: The actual message created by ``msg`` and ``...`` passed to :c:func:`logging`. With its regular format.
 *   * ``%T``: Current time, you can override :c:func:`log_time` to hook it with a RTC or whatever.
 *
 *     * Default implementation is seconds since boot.
 *   * ``%%``: Write a literal ``%``.
 *
 *   For example, with format of ``[%LL] %T -- %M``, messages would look like: ``[DEBUG] 3s -- Formatted message``
 */

/**
 * Emit a logging message.
 *
 * Args:
 *     level: Severity of the message.
 *     msg: Format string for the message.
 *     ...: Variadic arguments to fill the specifiers in ``msg``.
 *
 * Return: Error code.
 *    * ``0``: Message handled correctly (maybe ignored due to settings).
 *    * ``-EBUSY``: Could not acquire the mutex guarding this function.
 *    * ``-EINVAL``: Logging format is wrong. Following calls will behave as usual ``printf``, returning 0.
 */
ATTR_PRINTF(2, 3) int logging(log_level_t level, const char *msg, ...);

typedef enum {
    STR_END,
    NO_SPEC,
    INVALID_SPEC,
    LL_SPEC,
    LS_SPEC,
    M_SPEC,
    PERC_SPEC,
    T_SPEC,
} token_t;

/**
 * Get the current level.
 * Messages with a lower severity are dropped.
 */
log_level_t get_logging_level(void);

/**
 * Change the current level.
 */
void set_logging_level(log_level_t level);

/**
 * Increase (or decrease) by one the level.
 *
 * The direction is based on ``increase``.
 */
void step_logging_level(bool increase);

/**
 * .. attention::
 *   From this point, the functions are mostly implementation details.
 *
 *   You, most likely, don't need to know anything about them.
 */

/**
 * Get the severity of the message being emitted.
 *
 * This may be used by a :c:type:`sendchar_func_t` internally.
 */
log_level_t get_current_message_level(void);

/**
 * Get a string representing the current time.
 *
 * By default, seconds since boot, but it can be overwritten.
 */
const char *log_time(void);

/**
 * Check that an array has as many elements as logging levels are defined.
 *
 * If not, compilation will error out.
 */
#define ASSERT_LEVELS(__array) STATIC_ASSERT(ARRAY_SIZE(__array) == LOG_NONE + 1, "Wrong size")
