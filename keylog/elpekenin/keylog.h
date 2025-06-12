// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Utility to track last keys pressed in a string.
 *
 * This could later be shown on a screen, for example.
 */

// -- barrier --

#pragma once

#include <quantum/quantum.h>

/**
 * How big the array to store keylog will be.
 */
#ifndef KEYLOG_SIZE
#    define KEYLOG_SIZE 70
#endif

/**
 * Hook into :c:func:`process_record_user` that performs the tracking.
 */
void keylog_process(uint16_t keycode, keyrecord_t *record);

/**
 * Read the current state of the keylog.
 */
const char *get_keylog(void);

/**
 * Takes a basic string representation of a keycode and
 * replace it with a prettier one. Eg: ``KC_A`` becomes ``A``
 */
void keycode_repr(const char **str);

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui.h"

typedef struct {
    const uint8_t *font;
    uint32_t       interval;
} keylog_args_t;
STATIC_ASSERT(offsetof(keylog_args_t, font) == 0, "UI will crash :)");

bool     keylog_init(ui_node_t *self);
uint32_t keylog_render(const ui_node_t *self, painter_device_t display);
#endif
