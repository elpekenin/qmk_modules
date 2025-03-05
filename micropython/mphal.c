/* This file is part of the MicroPython project, http://micropython.org/
 * The MIT License (MIT)
 * Copyright (c) 2022-2023 Damien P. George
 */

// Some OS-like glue functions

// TODO:
//   * hook `import` into QMK filesystem API
//   * hook pin-control functions into QMK GPIO/UART/SPI/... API
//   * hook timing APIs into `timer_read` and whatnot

// DONE:
//   * hook print into lib/printf

#include <stdio.h>
#include "py/builtin.h"
#include "py/compile.h"
#include "py/mperrno.h"
#if MICROPY_PY_SYS_STDFILES
#    include "py/stream.h"
#endif

#if !MICROPY_VFS

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

#endif

#if MICROPY_PY_ASYNCIO

mp_uint_t mp_hal_ticks_ms(void) {
    return timer_read32();
}

#endif

#if MICROPY_PY_TIME

void mp_hal_delay_ms(mp_uint_t ms) {
    wait_ms(ms);
}

void mp_hal_delay_us(mp_uint_t us) {
    wait_ms(us / 1000);
}

mp_uint_t mp_hal_ticks_us(void) {
    return timer_read32() * 1000;
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return 0;
}

#endif

#if MICROPY_PY_SYS_STDFILES && !MICROPY_VFS_POSIX

// Binary-mode standard input
// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
    return 'X';
}

// Binary-mode standard output
// Send the string of given length.
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    printf("tx: %.*s", (int)len, str);
    return len;
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if ((poll_flags & MP_STREAM_POLL_RD) /* && can_read */) {
        ret |= MP_STREAM_POLL_RD;
    }
    if ((poll_flags & MP_STREAM_POLL_WR) /* && can_write */) {
        ret |= MP_STREAM_POLL_WR;
    }
    return ret;
}

#endif
