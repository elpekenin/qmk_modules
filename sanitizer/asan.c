// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Reference implementation:
 *   - https://github.com/llvm/llvm-project/tree/main/compiler-rt/lib/asan
 *
 * Notes:
 *  - Had to change between `uptr` and `void *` in several places
 *  - Similarly, globals' functions changed from `__asan_global *` to `void *`
 */

// TODO: malloc/free

/**
 * Small address sanitizer runtime.
 */

// -- barrier --

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "community_modules.h"
#include "printf/printf.h"

// can't use __nosanitizeaddress from <sys/cdefs.h> because it is clang-only and noop for GCC
#define __nosanitize __attribute__((no_sanitize_address))

// TODO?: Use linker to allocate the buffer's size, of exactly __ram0_size__ bytes
/**
 * How big the (tracked) RAM is.
 */
#ifndef ASAN_MEMORY_SIZE
// default to RP2040's RAM size
#    define ASAN_MEMORY_SIZE (264 * 1024)
#endif
_Static_assert(ASAN_MEMORY_SIZE % 8 == 0, "ASAN_MEMORY_SIZE must be a multiple of 8");

/**
 * How big redzones are.
 */
#ifndef ASAN_REDZONE_SIZE
#    define ASAN_REDZONE_SIZE (4)
#endif

//
// types
//

typedef uintptr_t uptr;

struct __asan_global_source_location {
    const char *filename;
    int         line_no;
    int         column_no;
};

typedef struct __asan_global_source_location __global_loc;

struct __asan_global {
    uptr          beg;
    uptr          n;
    uptr          n_with_redzone;
    const char   *name;
    const char   *module_name;
    uptr          has_dynamic_init;
    __global_loc *gcc_location;
    uptr          odr_indicator;
};

//
// internal
//

extern uint8_t __ram0_base__, __ram0_end__, __ram0_free__;
extern uint8_t __main_stack_base__, __main_stack_end__;
extern uint8_t __process_stack_base__, __process_stack_end__;

static const uptr ram_start = (uptr)&__ram0_base__;
static const uptr ram_end   = (uptr)&__ram0_end__;

#if 0
static const uptr ram_free  = (uptr)&__ram0_free__;

static const uptr main_stack_start = (uptr)&__main_stack_base__;
static const uptr main_stack_end   = (uptr)&__main_stack_end__;

static const uptr process_stack_start = (uptr)&__process_stack_base__;
static const uptr process_stack_end   = (uptr)&__process_stack_end__;
#endif

typedef struct {
    uptr    addr;
    uint8_t offset;
} aligned_t;

#define LOAD false
#define WRITE true

#define NON_POISON false
#define POISON true

#define NON_FATAL false
#define FATAL true

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

static bool asan_active = false;

static uint8_t shadow_mem[ASAN_MEMORY_SIZE / 8] = {0};

__nosanitize static void report_error(uptr start, uptr access_size, bool is_write, bool fatal) {
    printf("[ERROR] asan: %s %d byte(s) at %x\n", is_write ? "writing" : "loading", access_size, start);

    while (fatal) {
    }
}

static uint8_t *addr_to_shadow(uptr addr) {
    // out-of-bounds
    if (addr < ram_start || addr > ram_end) {
        return NULL;
    }

    uptr offset = addr - ram_start;
    return shadow_mem + (offset / 8);
}

__nosanitize static void set_region(uptr start, uptr size, bool poison) {
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
    if (__predict_false(!asan_active)) {
        return;
    }

    uptr n = access_size; // checked remaining

    const aligned_t aligned = align(start);
    const uint8_t  *shadow  = addr_to_shadow(aligned.addr);
    if (shadow == NULL) {
        return;
    }

    // fast path, no bits set
    if (*shadow == 0) {
        return;
    }

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

//
// required API
//

// perform cleanup before a noreturn function
void __asan_handle_no_return(void) {}

// poison `n` global variables
__nosanitize void __asan_register_globals(void *globals, uptr n) {
    struct __asan_global *g = globals;

    for (uptr i = 0; i < n; i++) {
        struct __asan_global global = g[i];

        // valid
        set_region(global.beg, global.n, NON_POISON);

        // redzone afterwards
        set_region(global.beg + global.n, global.n_with_redzone, POISON);
    }
}

// unpoison `n` global variables
__nosanitize void __asan_unregister_globals(void *globals, uptr n) {
    return;

    struct __asan_global *g = globals;

    for (uptr i = 0; i < n; i++) {
        struct __asan_global global = g[i];

        set_region(global.beg, global.n, POISON);
        set_region(global.beg + global.n, global.n_with_redzone, NON_POISON);
    }
}

#define ASAN_REPORT_LOAD_STORE(size)                      \
    void __asan_load##size##_noabort(void *addr) {        \
        check_region((uptr)addr, size, LOAD, NON_FATAL);  \
    }                                                     \
    void __asan_store##size##_noabort(void *addr) {       \
        check_region((uptr)addr, size, WRITE, NON_FATAL); \
    }

ASAN_REPORT_LOAD_STORE(1)
ASAN_REPORT_LOAD_STORE(2)
ASAN_REPORT_LOAD_STORE(4)
ASAN_REPORT_LOAD_STORE(8)
ASAN_REPORT_LOAD_STORE(16)

void __asan_loadN_noabort(void *start, uptr size) {
    check_region((uptr)start, size, LOAD, NON_FATAL);
}

void __asan_storeN_noabort(void *start, uptr size) {
    check_region((uptr)start, size, WRITE, NON_FATAL);
}

// poison alloca
void __asan_alloca_poison(void *start, uintptr_t n) {
    // valid
    set_region((uptr)start, n, NON_POISON);

    // redzone on both sides (out-of-bounds access)
    set_region((uptr)start - ASAN_REDZONE_SIZE, ASAN_REDZONE_SIZE, POISON);
    set_region((uptr)start + n, ASAN_REDZONE_SIZE, POISON);
}

// unpoison alloca
void __asan_allocas_unpoison(void *start, uintptr_t n) {
    set_region((uptr)start, n, NON_POISON);
}

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

void keyboard_post_init_sanitizer(void) {
    // FIXME:
#if 0
    // backlist unused RAM
    set_region(ram_free, ram_end - ram_free, POISON);

    // ... but not the stack
    set_region(main_stack_end, main_stack_end - main_stack_start, NON_POISON);
    set_region(process_stack_start, process_stack_end - main_stack_start, NON_POISON);
#endif

    asan_active = true;
    printf("asan: started\n");
}
