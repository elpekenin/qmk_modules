// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Reference implementation:
 *   - https://github.com/llvm/llvm-project/tree/main/compiler-rt/lib/asan
 *
 * Notes:
 *  - Had to change between `uintptr_t` and `void *` in several places
 */

/**
 * Small address sanitizer runtime.
 */

// -- barrier --

// TODO: Optimize using multi-bit read/write

#include <stdbool.h>
#include <stddef.h> // NULL
#include <stdint.h> // uintptr_t
#include <sys/cdefs.h>

#include "printf/printf.h"

/**
 * How big the array to track RAM usage will be.
 */
#ifndef ASAN_SHADOW_MEMORY_SIZE
// default to RP2040's RAM size
#    define ASAN_SHADOW_MEMORY_SIZE (264 / 8 * 1024)
#endif

/**
 * Memory address where RAM starts.
 */
#ifndef ASAN_SHADOW_MEMORY_START_ADDRESS
// default to ChibiOs symbol
extern uint8_t __ram0_base__;
#    define ASAN_SHADOW_MEMORY_START_ADDRESS ((void *)&__ram0_base__)
#endif

//
// types
//

typedef enum {
    ASAN_LOAD,
    ASAN_STORE,
} asan_operation_t;

typedef struct {
    const char *filename;
    int         line_no;
    int         column_no;
} __asan_global_source_location;

typedef struct {
    uintptr_t                      beg;
    uintptr_t                      n;
    uintptr_t                      n_with_redzone;
    const char                    *name;
    const char                    *module_name;
    uintptr_t                      has_dynamic_init;
    __asan_global_source_location *gcc_location;
    uintptr_t                      odr_indicator;
} __asan_global;

typedef struct {
    uintptr_t byte;
    uint8_t   bit;
} shadow_pos_t;

//
// internal
//

static uint8_t shadow[ASAN_SHADOW_MEMORY_SIZE] = {
    [0 ... ASAN_SHADOW_MEMORY_SIZE - 1] = 0xFF,
};

static const char *operation_as_str(asan_operation_t type) {
    switch (type) {
        case ASAN_LOAD:
            return "reading";

        case ASAN_STORE:
            return "storing";

        default:
            __unreachable();
    }
}

_Noreturn static void asan_error(asan_operation_t type, void *addr, uintptr_t size) {
    printf("[ERROR] asan: %s %d byte(s) at %p\n", operation_as_str(type), size, addr);
    while (1) {
        // deadloop
    }
}

static bool is_in_shadow_region(void *addr) {
    return ASAN_SHADOW_MEMORY_START_ADDRESS <= addr && addr <= (void *)((uintptr_t)ASAN_SHADOW_MEMORY_START_ADDRESS + ASAN_SHADOW_MEMORY_SIZE);
}

static shadow_pos_t addr_to_shadow(void *addr) {
    uintptr_t offset = addr - ASAN_SHADOW_MEMORY_START_ADDRESS;

    return (shadow_pos_t){
        .byte = offset / 8,
        .bit  = offset % 8,
    };
}

static bool is_poisoned(void *addr) {
    if (!is_in_shadow_region(addr)) {
        return false;
    }

    shadow_pos_t pos = addr_to_shadow(addr);
    return (shadow[pos.byte] & (1 << pos.bit)) != 0;
}

static void poison_addr(void *addr) {
    if (!is_in_shadow_region(addr)) {
        return;
    }

    if (is_poisoned(addr)) {
        printf("[ERROR] asan: %p was already poisoned\n", addr);
    }

    shadow_pos_t pos = addr_to_shadow(addr);
    shadow[pos.byte] |= (1 << pos.bit);
}

static void poison_region(void *start, uintptr_t n) {
    for (uintptr_t i = 0; i < n; i++) {
        poison_addr(start + i);
    }
}

static void unpoison_addr(void *addr) {
    if (!is_in_shadow_region(addr)) {
        return;
    }

    shadow_pos_t pos = addr_to_shadow(addr);
    shadow[pos.byte] &= ~(1 << pos.bit);
}

static void unpoison_region(void *start, uintptr_t n) {
    for (uintptr_t i = 0; i < n; i++) {
        unpoison_addr(start + i);
    }
}

