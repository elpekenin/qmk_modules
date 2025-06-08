// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "elpekenin/ui.h"

#ifndef UI_LAYER_REDRAW_INTERVAL
#    define UI_LAYER_REDRAW_INTERVAL 100
#endif

typedef struct {
    uint8_t  layer;
    uint16_t width;
} last_layer_t;

typedef struct {
    const uint8_t *font;
    uint32_t       timer;
    last_layer_t   last;
} layer_args_t;
STATIC_ASSERT(offsetof(layer_args_t, font) == 0, "UI will crash :)");

bool     layer_init(ui_node_t *self);
uint32_t layer_render(const ui_node_t *self, painter_device_t display);
