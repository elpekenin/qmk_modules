// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Tiny memory-usage and address "identification" functions.
 */

// -- barrier --

#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * Check if the input pointer belongs to the heap.
 */
bool ptr_in_heap(const void *addr);

/**
 * Check if the input pointer belongs to the main stack.
 */
bool ptr_in_main_stack(const void *addr);

/**
 * Check if the input pointer belongs to the process stack.
 */
bool ptr_in_process_stack(const void *addr);

/**
 * Check if the input pointer belongs to either stack.
 */
bool ptr_in_stack(const void *addr);

/**
 * Get the total size of the heap section.
 */
size_t get_heap_size(void);

#if defined(MCU_RP)
/**
 * Get amount of flash consumed by binary.
 *
 * .. warning::
 *    RP2040-only, for now.
 */
size_t get_used_flash(void);

/**
 * Get total size of the flash memory.
 *
 * .. warning::
 *    RP2040-only, for now.
 */
size_t get_flash_size(void);
#endif
