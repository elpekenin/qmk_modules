// Copyright 2025 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#include "py/gc.h"
#include "py/runtime.h"
#include "py/cstack.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

// ChibiOS-specific
extern uint8_t __main_stack_end__;
extern uint8_t __main_stack_base__;

static uint8_t py_heap[MICROPY_HEAP_SIZE] = {0};

/**
 * Initialize the MicroPython runtime.
 *
 * Configuration can be customized using `keyboard_post_init_micropython_{kb,user}()`,
 * as it is called between cstack+gc initialization and the actual initialization of MicroPython.
 */
void keyboard_post_init_micropython(void) {
    // dont consume too much, ChibiOS and/or QMK may need a fair amount too
    mp_cstack_init_with_top(&__main_stack_end__, (&__main_stack_end__ - &__main_stack_base__) / 2);

    gc_init(py_heap, py_heap + sizeof(py_heap));

    // allow user to override default settings
    keyboard_post_init_micropython_kb();

    mp_init();
}
