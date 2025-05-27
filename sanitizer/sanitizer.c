// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum/quantum.h>

#include "elpekenin/sanitizer/kasan.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

#ifndef KASAN_INIT_DELAY
#    define KASAN_INIT_DELAY 3000
#endif

static uint32_t delayed_kasan_init(__unused uint32_t trigger_time, __unused void *cb_arg) {
    kasan_init();
    return 0;
}

// This is a funny one...
//   * Initializing kasan eagerly (on post_init) caused device to not even enumerate USB
//   * Same logic with `housekeeping` + `timer_read()` doesn't work either WTF, leave as is
void keyboard_post_init_sanitizer(void) {
    defer_exec(KASAN_INIT_DELAY, delayed_kasan_init, NULL);
}
