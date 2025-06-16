// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/indicators.h"
#include "progmem.h"

#define NUM_INDICATORS_RAW ARRAY_SIZE(indicators)

size_t indicators_count(void) {
    return NUM_INDICATORS_RAW;
}

indicator_t get_indicator(size_t index) {
    indicator_t value;
    memcpy_P(&value, &indicators[index], sizeof(indicator_t));
    return value;
}
