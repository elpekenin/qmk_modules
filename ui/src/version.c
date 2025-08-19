// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/version.h"

#include "elpekenin/ui/utils.h"
#include "version.h"

bool version_qmk_init(ui_node_t *self) {
    // QMK_VERSION is "xx.yy.zz-...." but the numbers can be 1 or 2 digits
    // as such, we must find the '-' separator, instead of hardcoding the width
    const char *const dash = strchr(QMK_VERSION, '-');
    if (dash == NULL) {
        return false;
    }

    return ui_font_fits(self);
}

ui_time_t version_qmk_render(const ui_node_t *self, painter_device_t display) {
    version_args_t *args = self->args;

    const char *const version = QMK_VERSION;
    const char *const dash    = strchr(version, '-'); // can't return NULL, we checked on init
    const size_t      n_chars = dash - version;

    char *const str = alloca(n_chars + 1); // separator
    memcpy(str, version, n_chars);
    str[n_chars] = '\0';

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return (ui_time_t)UI_SECONDS(1);
}

bool version_date_init(ui_node_t *self) {
    return ui_font_fits(self);
}

ui_time_t version_date_render(const ui_node_t *self, painter_device_t display) {
    version_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const char *const str = QMK_BUILDDATE;
    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return (ui_time_t)UI_SECONDS(1);
}
