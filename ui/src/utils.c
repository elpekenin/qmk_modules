// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui.h"

typedef struct {
    const uint8_t *font;
} font_args_t;

typedef struct {
    const uint8_t *image;
} image_args_t;

bool ui_font_fits(ui_node_t *self) {
    font_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        return false;
    }

    const uint8_t line_height = font->line_height;
    qp_close_font(font);

    return line_height <= self->size.y;
}

bool ui_image_fits(ui_node_t *self) {
    image_args_t *args = self->args;

    const painter_image_handle_t image = qp_load_image_mem(args->image);
    if (image == NULL) {
        return false;
    }

    const uint16_t image_width  = image->width;
    const uint16_t image_height = image->height;
    qp_close_image(image);

    if (image_width > self->size.x || image_height > self->size.y) {
        return false;
    }

    return true;
}
