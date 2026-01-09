// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Synchronize an arbitrary variable over split comms
 *
 * Since the value is referenced by its memory address, it needs to have static lifetime.
 * That is: global variable or ``static`` variable in a function.
 *
 * NOTE: Community modules don't yet support custom IDs, you must add ``ELPEKENIN_SYNC_ID`` to your ``SPLIT_TRANSACTION_IDS_USER `` in ``config.h``
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

#define SYNC_MAX_PAYLOAD_SIZE (RPC_M2S_BUFFER_SIZE - sizeof(memory_slice_t))

typedef struct PACKED {
    memory_slice_t slice;
    uint8_t        value[SYNC_MAX_PAYLOAD_SIZE];
} memory_view_t;

void sync_variable(void *addr, size_t size);

/**
 * Sync `variable` across halves.
 */
#define SYNC_VARIABLE(variable)                                                          \
    do {                                                                                 \
        STATIC_ASSERT(sizeof(variable) <= SYNC_MAX_PAYLOAD_SIZE, "Variable is too big"); \
        sync_variable(&(variable), sizeof(variable));                                    \
    } while (0)
