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
 * How big the array to store different allocators will be.
 */
#ifndef ALLOCATORS_POOL_SIZE
#    define ALLOCATORS_POOL_SIZE 10
#endif

/**
 * How big the array to store allocations' metadata will be.
 */
#ifndef ALLOC_STATS_POOL_SIZE
#    define ALLOC_STATS_POOL_SIZE 100
#endif

typedef struct allocator_t allocator_t;

/**
 * Information about an allocation.
 */
typedef struct PACKED {
    /**
     * Allocator used to request this memory.
     */
    allocator_t *allocator;

    /**
     * Pointer to the memory region provided by allocator.
     */
    void *ptr;

    /**
     * Size in bytes of the memory region.
     */
    size_t size;

    /**
     * Information about this alloation's duration.
     */
    struct {
        /**
         * When was the memory allocated.
         */
        uint32_t start;

        /**
         * When was the memory freed.
         */
        uint32_t end;
    } lifetime;
} alloc_stats_t;

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
     * A short name/description.
     */
    const char *name;

    /**F
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
 *   For now, that's stdlib's implementation.
 */
const allocator_t *get_default_allocator(void);

#if defined(PROTOCOL_CHIBIOS) || defined(__SPHINX__)
#    include <ch.h>
#    include <chmemcore.h>

_Static_assert(CH_CFG_USE_MEMCORE == TRUE, "Enable ChibiOS core allocator");

/**
 * ChibiOS' core allocator.
 */
extern allocator_t ch_core_allocator;

#    if CH_CFG_USE_MEMPOOLS == TRUE || defined(__SPHINX__)
#        include <chmempools.h>
/**
 * Create a new ChibiOS' pool allocator.
 */
allocator_t new_ch_pool_allocator(memory_pool_t *pool, const char *name);
#    endif

#    if CH_CFG_USE_HEAP == TRUE || defined(__SPHINX__)
#        include <chmemheaps.h>
/**
 * Create a new ChibiOS' heap allocator.
 */
allocator_t new_ch_heap_allocator(memory_heap_t *heap, const char *name);
#    endif
#endif
