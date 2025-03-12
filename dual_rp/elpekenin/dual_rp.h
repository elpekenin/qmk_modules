// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Run code on the second core of your RP2040.
 */

/**
 * ----
 */

// -- barrier --

#pragma once

/**
 * Entrypoint of the second core.
 *
 * Its default implementation is:
 *    * Wait for first core to setup ChibiOS
 *    * Run ``c1_init_kb`` and ``c1_init_user``
 *    * In an endless loop, run ``c1_main_kb`` and ``c1_main_user``
 *
 * However, it is defined weakly, you can overwrite it.
 *
 * .. caution::
 *    If you do so, the functions mentioned above will no longer be called.
 */
void c1_main(void);

/**
 * Hook for keyboard-level initialization on the second core.
 */
void c1_init_kb(void);

/**
 * Hook for user-level initialization on the second core.
 */
void c1_init_user(void);

/**
 * Hook for keyboard-level logic on the second core.
 */
void c1_main_kb(void);

/**
 * Hook for user-level logic on the second core.
 */
void c1_main_user(void);
