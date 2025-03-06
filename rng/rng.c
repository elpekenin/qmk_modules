// Copyright 2023 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <platforms/timer.h>

#include <lib/lib8tion/lib8tion.h>

#include "elpekenin/rng.h"

void rng_set_seed(uint16_t seed) {
    random16_set_seed(seed);
}

uint16_t rng_min_max(uint16_t min, uint16_t max) {
    random16_add_entropy(timer_read32() + min + max);
    return random16_min_max(min, max);
}
