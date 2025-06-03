// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Generic types/functions.
 */

// -- barrier --

#pragma once

#include <quantum/util.h>
#include <stdbool.h>
#include <sys/cdefs.h>

#include "printf/printf.h"

_Noreturn static inline void raise_error(const char *msg) {
    printf("[ERROR] %s\n", msg);
    while (true) {
    }
}

//
// Option
//

#define _Option(T) option___##T

#define option_unwrap(T) option___##T##_unwrap

/**
 * Define a new option type.
 *
 * Args:
 *     T: The type of values.
 */
#define OptionImpl(T)                                      \
    typedef struct Option(T) Option(T);                    \
                                                           \
    struct Option(T) {                                     \
        const bool is_some;                                \
        const T    __value;                                \
        const T (*__unwrap)(Option(T)) __result_use_check; \
    };                                                     \
                                                           \
    static inline T option_unwrap(T)(Option(T) option) {   \
        if (option.is_some) {                              \
            return option.__value;                         \
        }                                                  \
        raise_error("Called `unwrap` on `None`");          \
    }

/**
 * Expresive type for operations that may return nothing.
 *
 * :c:member:`is_some`: Whether this option contains a value.
 */
#define Option(T) _Option(T)

/**
 * Create an option with the given value.
 */
#define Some(T, v)                                                     \
    (Option(T)) {                                                      \
        .is_some = true, .__value = (v), .__unwrap = option_unwrap(T), \
    }

/**
 * Create an empty option.
 */
#define None(T)                                         \
    (Option(T)) {                                       \
        .is_some = false, .__unwrap = option_unwrap(T), \
    }

//
// Result
//

#define _Result(T, E) result___##T##___##E

#define result_unwrap(T, E) result___##T##___##E##_unwrap
#define result_unwrap_err(T, E) result___##T##___##E##_unwrap_err

/**
 * Define a new result type.
 *
 * Args:
 *     T: The type of values.
 *     E: The type of errors.
 */
#define ResultImpl(T, E)                                           \
    typedef struct Result(T, E) Result(T, E);                      \
                                                                   \
    struct Result(T, E) {                                          \
        const bool is_ok;                                          \
        union {                                                    \
            const T __value;                                       \
            const E __error;                                       \
        };                                                         \
        const T (*__unwrap)(Result(T, E)) __result_use_check;      \
        const E (*__unwrap_err)(Result(T, E)) __result_use_check;  \
    };                                                             \
                                                                   \
    static inline T result_unwrap(T, E)(Result(T, E) result) {     \
        if (result.is_ok) {                                        \
            return result.__value;                                 \
        }                                                          \
        raise_error("Called `unwrap` on `Err`");                   \
    }                                                              \
                                                                   \
    static inline E result_unwrap_err(T, E)(Result(T, E) result) { \
        if (!result.is_ok) {                                       \
            return result.__error;                                 \
        }                                                          \
        raise_error("Called `unwrap_err` on `Ok`");                \
    }

/**
 * Expresive type for operations that may fail.
 *
 * :c:member:`is_ok`: Whether this result is a value or error.
 */
#define Result(T, E) _Result(T, E)

/**
 * Create a result with the given value.
 */
#define Ok(T, E, v)                                                                                              \
    (Result(T, E)) {                                                                                             \
        .is_ok = true, .__value = (v), .__unwrap = result_unwrap(T, E), .__unwrap_err = result_unwrap_err(T, E), \
    }

/**
 * Create a result instance with the given error.
 */
#define Err(T, E, e)                                                                                              \
    (Result(T, E)) {                                                                                              \
        .is_ok = false, .__error = (e), .__unwrap = result_unwrap(T, E), .__unwrap_err = result_unwrap_err(T, E), \
    }

/**
 * Get the value out of an `Ok`/`Some` value, halt if `Err`/`None`
 */
#define unwrap(option_or_result) ((option_or_result).__unwrap(option_or_result))

/**
 * Get the error out of an `Err` value, halt if `Ok`
 */
#define unwrap_err(result) ((result).__unwrap_err(result))

/**
 * ----
 */

//
// RingBuffer
//

#define _RingBuffer(T) rbuf___##T

