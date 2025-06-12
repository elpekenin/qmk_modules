// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/memory.h"

#include <stdint.h>

// from ChibiOS' ld
extern uint8_t __main_stack_base__, __main_stack_end__;
extern uint8_t __process_stack_base__, __process_stack_end__;
extern uint8_t __bss_end__;
extern uint8_t __flash_binary_start, __flash_binary_end;
extern uint8_t __flash1_base__, __flash1_end__;

bool ptr_in_heap(const void *ptr) {
    return (void *)&__bss_end__ <= ptr && ptr <= (void *)&__process_stack_end__;
}

bool ptr_in_main_stack(const void *ptr) {
    return (void *)&__main_stack_base__ <= ptr && ptr <= (void *)&__main_stack_end__;
}

bool ptr_in_process_stack(const void *ptr) {
    return (void *)&__process_stack_base__ <= ptr && ptr <= (void *)&__process_stack_end__;
}

bool ptr_in_stack(const void *ptr) {
    return ptr_in_main_stack(ptr) || ptr_in_process_stack(ptr);
}

// adapted from <https://forums.raspberrypi.com/viewtopic.php?t=347638>
size_t get_heap_size(void) {
    return &__process_stack_end__ - &__bss_end__;
}

#if defined(MCU_RP)
size_t get_flash_size(void) {
    return &__flash1_end__ - &__flash1_base__;
}

size_t get_used_flash(void) {
    return &__flash_binary_end - &__flash_binary_start;
}
#endif

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui/utils.h"

#    if defined(COMMUNITY_MODULE_STRING_ENABLE)
#        include "elpekenin/string.h"
#    else
#        error "Must enable 'elpekenin/string'"
#    endif

bool flash_init(ui_node_t *self) {
    flash_args_t *const args = self->args;
    args->last               = ~0;
    return ui_font_fits(self);
}

uint32_t flash_render(const ui_node_t *self, painter_device_t display) {
    flash_args_t *const args = self->args;

    const size_t flash = get_used_flash();
    if (args->last == flash) {
        goto exit;
    }

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    string_t str = str_new(30);

    str_append(&str, "Flash: ");
    pretty_bytes(&str, flash);
    str_append(&str, "/");
    pretty_bytes(&str, get_flash_size());

    if (!ui_text_fits(self, font, str.ptr)) {
        goto err;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, str.ptr);

    args->last = flash;

err:
    qp_close_font(font);

exit:
    return args->interval;
}
#endif
