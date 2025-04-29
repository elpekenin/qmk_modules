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

// TODO: realloc
// TODO: handle (start + size) going OOB

/**
 * Small address sanitizer runtime.
 */

// -- barrier --

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "community_modules.h"
#include "printf/printf.h"

#if defined(COMMUNITY_MODULE_CRASH_ENABLE)
#    include <backtrace.h>
#endif

/**
 * How big redzones are.
 */
#ifndef KASAN_REDZONE_SIZE
#    define KASAN_REDZONE_SIZE (4)
#endif

/**
 * How many malloc entries to track.
 */
#ifndef KASAN_MALLOC_ARRAY_SIZE
#    define KASAN_MALLOC_ARRAY_SIZE (100)
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
// sanitizer implementation
//

typedef struct {
    uptr    addr;
    uint8_t offset;
} aligned_t;

// custom symbols on linker
extern uint8_t __kasan_shadow_base__, __kasan_shadow_end__;
const uptr     shadow_base = (uptr)&__kasan_shadow_base__;
const uptr     shadow_end  = (uptr)&__kasan_shadow_end__;

// ChibiOS
extern uint8_t __ram0_base__, __ram0_end__;
extern uint8_t __heap_base__, __heap_end__;

const uptr ram_base = (uptr)&__ram0_base__;
const uptr ram_end  = (uptr)&__ram0_end__;

const uptr heap_base = (uptr)&__heap_base__;
const uptr heap_end  = (uptr)&__heap_end__;

static bool kasan_active = false;

static inline uptr get_caller_pc(void) {
    return (uptr)__builtin_extract_return_addr(__builtin_return_address(0));
}

static void report_error(uptr start, uptr access_size, uptr pc, bool is_write, bool fatal) {
#if defined(COMMUNITY_MODULE_CRASH_ENABLE)
    const char *const func = backtrace_function_name(pc);
#else
    const char *const func = "<unknown function>";
    (void)pc;
#endif

    printf("[ERROR] asan: invalid %s of %d byte(s) at 0x%X (in %s)\n", is_write ? "write" : "load", access_size, start, func);

    while (fatal) {
    }
}

static aligned_t get_aligned_shadow(uptr addr) {
    const uptr alignment      = 8;
    const uptr alignment_mask = alignment - 1;

    aligned_t ret = {
        .addr   = addr & ~alignment_mask,
        .offset = addr & alignment_mask,
    };

    // not in the section of RAM we are monitoring
    if (ret.addr < ram_base || ret.addr > ram_end) {
        ret.addr = (uptr)NULL;
        return ret;
    }

    // 1. compute offset in bytes between aligned-addr and start of RAM
    // 2. convert it to a position in shadow mem by `/ 8` (each byte represented with a bit)
    const uptr offset = ret.addr - ram_base;
    ret.addr          = shadow_base + (offset / 8);

    return ret;
}

static void set_region(uptr start, uptr size, bool poison) {
    const aligned_t aligned_shadow = get_aligned_shadow(start);
    if (aligned_shadow.addr == 0) {
        return;
    }

    uint8_t *shadow = (void *)aligned_shadow.addr;
    uint8_t  offset = aligned_shadow.offset;

    // first bits
    if (offset != 0) {
        // at most, we can update (8 - offset) bits
        // number of bits to update is the minimum between that and n
        const uint8_t bits = MIN(size, (8 - offset));

        const uint8_t mask = ((1 << bits) - 1) << offset;
        if (poison) {
            *shadow |= mask;
        } else {
            *shadow &= ~mask;
        }

        size -= bits;
    }

    // complete bytes
    const uint8_t value   = poison ? 0xFF : 0;
    const uptr    n_bytes = size / 8;
    size                  = size % 8;
    memset(shadow, value, n_bytes);

    // last bits
    if (size > 0) {
        const uint8_t mask = (1 << size) - 1;

        if (poison) {
            *shadow |= mask;
        } else {
            *shadow &= ~mask;
        }
    }
}

static bool is_valid_access(uptr start, uptr access_size) {
    if (__predict_false(!kasan_active)) {
        return true;
    }

    const aligned_t aligned_shadow = get_aligned_shadow(start);
    if (aligned_shadow.addr == 0) {
        return true;
    }

    const uint8_t *shadow = (void *)aligned_shadow.addr;
    const uint8_t  offset = aligned_shadow.offset;

    // all bits unset, not hitting poison
    if (*shadow == 0) {
        return true;
    }

    // all bits set, hitting poison for sure
    if (*shadow == 0xFF) {
        return false;
    }

    /* at this point, we must perform actual check */

    // first bits
    if (offset != 0) {
        const uint8_t bits = MIN(access_size, (8 - offset));

        const uint8_t mask = ((1 << bits) - 1) << offset;
        if (*shadow & mask) {
            return false;
        }

        access_size -= bits;
    }

    // complete bytes
    while (access_size >= 8) {
        access_size -= 8;

        if (*shadow != 0) {
            return false;
        }
    }

    // last bits
    if (access_size > 0) {
        const uint8_t mask = (1 << access_size) - 1;

        if (*shadow & mask) {
            return false;
        }
    }

    return true;
}

