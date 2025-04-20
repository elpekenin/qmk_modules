// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/crash.h"

#include <osal.h>
#include <quantum/quantum.h>

#if defined(COMMUNITY_MODULE_STRING_ENABLE)
#    include "elpekenin/string.h"
#endif

/* When set into a known address, flags that the program has crashed. */
#define MAGIC_VALUE (0xDEADA55)

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

__attribute__((section(".no_init"))) uint32_t magic;

static uint32_t copied_magic = 0;

__attribute__((section(".no_init"))) static crash_info_t crash_info;

Option(crash_info_t) get_crash_call_stack(void) {
    if (copied_magic != MAGIC_VALUE) {
        return Some(crash_info_t, crash_info);
    }

    return None(crash_info_t);
}

// copy magic from no-init variable
// then clear it, so we dont report same crash twice after restart
void keyboard_pre_init_crash(void) {
    copied_magic = magic;

    magic = 0;
}

// IRQ handler that will store the crash's cause and reset the controller (instead of deadloop or w/e)
// inline to prevent an extra stack frame in the bactracke
__attribute__((noreturn)) __attribute__((always_inline)) static inline void handler(const char *msg) {
    magic                  = MAGIC_VALUE;
    crash_info.stack_depth = backtrace_unwind(crash_info.call_stack, UNWIND_DEPTH);

    if (msg != NULL) {
        strlcpy(crash_info.msg, msg, sizeof(crash_info.msg));
    } else {
        // just in case we get to read it somehow
        crash_info.msg[0] = '\0';
    }

    NVIC_SystemReset();
}

__attribute__((interrupt)) void _unhandled_exception(void) {
    handler("Unknown");
}

__attribute__((interrupt)) void HardFault_Handler(void) {
    // Better error message for Cortex-M0 and M0+, based on:
    // https://community.arm.com/support-forums/f/embedded-and-microcontrollers-forum/3257/debugging-a-cortex-m0-hard-fault
#if defined(COMMUNITY_MODULE_STRING_ENABLE) && defined(__CORTEX_M) && __CORTEX_M == 0
    struct port_extctx interrupt_context;
    memcpy(&interrupt_context, (const void *)__get_PSP(), sizeof(struct port_extctx));

    string_t str = str_from_buffer(crash_info.msg);
    // clang-format off
    str_printf(
        &str,
        "Hardfault at %ld ('%s') | Instruction=%b | xPSR=%lb",
        interrupt_context.pc,
        backtrace_function_name(interrupt_context.pc),
        *(uintptr_t*)interrupt_context.pc,
        interrupt_context.xpsr
    );
    // clang-format on

    // not passing `str_get(str)` as argument
    // because `str_printf` already worked on that memory
    handler(NULL);
#endif
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
