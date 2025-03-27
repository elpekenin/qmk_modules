// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Utilities to use custom allocators instead of stdlib's implementation.
 *
 * This is: :c:func:`malloc`, :c:func:`free`, :c:func:`calloc` and :c:func:`realloc`.
 */

// -- barrier --

#pragma once

#include <quantum/util.h>
#include <stddef.h>

/**
 * How big a pre-allocated buffer to store backtraces will be.
 */
#ifndef ALLOC_BUFF_SIZE
#    define ALLOC_BUFF_SIZE 100
#endif

typedef struct allocator_t allocator_t;

/**
 * Signature of a :c:func:`malloc` function.
 */
typedef void *(*malloc_fn)(allocator_t *allocator, size_t size);

/**
 * Signature of a :c:func:`free` function.
 */
typedef void (*free_fn)(allocator_t *allocator, void *ptr);

/**
 * Signature of a :c:func:`calloc` function.
 */
typedef void *(*calloc_fn)(allocator_t *allocator, size_t nmemb, size_t size);

/**
 * Signature of a :c:func:`realloc` function.
 */
typedef void *(*realloc_fn)(allocator_t *allocator, void *ptr, size_t size);

/**
 * Information about a custom allocator.
 */
struct PACKED allocator_t {
    /**
     * Pointer to its ``malloc`` implementation.
     */
    malloc_fn malloc;

    /**
     * Pointer to its ``free`` implementation.
     */
    free_fn free;

    /**
     * Pointer to its ``calloc`` implementation.
     */
    calloc_fn calloc;

    /**
     * Pointer to its ``realloc`` implementation.
     */
    realloc_fn realloc;

    /**
     * Memory it has current allocated.
     */
    size_t used;

    /**
     * A short name/description.
     */
    const char *name;

    /**
     * Arbitrary config used by allocator. Eg a ChibiOS' pool.
     */
    void *arg;
};

/**
 * .. caution::
 *   These wrappers add some extra logic as well as calling ``allocator->function(args)``.
 *
 *   Use them instead of manually executing the functions.
 */

/**
 * Run ``malloc``'s implementation of the given allocator.
 */
void *malloc_with(allocator_t *allocator, size_t total_size);

/**
 * Run ``free``'s implementation of the given allocator.
 */
void free_with(allocator_t *allocator, void *ptr);

/**
 * Run ``calloc``'s implementation of the given allocator.
 */
void *calloc_with(allocator_t *allocator, size_t nmemb, size_t size);

/**
 * Run ``realloc``'s implementation of the given allocator.
 */
void *realloc_with(allocator_t *allocator, void *ptr, size_t size);

/**
 * Total heap used between all allocators.
 */
size_t get_used_heap(void);

/**
 * Get a pointer to every allocator implementation.
 *
 * :c:var:`n` will be set to the number of allocators.
 */
const allocator_t **get_known_allocators(int8_t *n);

/**
 * Get the allocator defined as "default".
 *
 * .. hint:
 *   For now, that's :c:var:`c_runtime_allocator`
 */
allocator_t *get_default_allocator(void);

/**
 * C's stdlib allocator.
 */
extern allocator_t c_runtime_allocator;

#if defined(PROTOCOL_CHIBIOS)
#    include <ch.h>
_Static_assert(CH_CFG_USE_MEMCORE == TRUE, "Enable ChibiOS core allocator");

#    include <chmemcore.h>

/**
 * ChibiOS' core allocator.
 */
extern allocator_t ch_core_allocator;

#    if CH_CFG_USE_MEMPOOLS == TRUE
#        include <chmempools.h>
/**
 * Create a new ChibiOS' pool allocator.
 */
allocator_t new_ch_pool_allocator(memory_pool_t *pool, const char *name);
#    endif

#    if CH_CFG_USE_HEAP == TRUE
#        include <chmemheaps.h>
/**
 * Create a new ChibiOS' heap allocator.
 */
allocator_t new_ch_heap_allocator(memory_heap_t *heap, const char *name);
#    endif
#endif
