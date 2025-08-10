// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/rgb.h"

#include "elpekenin/ui/utils.h"
#include "rgb_matrix.h"

bool rgb_init(ui_node_t *self) {
    return ui_font_fits(self);
}

uint32_t rgb_mode_render(const ui_node_t *self, painter_device_t display) {
    rgb_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const char *str = rgb_matrix_get_mode_name(rgb_matrix_config.mode);

    bool fits = false;
    while (*str != '\0') {
        if (ui_text_fits(self, font, str)) {
            fits = true;
            break;
        }

        str++;
    }

    if (fits) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return args->interval;
}

uint32_t rgb_speed_render(const ui_node_t *self, painter_device_t display) {
    rgb_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    char str[4] = {0};
    snprintf(str, sizeof(str), "%d", rgb_matrix_config.speed);

    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return args->interval;
}

uint32_t rgb_hsv_render(const ui_node_t *self, painter_device_t display) {
    rgb_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const hsv_t hsv = rgb_matrix_config.hsv;

    char str[15] = {0};
    snprintf(str, sizeof(str), "%3d %3d %3d", hsv.h, hsv.s, hsv.v);

    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return args->interval;
}
