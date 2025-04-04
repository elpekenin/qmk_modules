// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/**
 * These headers are available when building QMK, but not when MicroPy collects QSTRs
 * As such, all .c files will indirectly include QMK headers by using this wrapper
 */

#if __has_include("quantum.h")
#    include "quantum.h"
#    include "version.h"
#    define COLLECTING_QSTR 0
#else
#    pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#    define COLLECTING_QSTR 1
#endif
