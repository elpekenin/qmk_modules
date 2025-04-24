// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/memory.h"

// from ChibiOS' ld
extern uint8_t __main_stack_base__;
extern uint8_t __main_stack_end__;
extern uint8_t __process_stack_base__;
extern uint8_t __process_stack_end__;
extern uint8_t __bss_end__;
extern uint8_t __flash_binary_start;
extern uint8_t __flash_binary_end;
extern uint8_t __flash1_base__;
extern uint8_t __flash1_end__;

bool ptr_in_heap(const void *ptr) {
    return (void *)&__bss_end__ <= ptr && ptr <= (void *)&__process_stack_end__;
}

bool ptr_in_main_stack(const void *ptr) {
    return (void *)&__main_stack_base__ <= ptr && ptr <= (void *)&__main_stack_end__;
}

bool ptr_in_process_stack(const void *ptr) {
    return (void *)&__process_stack_base__ <= ptr && ptr <= (void *)&__process_stack_end__;
}

bool ptr_in_stack(const void *ptr) {
    return ptr_in_main_stack(ptr) || ptr_in_process_stack(ptr);
}

// adapted from <https://forums.raspberrypi.com/viewtopic.php?t=347638>
size_t get_heap_size(void) {
    return &__process_stack_end__ - &__bss_end__;
}

#if defined(MCU_RP)
size_t get_flash_size(void) {
    return &__flash1_end__ - &__flash1_base__;
}

size_t get_used_flash(void) {
    return &__flash_binary_end - &__flash_binary_start;
}
#endif
