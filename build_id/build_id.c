// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/build_id.h"

// TODO?: assert that desc_size==128
typedef struct {
    uint32_t name_size;
    uint32_t desc_size;
    uint32_t type;
    uint8_t  data[];
} gnu_note_t;

extern gnu_note_t __gnu_build_id__;

const u128 get_build_id(void) {
    // skip the name
    const uint8_t *data = __gnu_build_id__.data;
    data += __gnu_build_id__.name_size;

    // re-interpret cast and copy contents
    u128 id = *(u128 *)data;

    return id;
}

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui/utils.h"

bool build_id_init(ui_node_t *self) {
    return ui_font_fits(self);
}

uint32_t build_id_render(const ui_node_t *self, painter_device_t display) {
    build_id_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const u128 id = get_build_id();

    //      0x   each byte in hex   null
    char str[2 + sizeof(u128) * 2 + 0] = {'0', 'x'};

    for (size_t i = 0; i < sizeof(u128); ++i) {
        const size_t offset = 2 + (2 * i);
        snprintf(str + offset, sizeof(str) - offset, "%x", id.bytes[i]);
    }

    // trim trailing chars until it fits
    uint16_t width = ~0;
    for (size_t i = sizeof(128); i > 1; --i) {
        const size_t offset = 2 + (2 * i) + 1;
        str[offset]         = '\0';

        width = qp_textwidth(font, str);
        if (width == 0) {
            goto err;
        }

        if (width <= self->size.x) {
            break;
        }
    }

    // can't even fit 0x<ab>, quit
    if (width > self->size.x) {
        goto err;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, str);

err:
    qp_close_font(font);

exit:
    return args->interval;
}
#endif
