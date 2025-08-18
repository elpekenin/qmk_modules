// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui/os.h"

#include "elpekenin/ui/utils.h"
#include "os_detection.h"

// clang-format off
static const char *os_names[] = {
    [OS_UNSURE]  = "unknown",
    [OS_LINUX]   = "linux",
    [OS_WINDOWS] = "windows",
    [OS_MACOS]   = "macos",
    [OS_IOS]     = "ios",
};
// clang-format on

bool os_init(ui_node_t *self) {
    return ui_font_fits(self);
}

ui_time_t os_render(const ui_node_t *self, painter_device_t display) {
    os_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const os_variant_t os  = detected_host_os();
    const char *const  str = os_names[os];

    if (ui_text_fits(self, font, str)) {
        qp_drawtext(display, self->start.x, self->start.y, font, str);
    }

    qp_close_font(font);

exit:
    return args->interval;
}
