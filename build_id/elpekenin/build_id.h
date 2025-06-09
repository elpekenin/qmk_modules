// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Read GCC-provided build id.
 *
 * This identifier is stored on a special section, so you need to add something like this to your linker script:
 *
 * .. code-block::
 *
 *     SECTIONS {
 *         .build_id : {
 *             __gnu_build_id__ = .;
 *             *(.note.gnu.build-id)
 *         } > FLASH
 *     }
 */

// -- barrier --

#pragma once

#include <stdint.h>

#include "compiler_support.h"

/**
 * MD5 id of a build is 128 bits long.
 *
 * Since there is no such type in C, we have this struct.
 */
typedef struct {
    /**
     * Bytes composing the 128 bits;
     */
    uint8_t bytes[128 / 8];
} u128;
STATIC_ASSERT(sizeof(u128) == 128 / 8, "Invalid size for `u128`");

/**
 * Get the build id for the running program.
 */
const u128 get_build_id(void);

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui.h"

#    ifndef BUILD_ID_UI_REDRAW_INTERVAL
#        define BUILD_ID_UI_REDRAW_INTERVAL 100
#    endif

typedef struct {
    const uint8_t *font;
    uint32_t       timer;
} build_id_args_t;
STATIC_ASSERT(offsetof(build_id_args_t, font) == 0, "UI will crash :)");

bool     build_id_init(ui_node_t *self);
uint32_t build_id_render(const ui_node_t *self, painter_device_t display);
#endif
