// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum/quantum.h>

#include "py/cstack.h"
#include "py/gc.h"
#include "py/runtime.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

// ChibiOS-specific
extern uint8_t __main_stack_base__, __main_stack_end__;

static uint8_t py_heap[MICROPY_HEAP_SIZE] = {0};

/**
 * Initialize the MicroPython runtime.
 *
 * Configuration can be customized using `keyboard_post_init_micropython_{kb,user}()`,
 * as it is called between cstack+gc initialization and the actual initialization of MicroPython.
 */
void keyboard_post_init_micropython(void) {
    // dont consume too much, ChibiOS and/or QMK may need a fair amount too
    const uintptr_t stack_size     = &__main_stack_end__ - &__main_stack_base__;
    const uintptr_t mpy_stack_size = stack_size / 5;

    mp_cstack_init_with_top(&__main_stack_base__ + mpy_stack_size, mpy_stack_size);

    gc_init(py_heap, py_heap + sizeof(py_heap));

    // allow user to override default settings
    keyboard_post_init_micropython_kb();

    mp_init();
}

// NOTE: gc seems to crash the MCU, investigate further...
void housekeeping_task_micropython(void) {
#if MICROPY_ENABLE_GC && 0
    static uint32_t gc_timer = 0;
    if (timer_elapsed32(gc_timer) > 1000) {
        gc_timer = timer_read32();
        gc_collect();
    }
#endif
    housekeeping_task_micropython_kb();
}
