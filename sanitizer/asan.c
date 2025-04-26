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

#include "printf/printf.h"

/* can't use __nosanitizeaddress from <sys/cdefs.h> because it is clang-only and noop for GCC */
#define __nosanitize __attribute__((no_sanitize_address))

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

typedef struct {
    uptr    addr;
    uint8_t offset;
} aligned_t;

//
// internal
//

// convert a pointer to given alignment
// eg: start=0x203,alignment=8 -> aligned=0x200, offset=3
static aligned_t align(uptr addr) {
    const uptr alignment      = 8;
    const uptr alignment_mask = alignment - 1;

    return (aligned_t){
        .addr   = addr & ~alignment_mask,
        .offset = addr & alignment_mask,
    };
}

static uint8_t shadow_mem[ASAN_MEMORY_SIZE / 8];

__nosanitize static void report_error(uptr start, uptr access_size, bool is_write, bool fatal) {
    printf("[ERROR] asan: %s %d byte(s) at %x\n", is_write ? "writing" : "loading", access_size, start);

    if (!fatal) return;

    while (true) {
    }
}

static uint8_t *addr_to_shadow(uptr addr) {
    // out-of-bounds
    if (addr < (uptr)ASAN_MEMORY_START_ADDRESS) return NULL;
    if (addr > (uptr)ASAN_MEMORY_START_ADDRESS + ASAN_MEMORY_SIZE) return NULL;

    uptr offset = addr - (uptr)ASAN_MEMORY_START_ADDRESS;
    return shadow_mem + (offset / 8);
}

__attribute__((optimize("-fno-tree-loop-distribute-patterns"))) // prevent memset replacing the loop
__nosanitize static void
set_region(uptr start, uptr size, bool poison) {
    uptr n = size; // updates remaining

    const aligned_t aligned = align(start);

    uint8_t *shadow = addr_to_shadow(aligned.addr);
    if (shadow == NULL) {
        return;
    }

    // handle first byte
    if (aligned.offset != 0) {
        // at most, we can update (8 - offset) bits
        // number of bits to update is the minimum between that and n
        const uint8_t bits = MIN(n, (8 - aligned.offset));

        const uint8_t mask = ((1 << bits) - 1) << aligned.offset;
        if (poison) {
            *shadow |= mask;
        } else {
            *shadow &= ~mask;
        }

        n -= bits;
    }

    // packs of 8 bytes updated in batches
    while (n >= 8) {
        n -= 8;

        *shadow = poison ? 0xFF : 0;
    }

    // update leftover bits
    if (n > 0) {
        const uint8_t mask = (1 << n) - 1;

        if (poison) {
            *shadow |= mask;
        } else {
            *shadow &= ~mask;
        }
    }
}

__nosanitize static void check_region(uptr start, uptr access_size, bool is_write, bool fatal) {
    uptr n = access_size; // checked remaining

    const aligned_t aligned = align(start);
    const uint8_t  *shadow  = addr_to_shadow(aligned.addr);
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

    // slow path

    if (aligned.offset != 0) {
        const uint8_t bits = MIN(n, (8 - aligned.offset));

        const uint8_t mask = ((1 << bits) - 1) << aligned.offset;
        if (*shadow & mask) {
            report_error(start, access_size, is_write, fatal);
            return;
        }

        n -= bits;
    }

    while (n >= 8) {
        n -= 8;

        if (*shadow != 0) {
            report_error(start, access_size, is_write, fatal);
            return;
        }
    }

    if (n > 0) {
        const uint8_t mask = (1 << n) - 1;

        if (*shadow & mask) {
            report_error(start, access_size, is_write, fatal);
            return;
        }
    }
}

__nosanitize void poison_global(__asan_global *global) {
    set_region(global->beg, global->n, false);
    set_region(global->beg + global->n, global->n_with_redzone, true);
}

__nosanitize void unpoison_global(__asan_global *global) {
    set_region(global->beg, global->n, true);
    set_region(global->beg + global->n, global->n_with_redzone, false);
}

//
// macros for repetitive functions
//

#define ASAN_REPORT_LOAD_STORE(size)                                            \
    void __asan_load##size##_noabort(void *addr) {                              \
        check_region((uptr)addr, size, /*is_write=*/false, /*is_fatal=*/false); \
    }                                                                           \
    void __asan_store##size##_noabort(void *addr) {                             \
        check_region((uptr)addr, size, /*is_write=*/true, /*is_fatal=*/false);  \
    }

//
// required API
//

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

// poison alloca
void __asan_alloca_poison(void *start, uintptr_t n) {
    set_region((uptr)start, n, true);
}

// unpoison alloca
void __asan_allocas_unpoison(void *addr, uintptr_t n) {
    set_region((uptr)addr, n, false);
}

ASAN_REPORT_LOAD_STORE(1)
ASAN_REPORT_LOAD_STORE(2)
ASAN_REPORT_LOAD_STORE(4)
ASAN_REPORT_LOAD_STORE(8)
ASAN_REPORT_LOAD_STORE(16)

void __asan_loadN_noabort(void *start, uptr size) {
    check_region((uptr)start, size, /*is_write=*/false, /*fatal=*/false);
}

void __asan_storeN_noabort(void *start, uptr size) {
    check_region((uptr)start, size, /*is_write=*/true, /*fatal=*/false);
}
