// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/string.h"

#include <stdarg.h>
#include <stdint.h>

#define BIT(x) ((uint8_t)1 << (x))

size_t str_available(string_t str) {
    return str.size - str.used;
}

void str_reset(string_t *str) {
    str->used = 0;
}

int str_append(string_t *str, const char *text) {
    return str_printf(str, "%s", text);
}

int str_printf(string_t *str, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int ret = vsnprintf(str->ptr + str->used, str_available(*str), fmt, args);
    va_end(args);

    str->used += ret;

    return ret;
}

int pretty_bytes(string_t *str, size_t n) {
    // space for b to align with kb/mb/gb
    const static char *magnitudes[] = {" b", "kb", "mb", "gb", "tb", "pb"};

    int8_t index = 0;
    while (n >= 1024) {
        n /= 1024;
        index++;
    }

    return str_printf(str, "%3d%s", (int)n, magnitudes[index]);
}

bool is_utf8(char chr) {
    return (chr & BIT(7)) != 0; // 1xxx xxxx
}

bool is_utf8_continuation(char chr) {
    return is_utf8(chr) && (BIT(6) == 0); // 10xx xxxx
}
