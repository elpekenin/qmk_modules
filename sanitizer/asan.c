// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Reference implementation:
 *   - https://github.com/llvm/llvm-project/tree/main/compiler-rt/lib/asan
 *
 * Notes:
 *  - Had to change between `uptr` and `void *` in several places
 */

/**
 * Small address sanitizer runtime.
 */

// -- barrier --

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "printf/printf.h"

/**
 * Memory address where RAM starts.
 */
#ifndef ASAN_MEMORY_START_ADDRESS
// default to ChibiOs symbol
extern uint8_t __ram0_base__;
#    define ASAN_MEMORY_START_ADDRESS ((void *)&__ram0_base__)
#endif

/**
 * How big the (tracked) RAM is.
 */
#ifndef ASAN_MEMORY_SIZE
// default to RP2040's RAM size
#    define ASAN_MEMORY_SIZE (264 * 1024)
#endif
_Static_assert(ASAN_MEMORY_SIZE % 8 == 0, "ASAN_MEMORY_SIZE must be a multiple of 8");

//
// types
//

typedef uintptr_t uptr;

typedef struct {
    const char *filename;
    int         line_no;
    int         column_no;
} __asan_global_source_location;

typedef struct {
    uptr                           beg;
    uptr                           n;
    uptr                           n_with_redzone;
    const char                    *name;
    const char                    *module_name;
    uptr                           has_dynamic_init;
    __asan_global_source_location *gcc_location;
    uptr                           odr_indicator;
} __asan_global;

//
// internal
//

static uint8_t shadow_mem[ASAN_MEMORY_SIZE / 8];

__nosanitizeaddress static void report_error(void *start, uptr access_size, bool is_write, bool fatal) {
    printf("[ERROR] asan: %s at %p\n", is_write ? "writing" : "loading", start);

    if (!fatal) return;

    while (true) {
    }
}

static uint8_t *addr_to_shadow(void *addr) {
    // out-of-bounds
    if (addr < ASAN_MEMORY_START_ADDRESS) return NULL;
    if (addr > (void *)((uptr)ASAN_MEMORY_START_ADDRESS + ASAN_MEMORY_SIZE)) return NULL;

    uptr offset = addr - ASAN_MEMORY_START_ADDRESS;
    return shadow_mem + (offset / 8);
}

__nosanitizeaddress static void access_check(void *start, uptr access_size, bool is_write, bool fatal) {
    uint8_t *shadow = addr_to_shadow(start);
    if (shadow == NULL) {
        return;
    }

    // fast path, no bits set
    if (*shadow == 0) return;

    // fast path, all bits set -> poison for sure
    if (*shadow == 0xFF) {
        report_error(start, access_size, is_write, fatal);
        return;
    }

    // TODO: speed this up
    // TODO: handle start being in shadow, but (start+access_size) outside
    uint8_t bit = 0;
    for (uptr i = 0; i < access_size; ++i) {
        // check if the bit is set
        if (*shadow & (1 << bit)) {
            // bit is set, report error
            report_error(start, access_size, is_write, fatal);
            return;
        }

        // move to next bit
        bit++;
        if (bit == 8) {
            shadow++;
            bit = 0;
        }
    }
}

__nosanitizeaddress static inline void update_bits(uint8_t *address, uint8_t mask, bool set) {
    if (set) {
        *address |= mask;
    } else {
        *address &= ~mask;
    }
}

__nosanitizeaddress static void update_region(uptr start, uptr size, bool poison) {
    uptr n = size; // bits left to be updated

    const uint8_t alignment  = 8;
    const uptr    align_mask = alignment - 1;

    // eg: start=0x203 -> aligned=0x200, offset=3
    const uptr    aligned = start & ~align_mask;
    const uint8_t offset  = start & align_mask;

    uint8_t *shadow = addr_to_shadow((void *)aligned);
    if (shadow == NULL) {
        return;
    }

    // handle first byte
    if (offset != 0) {
        // at most, we can update (8 - offset) bits
        // number of bits to update is the minimum between that and n
        const uint8_t bits = MIN(n, (8 - offset));
        const uint8_t mask = ((1 << bits) - 1) << offset;

        update_bits(shadow, mask, poison);
        n -= bits;
    }

    // TODO: in one go uing memset (?)
    // packs of 8 bytes updated in batches
    while (n >= 8) {
        *shadow = poison ? 0xFF : 0;
        n -= 8;
    }

    // update leftover bits
    if (n > 0) {
        const uint8_t mask = (1 << n) - 1;
        update_bits(shadow, mask, poison);
    }
}

