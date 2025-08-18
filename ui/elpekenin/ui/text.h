// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "elpekenin/ui.h"

typedef struct {
    const uint8_t    *font;
    const char *const str;
    ui_time_t         interval;
} text_args_t;
STATIC_ASSERT(offsetof(text_args_t, font) == 0, "UI will crash :)");

bool      text_init(ui_node_t *self);
ui_time_t text_render(const ui_node_t *self, painter_device_t display);
