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

#include <quantum/compiler_support.h>
#include <quantum/util.h>
#include <stdbool.h>
#include <sys/cdefs.h>

#include "printf/printf.h"

//
// Result
//

#define _Result(T, E) result___##T##___##E

/**
 * Define a new result type.
 *
 * Args:
 *     T: The type of values.
 *     E: The type of errors.
 */
#define ResultImpl(T, E)    \
    typedef struct PACKED { \
        bool is_ok;         \
        union {             \
            T __value;      \
            E __error;      \
        };                  \
    } Result(T, E);         \
    STATIC_ASSERT(offsetof(Result(T, E), is_ok) == 0, "Invalid data layout")

/**
 * Result type.
 *
 * :c:member:`is_ok`: Whether this result is a value or error.
 */
#define Result(T, E) _Result(T, E)

/**
 * Create a result with the given value.
 */
#define Ok(T, E, v)                    \
    (Result(T, E)) {                   \
        .is_ok = true, .__value = (v), \
    }

/**
 * Create a result instance with the given error.
 */
#define Err(T, E, e)                    \
    (Result(T, E)) {                    \
        .is_ok = false, .__error = (e), \
    }

/**
 * ----
 */

//
// Option
//

#define _Option(T) option___##T

/**
 * Define a new option type.
 *
 * Args:
 *     T: The type of values.
 */
#define OptionImpl(T)       \
    typedef struct PACKED { \
        bool is_some;       \
        T    __value;       \
    } Option(T);            \
    STATIC_ASSERT(offsetof(Option(T), is_some) == 0, "Invalid data layout")

/**
 * Option type.
 *
 * :c:member:`is_some`: Whether this option contains a value.
 */
#define Option(T) _Option(T)

/**
 * Create an option with the given value.
 */
#define Some(T, v)                       \
    (Option(T)) {                        \
        .is_some = true, .__value = (v), \
    }

/**
 * Create an empty option.
 */
#define None(T)           \
    (Option(T)) {         \
        .is_some = false, \
    }

/**
 * ----
 */

// -- barrier --

_Noreturn static inline void raise_error(const char *msg) {
    printf("[ERROR] %s\n", msg);
    while (true) {
    }
}

/**
 * Get the inner value from an `Ok`/`Some` value. Panic if `Err`/`None`.
 */
#define unwrap(v)                                              \
    ({                                                         \
        /* is_ok / is_some */                                  \
        /* relies on them being first element of struct */     \
        bool *ptr = (bool *)&(v);                              \
        if (!(*ptr)) {                                         \
            raise_error("called `unwrap` on `Err` or `None`"); \
        }                                                      \
                                                               \
        (v).__value;                                           \
    })

/**
 * Get the inner value from an `Err`. Panic if `Ok`.
 */
#define unwrap_err(v)                                   \
    ({                                                  \
        if ((v).is_ok) {                                \
            raise_error("called `unwrap_err` on `Ok`"); \
        }                                               \
                                                        \
        (v).__error;                                    \
    })

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
#define RingBufferImpl(T)                                                     \
    typedef struct RingBuffer(T) RingBuffer(T);                               \
                                                                              \
    struct RingBuffer(T) {                                                    \
        T           *data;                                                    \
        const size_t size;                                                    \
        size_t       head;                                                    \
        size_t       tail;                                                    \
        bool (*push)(RingBuffer(T) *, T) __attribute__((warn_unused_result)); \
        Option(T) (*pop)(RingBuffer(T) *)__attribute__((warn_unused_result)); \
        bool (*has_data)(RingBuffer(T));                                      \
        void (*clear)(RingBuffer(T) *);                                       \
    };                                                                        \
                                                                              \
    static inline bool rbuf_push(T)(RingBuffer(T) * rbuf, T value) {          \
        const size_t next = (rbuf->head + 1) % rbuf->size;                    \
        if (next == rbuf->tail) {                                             \
            return false;                                                     \
        }                                                                     \
                                                                              \
        rbuf->data[rbuf->head] = value;                                       \
                                                                              \
        rbuf->head = next;                                                    \
                                                                              \
        return true;                                                          \
    }                                                                         \
                                                                              \
    static inline Option(T) rbuf_pop(T)(RingBuffer(T) * rbuf) {               \
        if (rbuf->head == rbuf->tail) {                                       \
            return None(T);                                                   \
        }                                                                     \
                                                                              \
        T val = rbuf->data[rbuf->tail];                                       \
                                                                              \
        rbuf->tail = (rbuf->tail + 1) % rbuf->size;                           \
                                                                              \
        return Some(T, val);                                                  \
    }                                                                         \
                                                                              \
    static inline bool rbuf_has_data(T)(RingBuffer(T) rbuf) {                 \
        return rbuf.head != rbuf.tail;                                        \
    }                                                                         \
                                                                              \
    static inline void rbuf_clear(T)(RingBuffer(T) * rbuf) {                  \
        rbuf->head = rbuf->tail = 0;                                          \
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
        .data     = (buf),            \
        .size     = ARRAY_SIZE(buf),  \
        .head     = 0,                \
        .tail     = 0,                \
        .push     = rbuf_push(T),     \
        .pop      = rbuf_pop(T),      \
        .has_data = rbuf_has_data(T), \
        .clear    = rbuf_clear(T),    \
    }
