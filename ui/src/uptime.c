// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/uptime.h"

#include "elpekenin/ui/utils.h"

bool uptime_init(ui_node_t *self) {
    return ui_font_fits(self);
}

uint32_t uptime_render(const ui_node_t *self, painter_device_t display) {
    uptime_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const div_t   days    = div(timer_read32(), DAYS(1));
    const div_t   hours   = div(days.rem, HOURS(1));
    const div_t   minutes = div(hours.rem, MINUTES(1));
    const uint8_t seconds = minutes.rem / SECONDS(1);

    char uptime[15] = {0};
    snprintf(uptime, sizeof(uptime), "Up|%02dh%02dm%02ds", hours.quot, minutes.quot, seconds);

    // text does not fit in bounds (width=0 means error)
    const int16_t width = qp_textwidth(font, uptime);
    if (width == 0 || width > self->size.x) {
        goto err;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, uptime);

err:
    qp_close_font(font);

exit:
    return SECONDS(1);
}
