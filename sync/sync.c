// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/sync.h"

#include <string.h>

static void sync_handler(uint8_t m2s_size, const void *m2s_buffer, uint8_t s2m_size, void *s2m_buffer) {
    memory_view_t *view = (memory_view_t *)m2s_buffer;
    memcpy(view->slice.addr, &view->value, view->slice.size);
}

void sync_variable(void *addr, size_t size) {
    // data is too big, can't send it
    if (size > SYNC_MAX_PAYLOAD_SIZE) return;

    memory_view_t view = {
        .slice =
            {
                .addr = addr,
                .size = size,
            },
    };

    memcpy(view.value, addr, size);

    transaction_rpc_send(ELPEKENIN_SYNC_ID, sizeof(memory_slice_t) + size, &view);
}

void keyboard_post_init_sync(void) {
    transaction_register_rpc(ELPEKENIN_SYNC_ID, sync_handler);
}

#ifdef AUTO_SYNC_ENABLE
extern uint32_t auto_sync_state[];

void housekeeping_task_sync(void) {
    if (!is_keyboard_master()) return;

    for (size_t i = 0; i < sync_variables_count(); ++i) {
        const sync_variable_t variable = get_sync_variable(i);

        const uint32_t last = auto_sync_state[i];
        if (timer_elapsed32(last) <= variable.ms_rate) continue;

        sync_variable(variable.slice.addr, variable.slice.size);
        auto_sync_state[i] = timer_read32();
    }
}
#endif
