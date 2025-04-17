// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Error types in C. Expresively return values on operations that can fail.
 */

// -- barrier --

#pragma once

// deferred macro, for argument expansion
#define _Result(T, E) result___##T##___##E
#define Result(T, E) _Result(T, E)

/**
 * Define a new result type.
 *
 * Args:
 *     T: The type of 'ok' values.
 *     E: The type of 'err' values.
 */
#define ResultImpl(T, E) \
    typedef struct {     \
        bool __is_ok;    \
        union {          \
            T __val;     \
            E __err;     \
        } __storage;     \
    } Result(T, E)

/**
 * Create an 'ok' instance with the given value.
 */
#define Ok(v)               \
    {                       \
        .__is_ok = true,    \
        .__storage =        \
            {               \
                .__val = v, \
            },              \
    }

/**
 * Create an 'err' instance with the given error.
 */
#define Err(e)              \
    {                       \
        .__is_ok = false,   \
        .__storage =        \
            {               \
                .__err = e, \
            },              \
    }

/**
 * Check if result contains an 'ok' value.
 */
#define is_ok(r) (r.__is_ok)

/**
 * Check if result contains an 'err' value.
 */
#define is_err(r) (!is_ok(r))

// get value/error out of the result
#define _val(r) (r.__storage.__val)
#define _err(r) (r.__storage.__err)

/**
 * Get the 'ok' value out of a result, or the provided value if 'err'.
 */
#define unwrap_or(r, val) (is_ok(r) ? _val(r) : val)

/**
 * Get the 'ok' value out of a result, or panic with the given message if 'err'.
 */
#define expect(r, msg)      \
    ({                      \
        if (is_err(r)) {    \
            chSysHalt(msg); \
        }                   \
                            \
        _val(r);            \
    })

/**
 * Same as :c:func:`expect`, with a default message.
 */
#define unwrap(r) expect(r, "called `unwrap` on an 'err' value")

/**
 * Get the 'err' value out of a result, or panic with the given message if 'ok'.
 */
#define expect_err(r, msg)  \
    ({                      \
        if (is_ok(r)) {     \
            chSysHalt(msg); \
        }                   \
                            \
        _err(r);            \
    })

/**
 * Same as :c:func:`expect_err`, with a default message.
 */
#define unwrap_err(r) expect_err(r, "called `unwrap_err` on an 'ok' value")
