// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Small address sanitizer runtime.
 *
 * Compiler will insert calls to functions in this file for each pointer write/read, we then check if the address is "unavailable" and show a warning.
 *
 * You can tweak this module's behavior with the following flags on your makefile (set them to either ``0`` or ``1``)
 *   - ``KASAN_GLOBALS``: track global variables
 *   - ``KASAN_STACK``: extra checks on stack variables... Seems to crash as of now
 *   - ``KASAN_ALLOCAS``: extra checks for ``alloca()`` and VLA
 *   - ``KASAN_MALLOC``: wrap dynamic memory allocation, to mark it as available
 *
 * .. warning::
 *     To set this module up you need to define a custom linker script, defining the area in which the sanitizer stores information
 *     You must provide ``__kasan_shadow_base__`` (8-byte aligned) and ``__kasan_shadow_end__`` as the memory region where to work.
 *
 *     To do that on my RP2040, i've edited ChibiOS' linker script such that ram0 is now 8/9 of its original size, and the last 1/9 (now ram2) is used by sanitizer.
 *
 *     These numbers come from the fact that for tracking whether each memory location (byte) is available, we use a single bit.
 *     As such, the memory needed would be 1/8 of total RAM, but since we dont want to track this memory itself, we can do 1/9.
 *
 * .. danger::
 *    Adds significant RAM usage and some code slow down.
 *
 *    Only use it for debugging, or if your MCU has enough resources.
 */

// -- barrier --

#pragma once

/**
 * How big redzones are.
 */
#ifndef CONFIG_KASAN_REDZONE_SIZE
#    define CONFIG_KASAN_REDZONE_SIZE (4)
#endif

/**
 * How many malloc entries to track.
 */
#ifndef CONFIG_KASAN_MALLOC_ARRAY_SIZE
#    define CONFIG_KASAN_MALLOC_ARRAY_SIZE (300)
#endif

void kasan_init(void);
