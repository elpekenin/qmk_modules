// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Read GCC-provided build id.
 *
 * This identifier is stored on a special section, so you need to add something like this to your linker script:
 *
 *     .. code-block::
 *
 *        SECTIONS {
 *            .build_id : {
 *                __gnu_build_id__ = .;
 *                *(.note.gnu.build-id)
 *            } > FLASH
 *        }
 */

// -- barrier --

#pragma once

#include <quantum/compiler_support.h>
#include <stdint.h>

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
