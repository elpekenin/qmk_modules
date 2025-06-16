// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"

typedef void (*init_fn)(void);

extern init_fn __preinit_array_base__;
extern init_fn __preinit_array_end__;

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

void keyboard_pre_init_rp_sdk_wrappers(void) {
    for (init_fn *func = &__preinit_array_base__; func < &__preinit_array_end__; func++) {
        (*func)();
    }
}
