// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/uptime.h"

#include "elpekenin/ui/utils.h"

bool uptime_init(ui_node_t *self) {
    return ui_font_fits(self);
}

ui_time_t uptime_render(const ui_node_t *self, painter_device_t display) {
    uptime_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const div_t   days    = div(timer_read32(), UI_DAYS(1).value);
    const div_t   hours   = div(days.rem, UI_HOURS(1).value);
    const div_t   minutes = div(hours.rem, UI_MINUTES(1).value);
    const uint8_t seconds = minutes.rem / UI_SECONDS(1).value;

    char str[15] = {0};
    snprintf(str, sizeof(str), "Up|%02dh%02dm%02ds", hours.quot, minutes.quot, seconds);

    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return UI_SECONDS(1);
}
