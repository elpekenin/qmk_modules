#pragma once

#include <port/mpconfigport_common.h>

#ifndef MICROPY_CONFIG_ROM_LEVEL
#    define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES)
#endif

#ifndef MICROPY_ENABLE_COMPILER
#    define MICROPY_ENABLE_COMPILER (1)
#endif

#ifndef MICROPY_ENABLE_GC
#    define MICROPY_ENABLE_GC (1)
#endif

#ifndef MICROPY_HEAP_SIZE
#    define MICROPY_HEAP_SIZE (16 * 1024)
#endif

#ifndef MICROPY_PY_GC
#    define MICROPY_PY_GC (1)
#endif

#ifndef MICROPY_PY_SYS_PLATFORM
#    define MICROPY_PY_SYS_PLATFORM "QMK"
#endif

#ifndef MICROPY_PY_TIME_TIME_TIME_NS
#    define MICROPY_PY_TIME_TIME_TIME_NS (0)
#endif

#ifndef MP_PLAT_PRINT_STRN
#    define MP_PLAT_PRINT_STRN(str, len)          \
        do {                                      \
            extern int printf(const char *, ...); \
            printf("%.*s", (int)len, str);        \
        } while (0)
#endif
