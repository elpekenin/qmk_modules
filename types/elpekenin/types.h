// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Generic types.
 *    - Result: Expresive type for operations that can fail.
 *    - Option: Expresive type for operations that can return nothing.
 *    - RingBuffer: Name says it all.
 */

// -- barrier --

#pragma once

#include <stdbool.h>

#include "printf/printf.h"

_Noreturn static inline void raise_error(const char *msg) {
    printf("[ERROR] %s\n", msg);
    while (true) {
    }
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
#define ResultImpl(T, E)                                                         \
    typedef struct Result(T, E) Result(T, E);                                    \
                                                                                 \
    struct Result(T, E) {                                                        \
        const bool is_ok;                                                        \
        union {                                                                  \
            const T __value;                                                     \
            const E __error;                                                     \
        };                                                                       \
        const T (*unwrap)(Result(T, E)) __attribute__((warn_unused_result));     \
        const E (*unwrap_err)(Result(T, E)) __attribute__((warn_unused_result)); \
    };                                                                           \
                                                                                 \
    static inline T result_unwrap(T, E)(Result(T, E) result) {                   \
        if (result.is_ok) {                                                      \
            return result.__value;                                               \
        }                                                                        \
        raise_error("Called `unwrap` on `Err`");                                 \
    }                                                                            \
                                                                                 \
    static inline E result_unwrap_err(T, E)(Result(T, E) result) {               \
        if (!result.is_ok) {                                                     \
            return result.__error;                                               \
        }                                                                        \
        raise_error("Called `unwrap_err` on `Ok`");                              \
    }

/**
 * Result type.
 *
 * :c:member:`is_ok`: Whether this result is a value or error.
 */
#define Result(T, E) _Result(T, E)

/**
 * Create a result with the given value.
 */
#define Ok(T, E, v)                                                                                          \
    (Result(T, E)) {                                                                                         \
        .is_ok = true, .__value = (v), .unwrap = result_unwrap(T, E), .unwrap_err = result_unwrap_err(T, E), \
    }

/**
 * Create a result instance with the given error.
 */
#define Err(T, E, e)                                                                                          \
    (Result(T, E)) {                                                                                          \
        .is_ok = false, .__error = (e), .unwrap = result_unwrap(T, E), .unwrap_err = result_unwrap_err(T, E), \
    }

/**
 * ----
 */

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
#define OptionImpl(T)                                                     \
    typedef struct Option(T) Option(T);                                   \
                                                                          \
    struct Option(T) {                                                    \
        const bool is_some;                                               \
        const T    __value;                                               \
        const T (*unwrap)(Option(T)) __attribute__((warn_unused_result)); \
    };                                                                    \
                                                                          \
    static inline T option_unwrap(T)(Option(T) option) {                  \
        if (option.is_some) {                                             \
            return option.__value;                                        \
        }                                                                 \
        raise_error("Called `unwrap` on `None`");                         \
    }

/**
 * Option type.
 *
 * :c:member:`is_some`: Whether this option contains a value.
 */
#define Option(T) _Option(T)

/**
 * Create an option with the given value.
 */
#define Some(T, v)                                                   \
    (Option(T)) {                                                    \
        .is_some = true, .__value = (v), .unwrap = option_unwrap(T), \
    }

/**
 * Create an empty option.
 */
#define None(T)                                       \
    (Option(T)) {                                     \
        .is_some = false, .unwrap = option_unwrap(T), \
    }

/**
 * ----
 */

//
// RingBuffer
//

#define _RingBuffer(T) rbuf___##T

#define rbuf_push(T) rbuf___##T##_push
#define rbuf_pop(T) rbuf___##T##_pop
#define rbuf_has_data(T) rbuf___##T##_has_data
#define rbuf_clear(T) rbuf___##T##_clear

/**
 * Define a new ring buffer type.
 *
 * Args:
 *     T: The type of values.
 */
#define RingBufferImpl(T)                                                           \
    typedef struct RingBuffer(T) RingBuffer(T);                                     \
                                                                                    \
    struct RingBuffer(T) {                                                          \
        T *const     __data;                                                        \
        const size_t __size;                                                        \
        size_t       __head;                                                        \
        size_t       __tail;                                                        \
        const bool (*push)(RingBuffer(T) *, T) __attribute__((warn_unused_result)); \
        const Option(T) (*pop)(RingBuffer(T) *)__attribute__((warn_unused_result)); \
        const bool (*has_data)(RingBuffer(T));                                      \
        const void (*clear)(RingBuffer(T) *);                                       \
    };                                                                              \
                                                                                    \
    static inline bool rbuf_push(T)(RingBuffer(T) * rbuf, T value) {                \
        const size_t next = (rbuf->__head + 1) % rbuf->__size;                      \
        if (next == rbuf->__tail) {                                                 \
            return false;                                                           \
        }                                                                           \
                                                                                    \
        rbuf->__data[rbuf->__head] = value;                                         \
                                                                                    \
        rbuf->__head = next;                                                        \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    static inline Option(T) rbuf_pop(T)(RingBuffer(T) * rbuf) {                     \
        if (rbuf->__head == rbuf->__tail) {                                         \
            return None(T);                                                         \
        }                                                                           \
                                                                                    \
        T val = rbuf->__data[rbuf->__tail];                                         \
                                                                                    \
        rbuf->__tail = (rbuf->__tail + 1) % rbuf->__size;                           \
                                                                                    \
        return Some(T, val);                                                        \
    }                                                                               \
                                                                                    \
    static inline bool rbuf_has_data(T)(RingBuffer(T) rbuf) {                       \
        return rbuf.__head != rbuf.__tail;                                          \
    }                                                                               \
                                                                                    \
    static inline void rbuf_clear(T)(RingBuffer(T) * rbuf) {                        \
        rbuf->__head = rbuf->__tail = 0;                                            \
    }

/**
 * Ring buffer type.
 *
 * It has "methods":
 *
 * .. code-block:: c
 *
 *     bool      push(RingBuffer(T) *rbuf, T value); // add an element
 *     Option(T) pop(RingBuffer(T) *rbuf);           // remove an element
 *     bool      has_data(RingBuffer(T) rbuf);       // check emptyness
 *     void      clear(RingBuffer(T) *rbuf);         // reset the buffer
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
#define rbuf_from(T, buf)             \
    {                                 \
        .__data   = (buf),            \
        .__size   = ARRAY_SIZE(buf),  \
        .__head   = 0,                \
        .__tail   = 0,                \
        .push     = rbuf_push(T),     \
        .pop      = rbuf_pop(T),      \
        .has_data = rbuf_has_data(T), \
        .clear    = rbuf_clear(T),    \
    }
