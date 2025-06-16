// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/allocator.h"

void *__wrap_malloc(size_t total_size) {
    return malloc_with(c_runtime_allocator, total_size);
}

void __wrap_free(void *ptr) {
    return free_with(c_runtime_allocator, ptr);
}

void *__wrap_calloc(size_t nmemb, size_t size) {
    return calloc_with(c_runtime_allocator, nmemb, size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    return realloc_with(c_runtime_allocator, ptr, size);
}
