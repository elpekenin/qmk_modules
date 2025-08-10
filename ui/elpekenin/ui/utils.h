// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

// NOTE: Not a builtin integration to display things on your keyboard.
//       These are just a few functions that are reused across them.

#pragma once

#include "elpekenin/ui.h"

bool ui_font_fits(const ui_node_t *self);
bool ui_image_fits(const ui_node_t *self);
bool ui_text_fits(const ui_node_t *self, painter_font_handle_t font, const char *str);