static void kasan_init(void) {
    // initialize shadow with 0
    memset((void *)shadow_base, 0, shadow_end - shadow_base);

    // blacklist yet-unused RAM (aka: heap)
    set_region(heap_base, heap_end - heap_base, /*poison=*/true);

    kasan_active = true;
}

//
// track heap operations
//

typedef struct {
    void  *addr;
    size_t n;
} allocation_t;

extern void *__real_malloc(size_t);
extern void  __real_free(void *);

static allocation_t allocations[KASAN_MALLOC_ARRAY_SIZE] = {0};

static allocation_t *find_allocation(void *ptr) {
    for (uint8_t i = 0; i < KASAN_MALLOC_ARRAY_SIZE; ++i) {
        if (allocations[i].addr == ptr) {
            return &allocations[i];
        }
    }

    return NULL;
}

void *__wrap_malloc(size_t n) {
    void *ptr = __real_malloc(n);
    if (ptr == NULL) {
        return NULL;
    }

    allocation_t *alloc = find_allocation(NULL); // find empty slot
    if (alloc != NULL) {
        set_region((uptr)ptr, n, /*poison=*/false);

        *alloc = (allocation_t){
            .addr = ptr,
            .n    = n,
        };
    }

    return ptr;
}

void __wrap_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    allocation_t *alloc = find_allocation(ptr);
    if (alloc != NULL) {
        set_region((uptr)ptr, alloc->n, /*poison=*/true);

        *alloc = (allocation_t){
            .addr = NULL,
            .n    = 0,
        };
    }

    __real_free(ptr);
}

//
// required API
//

// perform cleanup before a noreturn function, no-op for now
void __asan_handle_no_return(void) {}

#if KASAN_GLOBALS
// poison `n` global variables
void __asan_register_globals(void *globals, uptr n) {
    for (uptr i = 0; i < n; i++) {
        struct __asan_global *global = (struct __asan_global *)globals + i;
        set_region(global->beg, global->n, /*poison=*/false);                         // valid
        set_region(global->beg + global->n, global->n_with_redzone, /*poison=*/true); // redzone
    }
}

// unpoison `n` global variables, never called
void __asan_unregister_globals(void *globals, uptr n) {}
#endif

#define ASAN_REPORT_LOAD_STORE(size)                                                     \
    void __asan_load##size##_noabort(void *addr) {                                       \
        const uptr s = (uptr)addr;                                                       \
                                                                                         \
        if (!is_valid_access(s, size)) {                                                 \
            report_error(s, size, get_caller_pc(), /*is_write=*/false, /*fatal=*/false); \
        }                                                                                \
    }                                                                                    \
    void __asan_store##size##_noabort(void *addr) {                                      \
        const uptr s = (uptr)addr;                                                       \
                                                                                         \
        if (!is_valid_access(s, size)) {                                                 \
            report_error(s, size, get_caller_pc(), /*is_write=*/true, /*fatal=*/false);  \
        }                                                                                \
    }

ASAN_REPORT_LOAD_STORE(1)
ASAN_REPORT_LOAD_STORE(2)
ASAN_REPORT_LOAD_STORE(4)
ASAN_REPORT_LOAD_STORE(8)
ASAN_REPORT_LOAD_STORE(16)

void __asan_loadN_noabort(void *start, uptr size) {
    const uptr s = (uptr)start;

    if (!is_valid_access(s, size)) {
        report_error(s, size, get_caller_pc(), /*is_write=*/false, /*fatal=*/false);
    }
}

void __asan_storeN_noabort(void *start, uptr size) {
    const uptr s = (uptr)start;

    if (!is_valid_access(s, size)) {
        report_error(s, size, get_caller_pc(), /*is_write=*/true, /*fatal=*/false);
    }
}

#if KASAN_ALLOCAS
// poison alloca
void __asan_alloca_poison(void *start, uintptr_t n) {
    // valid
    set_region((uptr)start, n, /*poison=*/false);

    // redzone on both sides (out-of-bounds access)
    set_region((uptr)start - KASAN_REDZONE_SIZE, KASAN_REDZONE_SIZE, /*poison=*/true);
    set_region((uptr)start + n, KASAN_REDZONE_SIZE, /*poison=*/true);
}

// unpoison alloca
void __asan_allocas_unpoison(void *start, uintptr_t n) {
    set_region((uptr)start, n, /*poison=*/false);
}
#endif

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

void keyboard_post_init_sanitizer(void) {
    kasan_init();
}
