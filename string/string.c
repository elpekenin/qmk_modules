// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/string.h"

#include <stdarg.h>
#include <stdint.h>

char *str_get(string_t str) {
    return str.ptr - str.used;
}

size_t str_available(string_t str) {
    return str.size - str.used;
}

void str_reset(string_t *str) {
    str->ptr  = str_get(*str);
    str->used = 0;
}

int str_append(string_t *str, const char *text) {
    return str_printf(str, "%s", text);
}

int str_printf(string_t *str, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int ret = vsnprintf(str->ptr, str_available(*str), fmt, args);
    va_end(args);

    str->ptr += ret;
    str->used += ret;

    return ret;
}

int pretty_bytes(string_t *str, size_t n) {
    // space for b to align with kb/mb/gb
    const static char *magnitudes[] = {" b", "kb", "mb", "gb"};

    int8_t index = 0;
    size_t copy  = n / 1024;
    while (copy >= 1024) {
        copy /= 1024;
        index++;
    }

    return str_printf(str, "%3d%s", (int)copy, magnitudes[index]);
}

bool is_utf8(char c) {
    return c & (1 << 7); // 1xxx xxxx
}

bool is_utf8_continuation(char c) {
    return is_utf8(c) && !(c & (1 << 6)); // 10xx xxxx
}
