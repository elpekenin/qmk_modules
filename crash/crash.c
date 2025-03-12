// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/crash.h"

#include <osal.h>
#include <quantum/quantum.h>

/* When set into a known address, flags that the program has crashed. */
#define MAGIC_VALUE (0xDEADA55)

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

__attribute__((section(".no_init"))) uint32_t magic;

static uint32_t copied_magic = 0;

typedef struct {
    uint8_t     stack_depth;
    backtrace_t call_stack[UNWIND_DEPTH];
    char        msg[10]; // slightly oversized
} crash_info_t;

__attribute__((section(".no_init"))) static crash_info_t crash_info;

backtrace_t *get_crash_call_stack(uint8_t *depth, const char **msg) {
    if (copied_magic != MAGIC_VALUE) {
        *depth = 0;
        *msg   = NULL;
        return NULL;
    }

    *depth = crash_info.stack_depth;
    *msg   = crash_info.msg;
    return crash_info.call_stack;
}

// copy magic from no-init variable
// then clear it, so we dont report same crash twice after restart
void keyboard_pre_init_crash(void) {
    copied_magic = magic;

    magic = 0;
}

// IRQ handler that will store the crash's cause and reset the controller (instead of deadloop or w/e)
__attribute__((noreturn)) static inline void handler(const char *msg) {
    magic                  = MAGIC_VALUE;
    crash_info.stack_depth = backtrace_unwind(crash_info.call_stack, UNWIND_DEPTH);
    strlcpy(crash_info.msg, msg, sizeof(crash_info.msg));

    NVIC_SystemReset();
}

__attribute__((interrupt)) void _unhandled_exception(void) {
    handler("Unknown");
}

__attribute__((interrupt)) void HardFault_Handler(void) {
    handler("Hard");
}

__attribute__((interrupt)) void BusFault_Handler(void) {
    handler("Bus");
}

__attribute__((interrupt)) void UsageFault_Handler(void) {
    handler("Usage");
}

__attribute__((interrupt)) void MemManage_Handler(void) {
    handler("MemMan");
}

// defined by ChibiOS, for context swap (?)
// HANDLER(NMI_Handler);
