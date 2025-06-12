// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/text.h"

#include "elpekenin/ui/utils.h"

bool text_init(ui_node_t *self) {
    text_args_t *args = self->args;

    if (!ui_font_fits(self)) {
        return false;
    }

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        return false;
    }

    bool ret = ui_text_fits(self, font, args->str);
    qp_close_font(font);
    return ret;
}

uint32_t text_render(const ui_node_t *self, painter_device_t display) {
    text_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, args->str);
    qp_close_font(font);

exit:
    return args->interval;
}