__nosanitizeaddress void poison_global(__asan_global *global) {
    update_region(global->beg, global->n, false);
    update_region(global->beg + global->n, global->n_with_redzone, true);
}

__nosanitizeaddress void unpoison_global(__asan_global *global) {
    update_region(global->beg, global->n, true);
    update_region(global->beg + global->n, global->n_with_redzone, false);
}

static void *stack_malloc(__unused int id, uptr size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        return NULL;
    }

    update_region((uptr)ptr, size, false);

    return ptr;
}

static void stack_free(__unused int id, void *ptr, uptr size) {
    if (ptr == NULL) {
        return;
    }

    update_region((uptr)ptr, size, true);

    free(ptr);
}

//
// macros for repetitive functions
//
#define ASAN_REPORT_LOAD_STORE(size)                                     \
    __noinline void __asan_report_load##size(void *addr) {               \
        report_error(addr, size, /*is_write=*/false, /*is_fatal=*/true); \
        __unreachable();                                                 \
    }                                                                    \
    __noinline void __asan_report_store##size(void *addr) {              \
        report_error(addr, size, /*is_write=*/true, /*is_fatal=*/true);  \
        __unreachable();                                                 \
    }

#define ASAN_STACK_MALLOC_FREE_ID(id)                              \
    __noinline void *__asan_stack_malloc_##id(uptr size) {         \
        return stack_malloc(id, size);                             \
    }                                                              \
    __noinline void __asan_stack_free_##id(void *ptr, uptr size) { \
        return stack_free(id, ptr, size);                          \
    }

//
// required API
//

// whether we want to detect stack use after return
// not sure how to even implement this, so we disable it regardless of compiler flags
int __asan_option_detect_stack_use_after_return = 0;

// no-op, causes link error if compiler references a different version than implemented on this runtime
void __asan_version_mismatch_check_v8(void) {}

// runs before any instrumented code, no-op for now
void __asan_init(void) {}

// perform cleanup before a noreturn function
void __asan_handle_no_return(void) {}

// poison `n` global variables
void __asan_register_globals(void *globals, uptr n) {
    __asan_global *g = globals;
    for (uptr i = 0; i < n; i++) {
        poison_global(g + i);
    }
}

// unpoison `n` global variables
void __asan_unregister_globals(void *globals, uptr n) {
    __asan_global *g = globals;
    for (uptr i = 0; i < n; i++) {
        unpoison_global(g + i);
    }
}

// poison stack
void __asan_poison_stack_memory(void *start, uintptr_t n) {
    update_region((uptr)start, n, true);
}

// poison alloca
void __asan_alloca_poison(void *start, uintptr_t n) {
    update_region((uptr)start, n, true);
}

// unpoison alloca
void __asan_allocas_unpoison(void *addr, uintptr_t n) {
    update_region((uptr)addr, n, false);
}

// not used :/
// __noinline void __asan_report_ ## type ## size ## _noabort(void *addr) {
//     report_error(addr, is_write, size, 0, false);
// }

ASAN_REPORT_LOAD_STORE(1)
ASAN_REPORT_LOAD_STORE(2)
ASAN_REPORT_LOAD_STORE(4)
ASAN_REPORT_LOAD_STORE(8)
ASAN_REPORT_LOAD_STORE(16)

__noinline void __asan_report_load_n(void *start, uptr size) {
    access_check(start, size, /*is_write=*/false, /*fatal=*/true);
    __unreachable();
}

__noinline void __asan_report_store_n(void *start, uptr size) {
    access_check(start, size, /*is_write=*/true, /*fatal=*/true);
    __unreachable();
}

ASAN_STACK_MALLOC_FREE_ID(0)
ASAN_STACK_MALLOC_FREE_ID(1)
ASAN_STACK_MALLOC_FREE_ID(2)
ASAN_STACK_MALLOC_FREE_ID(3)
ASAN_STACK_MALLOC_FREE_ID(4)
ASAN_STACK_MALLOC_FREE_ID(5)
ASAN_STACK_MALLOC_FREE_ID(6)
ASAN_STACK_MALLOC_FREE_ID(7)
ASAN_STACK_MALLOC_FREE_ID(8)
ASAN_STACK_MALLOC_FREE_ID(9)
ASAN_STACK_MALLOC_FREE_ID(10)