static void *stack_malloc(__unused uintptr_t id, __unused uintptr_t n) {
    return NULL;
}

static void stack_free(__unused uintptr_t id, __unused void *addr, __unused uintptr_t n) {}

//
// config flags
//

// whether we want to detect stack use after return
// not sure how to even implement this, so we disable it regardless of compiler flags
int __asan_option_detect_stack_use_after_return = 0;

//
// required API
//

// no-op, causes link error if compiler references a different version than implemented on this runtime
void __asan_version_mismatch_check_v8(void) {}

// runs before any instrumented code, no-op for now
void __asan_init(void) {}

// perform cleanup before a noreturn function
void __asan_handle_no_return(void) {}

// track an array of `n` global variables
__nosanitizeaddress void __asan_register_globals(void *globals, uintptr_t n) {
    __asan_global *g = globals;
    for (uintptr_t i = 0; i < n; i++) {
        __asan_global global = g[i];
        poison_region((void *)global.beg, global.n);
    }
}

// stop tracking an array of `n` global variables
__nosanitizeaddress void __asan_unregister_globals(void *globals, uintptr_t n) {
    __asan_global *g = globals;
    for (uintptr_t i = 0; i < n; i++) {
        __asan_global global = g[i];
        unpoison_region((void *)global.beg, global.n);
    }
}

// poison stack
void __asan_poison_stack_memory(void *addr, uintptr_t n) {
    poison_region(addr, n);
}

// poison for alloca (?)
void __asan_alloca_poison(void *addr, uintptr_t n) {
    poison_region(addr, n);
}

// clear poison
void __asan_allocas_unpoison(void *addr, uintptr_t n) {
    unpoison_region(addr, n);
}

#define REPORT_LOAD_FUNC(size)                            \
    _Noreturn void __asan_report_load##size(void *addr) { \
        asan_error(ASAN_LOAD, addr, size);                \
    }
REPORT_LOAD_FUNC(1)
REPORT_LOAD_FUNC(2)
REPORT_LOAD_FUNC(4)
REPORT_LOAD_FUNC(8)
REPORT_LOAD_FUNC(16)
_Noreturn void __asan_report_load_n(void *addr, uintptr_t size) {
    asan_error(ASAN_LOAD, addr, size);
}

#define REPORT_STORE_FUNC(size)                            \
    _Noreturn void __asan_report_store##size(void *addr) { \
        asan_error(ASAN_STORE, addr, size);                \
    }
REPORT_STORE_FUNC(1)
REPORT_STORE_FUNC(2)
REPORT_STORE_FUNC(4)
REPORT_STORE_FUNC(8)
REPORT_STORE_FUNC(16)
_Noreturn void __asan_report_store_n(void *addr, uintptr_t size) {
    asan_error(ASAN_STORE, addr, size);
}

#define STACK_MALLOC_FUNC(id)                    \
    void __asan_stack_malloc_##id(uintptr_t n) { \
        stack_malloc(id, n);                     \
    }
STACK_MALLOC_FUNC(0)
STACK_MALLOC_FUNC(1)
STACK_MALLOC_FUNC(2)
STACK_MALLOC_FUNC(3)
STACK_MALLOC_FUNC(4)
STACK_MALLOC_FUNC(5)
STACK_MALLOC_FUNC(6)
STACK_MALLOC_FUNC(7)
STACK_MALLOC_FUNC(8)
STACK_MALLOC_FUNC(9)
STACK_MALLOC_FUNC(10)

#define STACK_FREE_FUNC(id)                                \
    void __asan_stack_free_##id(void *addr, uintptr_t n) { \
        stack_free(id, addr, n);                           \
    }
STACK_FREE_FUNC(0)
STACK_FREE_FUNC(1)
STACK_FREE_FUNC(2)
STACK_FREE_FUNC(3)
STACK_FREE_FUNC(4)
STACK_FREE_FUNC(5)
STACK_FREE_FUNC(6)
STACK_FREE_FUNC(7)
STACK_FREE_FUNC(8)
STACK_FREE_FUNC(9)
STACK_FREE_FUNC(10)
