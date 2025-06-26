// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/dual_rp.h"

#include <ch.h>

__weak_symbol void c1_init_kb(void) {}
__weak_symbol void c1_init_user(void) {}

__weak_symbol void c1_main_kb(void) {}
__weak_symbol void c1_main_user(void) {}

__weak_symbol void c1_main(void) {
    chSysWaitSystemState(ch_sys_running);
    chInstanceObjectInit(&ch1, &ch_core1_cfg);
    chSysUnlock();

    c1_init_kb();
    c1_init_user();

    while (true) {
        c1_main_kb();
        c1_main_user();
    }
}
