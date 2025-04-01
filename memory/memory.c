// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/memory.h"

#define SYMBOL(name) extern uint8_t name
#define ADDR(__var) ((void *)&__var)

// from ChibiOS' ld
SYMBOL(__main_stack_base__);
SYMBOL(__main_stack_end__);
SYMBOL(__process_stack_base__);
SYMBOL(__process_stack_end__);
SYMBOL(__bss_end__);
SYMBOL(__flash_binary_start);
SYMBOL(__flash_binary_end);
SYMBOL(__flash1_base__);
SYMBOL(__flash1_end__);

bool ptr_in_heap(const void *ptr) {
    return ADDR(__bss_end__) <= ptr && ptr <= ADDR(__process_stack_end__);
}

bool ptr_in_main_stack(const void *ptr) {
    return ADDR(__main_stack_base__) <= ptr && ptr <= ADDR(__main_stack_end__);
}

bool ptr_in_process_stack(const void *ptr) {
    return ADDR(__process_stack_base__) <= ptr && ptr <= ADDR(__process_stack_end__);
}

bool ptr_in_stack(const void *ptr) {
    return ptr_in_main_stack(ptr) || ptr_in_process_stack(ptr);
}

// adapted from <https://forums.raspberrypi.com/viewtopic.php?t=347638>
size_t get_heap_size(void) {
    return ADDR(__process_stack_end__) - ADDR(__bss_end__);
}

#if defined(MCU_RP)
size_t get_flash_size(void) {
    return ADDR(__flash1_end__) - ADDR(__flash1_base__);
}

size_t get_used_flash(void) {
    return ADDR(__flash_binary_end) - ADDR(__flash_binary_start);
}
#endif
