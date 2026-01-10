// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "progmem.h"

#ifdef AUTO_SYNC_ENABLE
#    define NUM_SYNC_VARIABLES_RAW ARRAY_SIZE(sync_variables)

uint8_t sync_variables_count(void) {
    return NUM_SYNC_VARIABLES_RAW;
}

sync_variable_t get_sync_variable(size_t index) {
    sync_variable_t value;
    memcpy_P(&value, &sync_variables[index], sizeof(sync_variable_t));
    return value;
}

// TODO: getter?
uint32_t auto_sync_state[NUM_SYNC_VARIABLES_RAW] = {0};
#endif
