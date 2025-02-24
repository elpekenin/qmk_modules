// Copyright 2025 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#include "py/gc.h"
#include "py/runtime.h"
#include "py/stackctrl.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

// ChibiOS-specific
extern uint8_t __main_stack_end__;
extern uint8_t __main_stack_base__;

static uint8_t py_heap[MICROPY_HEAP_SIZE] = {0};

void keyboard_post_init_micropython(void) {
    mp_stack_set_top(&__main_stack_end__);

    gc_init(py_heap, py_heap + sizeof(py_heap));

    // dont consume too much, ChibiOS and/or QMK may need a fair amount too
    mp_stack_set_limit((&__main_stack_end__ - &__main_stack_base__) / 2);

    // allow user to override the default settings above
    keyboard_post_init_micropython_kb();

    mp_init();
}
