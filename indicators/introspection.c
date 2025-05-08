// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <platforms/progmem.h>

#include "elpekenin/indicators.h"

#define NUM_INDICATORS_RAW (sizeof(indicators) / sizeof(indicator_t))

uint8_t indicators_count(void) {
    return NUM_INDICATORS_RAW;
}

indicator_t get_indicator(uint8_t index) {
    indicator_t value;
    memcpy_P(&value, &indicators[index], sizeof(indicator_t));
    return value;
}
