// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "elpekenin/ui.h"

typedef struct {
    const uint8_t *font;
    ui_time_t      interval;
} rgb_args_t;
STATIC_ASSERT(offsetof(rgb_args_t, font) == 0, "UI will crash :)");

bool      rgb_init(ui_node_t *self);
ui_time_t rgb_mode_render(const ui_node_t *self, painter_device_t display);
ui_time_t rgb_speed_render(const ui_node_t *self, painter_device_t display);
ui_time_t rgb_hsv_render(const ui_node_t *self, painter_device_t display);
