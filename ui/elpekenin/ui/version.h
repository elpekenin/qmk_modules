// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "elpekenin/ui.h"

typedef struct {
    const uint8_t *font;
} version_args_t;
STATIC_ASSERT(offsetof(version_args_t, font) == 0, "UI will crash :)");

bool      version_qmk_init(ui_node_t *self);
ui_time_t version_qmk_render(const ui_node_t *self, painter_device_t display);

bool      version_date_init(ui_node_t *self);
ui_time_t version_date_render(const ui_node_t *self, painter_device_t display);
