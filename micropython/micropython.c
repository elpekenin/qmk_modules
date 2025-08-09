// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum/quantum.h>

#include "py/cstack.h"
#include "py/gc.h"
#include "py/runtime.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

// ChibiOS-specific
extern uint8_t __main_stack_base__[];

static uint8_t py_heap[MICROPY_HEAP_SIZE] = {0};

/**
 * Initialize the MicroPython runtime.
 *
 * Configuration can be customized using `keyboard_post_init_micropython_{kb,user}()`,
 * as it is called between cstack+gc initialization and the actual initialization of MicroPython.
 */
void keyboard_post_init_micropython(void) {
    // dont consume too much, ChibiOS and/or QMK may need a fair amount
    void *const stack_base = __main_stack_base__;
    void *const stack_top  = stack_base + MICROPY_QMK_STACK_SIZE;
    mp_cstack_init_with_top(stack_top, MICROPY_QMK_STACK_SIZE);

    gc_init(py_heap, py_heap + sizeof(py_heap));

    // allow user to override these default settings
    keyboard_post_init_micropython_kb();

    mp_init();
}

void housekeeping_task_micropython(void) {
#if MICROPY_ENABLE_GC
    static uint32_t gc_timer = 0;
    if (timer_elapsed32(gc_timer) > 1000) {
        gc_timer = timer_read32();
        gc_collect();
    }
#endif
    housekeeping_task_micropython_kb();
}
