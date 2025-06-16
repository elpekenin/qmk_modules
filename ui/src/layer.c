// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/layer.h"

#include "compiler_support.h"
#include "elpekenin/ui/utils.h"

bool layer_init(ui_node_t *self) {
    layer_args_t *const args = self->args;
    if (args->layer_name == NULL) {
        return false;
    }

    args->last.layer = ~0;
    return ui_font_fits(self);
}

uint32_t layer_render(const ui_node_t *self, painter_device_t display) {
    layer_args_t *const args = self->args;

    const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (args->last.layer == layer) {
        goto exit;
    }

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const char *const str = args->layer_name(layer);

    const uint16_t width = qp_textwidth(font, str);
    if (width == 0 || width > self->size.x) {
        goto err;
    }

    if (args->last.width > width) {
        bool ret = qp_rect(display, self->start.x + width, self->start.y, self->start.x + args->last.width, self->start.y + font->line_height, HSV_BLACK, true);
        if (!ret) {
            goto err;
        }
    }

    qp_drawtext(display, self->start.x, self->start.y, font, str);

    args->last = (last_layer_t){
        .layer = layer,
        .width = width,
    };

err:
    qp_close_font(font);

exit:
    return args->interval;
}
