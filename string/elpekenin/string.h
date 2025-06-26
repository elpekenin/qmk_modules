// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Small utilities to work with text.
 */

// -- barrier --

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "printf/printf.h" // ATTR_PRINTF

/**
 * "String builder" without heap (``malloc()``, ``realloc()``, ``free``) usage.
 *
 * Code using this API would look similar to:
 *
 * .. code-block:: c
 *
 *     // create a stack-based buffer, 50bytes long
 *     string_t str = str_new(50);
 *     // add text to it
 *     str_append(&str, "Hello");
 *     str_append(&str, " world");
 *     // display it
 *     printf("%.*s\n", str.used, str.ptr);
 */
typedef struct {
    /**
     * Total size in bytes.
     */
    const size_t size;

    /**
     * How many bytes have been used.
     */
    size_t used;

    /**
     * Text start.
     */
    char *ptr;
} string_t;

/**
 * Create a **temporary** ``string_t``.
 */
#define str_new(n)                              \
    (string_t) {                                \
        .size = n, .used = 0, .ptr = alloca(n), \
    }

/**
 * Create a ``string_t`` wrapper for the given buffer.
 */
#define str_from_buffer(buffer)                                 \
    (string_t) {                                                \
        .size = ARRAY_SIZE(buffer), .used = 0, .ptr = (buffer), \
    }

/**
 * Find out how many bytes in the buffer are left to be used.
 */
size_t str_available(string_t str);

/**
 * Reset the string's state.
 */
void str_reset(string_t *str);

/**
 * Add text to the string.
 */
int str_append(string_t *str, const char *text);

/**
 * Format text and add it to the string.
 */
ATTR_PRINTF(2, 3) int str_printf(string_t *str, const char *fmt, ...);

/**
 * Write the value of ``n`` (# of bytes) in an human-friendly format, into ``string``
 */
int pretty_bytes(string_t *str, size_t n);

/**
 * Check whether a char is UTF8.
 */
bool is_utf8(char chr);

/**
 * Check whether a char is a UTF8-continuation byte.
 */
bool is_utf8_continuation(char chr);
