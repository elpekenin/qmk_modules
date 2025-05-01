// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Reference implementation:
 *   - https://github.com/llvm/llvm-project/tree/main/compiler-rt/lib/asan
 *
 * Notes:
 *  - Had to change between `uptr` and `void *` in several places
 *  - Similarly, globals' functions changed from `__asan_global *` to `void *`
 *
 * TODO:
 *  - Check if start+size goes OOB
 */

#include "elpekenin/sanitizer/kasan.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>

#include "printf/printf.h"

#if defined(COMMUNITY_MODULE_CRASH_ENABLE)
#    include <backtrace.h>
#endif

#ifdef KASAN_DEBUG
#    include "quantum/logging/debug.h"
#    define kasan_dprintf dprintf
#else
#    define kasan_dprintf(...)
#endif

#define __nosan __attribute__((no_sanitize("address", "kernel-address")))

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
    uint8_t offset; // 0-7, u8 is enough
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

static inline __always_inline uptr get_caller_pc(void) {
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

__nosan static aligned_t get_aligned_shadow(uptr start, uptr size) {
    const uptr end = start + size;

    // not in the address range we are monitoring
    if (start < ram_base || end > ram_end) {
        return (aligned_t){
            .addr   = 0,
            .offset = 0,
        };
    }

    // end is out of shadow mem
    const uptr shadow_size = shadow_end - shadow_base;
    if ((size / 8) > shadow_size) {
        return (aligned_t){
            .addr   = 0,
            .offset = 0,
        };
    }

    const uptr alignment      = 8;
    const uptr alignment_mask = alignment - 1;

    const uptr aligned = start & ~alignment_mask;
    const uptr offset  = start & alignment_mask;

    const uptr sh_offset = aligned - ram_base;
    const uptr sh_addr   = shadow_base + (sh_offset / 8);

    return (aligned_t){
        .addr   = sh_addr,
        .offset = offset,
    };
}

__nosan static void set_region(uptr start, uptr size, bool poison) {
    const aligned_t aligned_shadow = get_aligned_shadow(start, size);
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

    const aligned_t aligned_shadow = get_aligned_shadow(start, access_size);
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

//
// track heap operations
//

typedef struct {
    const void *addr;
    size_t      n;
} allocation_t;

extern void *__real_malloc(size_t);
extern void  __real_free(void *);
extern void *__real_calloc(size_t, size_t);
extern void *__real_realloc(void *, size_t);

static allocation_t allocations[KASAN_MALLOC_ARRAY_SIZE] = {0};

__nosan static allocation_t *find_allocation(const void *ptr) {
    for (uptr i = 0; i < KASAN_MALLOC_ARRAY_SIZE; ++i) {
        if (allocations[i].addr == ptr) {
            return allocations + i;
        }
    }

    kasan_dprintf("could not find allocation slot with ptr=%p\n", ptr);
    return NULL;
}

__nosan void push_allocation(const void *ptr, size_t n) {
    allocation_t *alloc = find_allocation(NULL); // find empty slot
    if (alloc == NULL) {
        return;
    }

    set_region((uptr)ptr, n, /*poison=*/false);

    *alloc = (allocation_t){
        .addr = ptr,
        .n    = n,
    };
}

__nosan void pop_allocation(const void *ptr) {
    allocation_t *alloc = find_allocation(ptr);
    if (alloc == NULL) {
        return;
    }

    set_region((uptr)ptr, alloc->n, /*poison=*/true);

    *alloc = (allocation_t){
        .addr = NULL,
        .n    = 0,
    };
}

__nosan void *__wrap_malloc(size_t n) {
    void *ptr = __real_malloc(n);
    if (__predict_false(ptr == NULL)) {
        return NULL;
    }

    push_allocation(ptr, n);
    return ptr;
}

__nosan void __wrap_free(void *ptr) {
    __real_free(ptr);
    if (__predict_false(ptr == NULL)) {
        return;
    }

    pop_allocation(ptr);
}

__nosan void *__wrap_calloc(size_t nmemb, size_t size) {
    void *ptr = __real_calloc(nmemb, size);
    if (__predict_false(ptr == NULL)) {
        return NULL;
    }

    push_allocation(ptr, nmemb * size);
    return ptr;
}

__nosan void *__wrap_realloc(void *ptr, size_t size) {
    void *new_ptr = __real_realloc(ptr, size);
    if (__predict_false(new_ptr == NULL)) {
        return NULL;
    }

    pop_allocation(ptr);
    push_allocation(new_ptr, size);
    return ptr;
}

//
// required by runtime
//

// perform cleanup before a noreturn function, no-op for now
void __asan_handle_no_return(void) {
    kasan_dprintf("no-return cleanup invoked\n");
}

#if KASAN_GLOBALS
// poison `n` global variables
__nosan void __asan_register_globals(void *globals, uptr n) {
    kasan_dprintf("registering %d globals\n", n);

    for (uptr i = 0; i < n; i++) {
        struct __asan_global *global = (struct __asan_global *)globals + i;
        set_region(global->beg, global->n, /*poison=*/false);                         // valid
        set_region(global->beg + global->n, global->n_with_redzone, /*poison=*/true); // redzone
    }
}

// unpoison `n` global variables, never called
void __asan_unregister_globals(void *globals, uptr n) {
    kasan_dprintf("unregistering of %d globals was ignored\n", n);
}
#endif

#define ASAN_REPORT_LOAD_STORE(size)                                                     \
    void __asan_load##size##_noabort(void *addr) {                                       \
        const uptr s = (uptr)addr;                                                       \
        if (__predict_false(!is_valid_access(s, size))) {                                \
            report_error(s, size, get_caller_pc(), /*is_write=*/false, /*fatal=*/false); \
        }                                                                                \
    }                                                                                    \
                                                                                         \
    void __asan_store##size##_noabort(void *addr) {                                      \
        const uptr s = (uptr)addr;                                                       \
        if (__predict_false(!is_valid_access(s, size))) {                                \
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

    if (__predict_false(!is_valid_access(s, size))) {
        report_error(s, size, get_caller_pc(), /*is_write=*/false, /*fatal=*/false);
    }
}

void __asan_storeN_noabort(void *start, uptr size) {
    const uptr s = (uptr)start;

    if (__predict_false(!is_valid_access(s, size))) {
        report_error(s, size, get_caller_pc(), /*is_write=*/true, /*fatal=*/false);
    }
}

#if KASAN_ALLOCAS
// poison alloca
void __asan_alloca_poison(void *start, uptr n) {
    kasan_dprintf("poison alloca\n");

    set_region((uptr)start - KASAN_REDZONE_SIZE, KASAN_REDZONE_SIZE, /*poison=*/true); // redzone
    set_region((uptr)start, n, /*poison=*/false);                                      // valid
    set_region((uptr)start + n, KASAN_REDZONE_SIZE, /*poison=*/true);                  // redzone
}

// unpoison alloca
void __asan_allocas_unpoison(void *start, uptr n) {
    kasan_dprintf("unpoison alloca\n");
    set_region((uptr)start, n, /*poison=*/false);
}
#endif

//
// exposed API
//

void kasan_init(void) {
    kasan_dprintf("initializing shadow with 0s\n");
    memset((void *)shadow_base, 0, shadow_end - shadow_base);

    kasan_dprintf("poisoning unused RAM (heap)\n");
    set_region(heap_base, heap_end - heap_base, /*poison=*/true);

    kasan_active = true;
    kasan_dprintf("kasan enabled\n");
}