#define _rbuf_push(T) rbuf___##T##_push
#define _rbuf_pop(T) rbuf___##T##_pop
#define _rbuf_has_data(T) rbuf___##T##_has_data
#define _rbuf_clear(T) rbuf___##T##_clear

/**
 * Define a new ring buffer type.
 *
 * Args:
 *     T: The type of values.
 */
#define RingBufferImpl(T)                                             \
    typedef struct RingBuffer(T) RingBuffer(T);                       \
                                                                      \
    struct RingBuffer(T) {                                            \
        T *const     __data;                                          \
        const size_t __size;                                          \
        size_t       __head;                                          \
        size_t       __tail;                                          \
        const bool (*__push)(RingBuffer(T) *, T) __result_use_check;  \
        const Option(T) (*__pop)(RingBuffer(T) *)__result_use_check;  \
        const bool (*__has_data)(RingBuffer(T));                      \
        const void (*__clear)(RingBuffer(T) *);                       \
    };                                                                \
                                                                      \
    static inline bool _rbuf_push(T)(RingBuffer(T) * rbuf, T value) { \
        const size_t next = (rbuf->__head + 1) % rbuf->__size;        \
        if (next == rbuf->__tail) {                                   \
            return false;                                             \
        }                                                             \
                                                                      \
        rbuf->__data[rbuf->__head] = value;                           \
                                                                      \
        rbuf->__head = next;                                          \
                                                                      \
        return true;                                                  \
    }                                                                 \
                                                                      \
    static inline Option(T) _rbuf_pop(T)(RingBuffer(T) * rbuf) {      \
        if (rbuf->__head == rbuf->__tail) {                           \
            return None(T);                                           \
        }                                                             \
                                                                      \
        T val = rbuf->__data[rbuf->__tail];                           \
                                                                      \
        rbuf->__tail = (rbuf->__tail + 1) % rbuf->__size;             \
                                                                      \
        return Some(T, val);                                          \
    }                                                                 \
                                                                      \
    static inline bool _rbuf_has_data(T)(RingBuffer(T) rbuf) {        \
        return rbuf.__head != rbuf.__tail;                            \
    }                                                                 \
                                                                      \
    static inline void _rbuf_clear(T)(RingBuffer(T) * rbuf) {         \
        rbuf->__head = rbuf->__tail = 0;                              \
    }

/**
 * Ring buffer type.
 */
#define RingBuffer(T) _RingBuffer(T)

// TODO: infer `T` from `buf` somehow ?
/**
 * Create a ring buffer around the given buffer.
 *
 * Args:
 *     T: The type of values.
 *     buf: The buffer to wrap.
 */
#define rbuf_from(T, buf)                                                                                                                                                                 \
    (RingBuffer(T)) {                                                                                                                                                                     \
        .__data = (buf), .__size = ARRAY_SIZE(buf), .__head = 0, .__tail = 0, .__push = _rbuf_push(T), .__pop = _rbuf_pop(T), .__has_data = _rbuf_has_data(T), .__clear = _rbuf_clear(T), \
    }

/**
 * Add an element.
 */
#define rbuf_push(rbuf, value) ((rbuf).__push(&rbuf, value))

/**
 * Remove an element.
 */
#define rbuf_pop(rbuf) ((rbuf).__pop(&rbuf))

/**
 * Check emptiness.
 */
#define rbuf_has_data(rbuf) ((rbuf).__has_data(buf))

/**
 * Reset a buffer
 */
#define rbuf_clear(rbuf) ((rbuf).__clear(&rbuf))

//
// Find
//

#define _find(T, ptr, n, func)                         \
    ({                                                 \
        T *out = NULL;                                 \
        /* this assignment validates func signature */ \
        bool (*f)(T) = func;                           \
        for (size_t i = 0; i < n; ++i) {               \
            if (f(ptr[i])) {                           \
                out = &ptr[i];                         \
            }                                          \
        }                                              \
                                                       \
        out;                                           \
    })

#define find(ptr, n, func) _find(typeof(*ptr), ptr, n, func)
#define find_array(array, func) find(array, ARRAY_SIZE(array), func)
