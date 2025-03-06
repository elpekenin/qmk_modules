// Copyright 2024 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

typedef void (*init_fn)(void);

extern init_fn __preinit_array_base__;
extern init_fn __preinit_array_end__;

void keyboard_pre_init_sdk_wrappers(void) {
    for (init_fn *func = &__preinit_array_base__; func < &__preinit_array_end__; func++) {
        (*func)();
    }
}
