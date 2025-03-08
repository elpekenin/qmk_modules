// Copyright 2023 Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Generate (pseudo-)random numbers.
 *
 * .. note::
 *    This is just a convenience layer on top of ``lib8tion.h`` right now,
 *    but may use another source in the future.
 */

/**
 * ----
 */

// -- barrier --

#pragma once

#include <stdint.h>

/**
 * Set the seed for the RNG. eg: invoking it from ``keyboard_post_init_user`` before consuming RNG.
 *
 * .. tip::
 *    You can for example call this:
 *      * The reading on a floating ADC pin
 *      * The value on some uninitialized memory address (only good entropy on cold start)
 *
 * .. warning::
 *    Setting a constant value means the PRNG sequence will be the same on every restart.
 */
void rng_set_seed(uint16_t seed);

/**
 * Generate a random number in the ``[min, max]`` range.
 *
 * It will also add some entropy to the RNG.
 */
uint16_t rng_min_max(uint16_t min, uint16_t max);
