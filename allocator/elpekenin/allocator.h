// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Utilities to use custom allocators instead of stdlib's implementation.
 *
 * This is: :c:func:`malloc`, :c:func:`free`, :c:func:`calloc` and :c:func:`realloc`.
 */

// -- barrier --

#pragma once

#include <quantum/compiler_support.h>
#include <quantum/util.h>
#include <stddef.h>

/**
 * How big the array to store different allocators will be.
 */
#ifndef ALLOC_ALLOCATORS_SIZE
#    define ALLOC_ALLOCATORS_SIZE 10
#endif

/**
 * How big the array to store allocations' metadata will be.
 */
#ifndef ALLOC_ALLOCATIONS_SIZE
#    define ALLOC_ALLOCATIONS_SIZE 100
#endif

typedef struct allocator_t allocator_t;

/**
 * Information about an object's lifetime.
 */
typedef struct PACKED {
    /**
     * When was the memory allocated.
     */
    uint32_t start;

    /**
     * When was the memory freed.
     */
    uint32_t end;
} lifetime_t;

/**
 * Information about an allocation.
 */
typedef struct PACKED {
    /**
     * Allocator used to request this memory.
     */
    const allocator_t *allocator;

    /**
     * Pointer to the memory region provided by allocator.
     */
    void *ptr;

    /**
     * Size in bytes of the memory region.
     */
    size_t size;

    /**
     * Allocation's duration.
     */
    lifetime_t lifetime;
} alloc_stats_t;

/**
 * Signature of a :c:func:`malloc` function.
 */
typedef void *(*malloc_fn)(const allocator_t *allocator, size_t size);

/**
 * Signature of a :c:func:`free` function.
 */
typedef void (*free_fn)(const allocator_t *allocator, void *ptr);

/**
 * Signature of a :c:func:`calloc` function.
 */
typedef void *(*calloc_fn)(const allocator_t *allocator, size_t nmemb, size_t size);

/**
 * Signature of a :c:func:`realloc` function.
 */
typedef void *(*realloc_fn)(const allocator_t *allocator, void *ptr, size_t size);

/**
 * Information about a custom allocator.
 */
struct PACKED allocator_t {
    /**
     * Pointer to its ``malloc`` implementation.
     */
    const malloc_fn malloc;

    /**
     * Pointer to its ``free`` implementation.
     */
    const free_fn free;

    /**
     * Pointer to its ``calloc`` implementation.
     */
    const calloc_fn calloc;

    /**
     * Pointer to its ``realloc`` implementation.
     */
    const realloc_fn realloc;

    /**
     * A short name/description.
     */
    const char *const name;

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
void *malloc_with(const allocator_t *allocator, size_t total_size);

/**
 * Run ``free``'s implementation of the given allocator.
 */
void free_with(const allocator_t *allocator, void *ptr);

/**
 * Run ``calloc``'s implementation of the given allocator.
 */
void *calloc_with(const allocator_t *allocator, size_t nmemb, size_t size);

/**
 * Run ``realloc``'s implementation of the given allocator.
 */
void *realloc_with(const allocator_t *allocator, void *ptr, size_t size);

/**
 * Total heap used between all allocators.
 */
size_t get_used_heap(void);

/**
 * Get a pointer to every allocator implementation.
 *
 * :c:var:`n` will be set to the number of allocators.
 */
const allocator_t *const *get_known_allocators(size_t *n);

/**
 * Get a pointer to every tracked allocation implementation.
 *
 * :c:var:`n` will be set to the number of allocation.
 */
const alloc_stats_t *get_allocations(size_t *n);

extern const allocator_t *const c_runtime_allocator;

#if defined(PROTOCOL_CHIBIOS) || defined(__SPHINX__)
#    include <ch.h>
#    include <chmemcore.h>

STATIC_ASSERT(CH_CFG_USE_MEMCORE == TRUE, "Enable ChibiOS core allocator");

/**
 * ChibiOS' core allocator.
 */
extern const allocator_t *const ch_core_allocator;

#    if CH_CFG_USE_MEMPOOLS == TRUE || defined(__SPHINX__)
#        include <chmempools.h>
/**
 * Create a new ChibiOS' pool allocator.
 */
const allocator_t new_ch_pool_allocator(memory_pool_t *pool, const char *name);
#    endif

#    if CH_CFG_USE_HEAP == TRUE || defined(__SPHINX__)
#        include <chmemheaps.h>
/**
 * Create a new ChibiOS' heap allocator.
 */
const allocator_t new_ch_heap_allocator(memory_heap_t *heap, const char *name);
#    endif
#endif

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui.h"

typedef struct {
    const uint8_t *font;
    size_t         last;
    uint32_t       interval;
} heap_args_t;
STATIC_ASSERT(offsetof(heap_args_t, font) == 0, "UI will crash :)");

bool     heap_init(ui_node_t *self);
uint32_t heap_render(const ui_node_t *self, painter_device_t display);
#endif
