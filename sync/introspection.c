// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "progmem.h"

#ifdef AUTO_SYNC_ENABLE
#    define NUM_SYNC_CONFIGS_RAW ARRAY_SIZE(sync_configs)

uint8_t sync_configs_count(void) {
    return NUM_SYNC_CONFIGS_RAW;
}

sync_config_t get_sync_config(size_t index) {
    sync_config_t value;
    memcpy_P(&value, &sync_configs[index], sizeof(sync_config_t));
    return value;
}

// TODO: getter?
sync_state_t auto_sync_states[NUM_SYNC_CONFIGS_RAW] = {0};
#endif
