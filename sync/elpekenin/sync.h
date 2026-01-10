// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Synchronize variables over split comms.
 *
 * Values are synchronized by writing to their memory addresses. As such, the variables need to be on the same address on both sides.
 *   - ☑ Global variables
 *   - ☐ Local variables (stack memory)
 *   - ☑ Local variables in a function marked as ``static``
 *   - ☐ Dynamically-allocated variables (heap)
 *
 * NOTE: Community modules don't yet support custom IDs, you must add ``ELPEKENIN_SYNC_ID`` to your ``SPLIT_TRANSACTION_IDS_USER`` in ``config.h``
 */

// -- barrier --

#pragma once

#if !defined(SPLIT_KEYBOARD)
#    error "Sync doesn't make sense on non-split keyboards"
#endif

#include <stddef.h>

#include "compiler_support.h"
#include "transactions.h"
#include "util.h"

typedef struct PACKED {
    void  *addr;
    size_t size;
} memory_slice_t;

typedef struct PACKED {
    memory_slice_t slice;
    uint32_t       ms_rate;
} sync_variable_t;

#define SYNC_MAX_PAYLOAD_SIZE (RPC_M2S_BUFFER_SIZE - sizeof(memory_slice_t))

typedef struct PACKED {
    memory_slice_t slice;
    uint8_t        value[SYNC_MAX_PAYLOAD_SIZE];
} memory_view_t;

void sync_variable(void *addr, size_t size);

/**
 * Sync the value of ``variable`` to slave side.
 *
 * .. code-block:: c
 *
 *     #include "elpekenin/sync.h"
 *
 *     uint8_t my_variable = 0;
 *
 *     bool process_record_user(uint16_t keycode, keyrecord_t *record) {
 *         if (keycode == MY_KEYCODE && record->event.pressed) {
 *             my_variable += 1;
 *             SYNC_VARIABLE(my_variable);
 *             return false;
 *         }
 *
 *         return true;
 *     }
 */
#define SYNC_VARIABLE(variable)                                                          \
    do {                                                                                 \
        STATIC_ASSERT(sizeof(variable) <= SYNC_MAX_PAYLOAD_SIZE, "Variable is too big"); \
        sync_variable(&(variable), sizeof(variable));                                    \
    } while (0)

#define AUTO_SYNC_VARIABLE(variable, rate) \
    {                                      \
        .slice =                           \
            {                              \
                .addr = &(variable),       \
                .size = sizeof(variable),  \
            },                             \
        .ms_rate = (rate),                 \
    }

/**
 * You can also define a list of variables to be synched automatically by the module.
 *
 * For this, you need to add ``#define AUTO_SYNC_ENABLE`` on ``config.h``
 *
 * The sync will happen at a timely rate, and module won't check whether the value has changed.
 * As such, be careful not to use too-low of a value, slowing your keyboard down.
 *
 * NOTE: Because the array has to be global, it can only reference/sync other global variables.
 *
 * .. code-block:: c
 *
 *     #include "elpekenin/sync.h"
 *
 *     size_t foo = 0;
 *     struct { uint8_t x; bool y; } bar = {0};
 *
 *     // time in milliseconds
 *     const sync_variable_t PROGMEM sync_variables[] = {
 *         AUTO_SYNC_VARIABLE(foo, 200),
 *         AUTO_SYNC_VARIABLE(bar, 1000),
 *     };
 */

// Not intended to be used by users -> no docstring
uint8_t sync_variables_count(void);

sync_variable_t get_sync_variable(size_t index);
