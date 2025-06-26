// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/crash.h"

#include <osal.h>

#include "logging/print.h"
#include "quantum.h"

#if defined(COMMUNITY_MODULE_STRING_ENABLE)
#    include "elpekenin/string.h"
#endif

#define __noinit __attribute__((section(".no_init")))
#define __interrupt _Noreturn __attribute__((interrupt))

/* When set into a known address, flags that the program has crashed. */
#define MAGIC_VALUE (0xDEADA55)

__noinit uint32_t magic;

static uint32_t copied_magic = 0;

__noinit static crash_info_t crash_info;

Option(crash_info_t) get_crash(void) {
    if (copied_magic == MAGIC_VALUE) {
        return Some(crash_info_t, crash_info);
    }

    return None(crash_info_t);
}

// store crash's cause and reset the controller (instead of deadloop or w/e)
//
// should be inline to prevent an extra stack frame in the backtrace
// but then, it wouldn't be possible to expose on the header
_Noreturn void exception(const char *reason) {
    magic                  = MAGIC_VALUE;
    crash_info.stack_depth = backtrace_unwind(crash_info.call_stack, CRASH_UNWIND_DEPTH);

    if (reason != NULL) {
        strlcpy(crash_info.msg, reason, sizeof(crash_info.msg));
    } else {
        // just in case we get to read it somehow
        crash_info.msg[0] = '\0';
    }

    NVIC_SystemReset();
}

__interrupt void _unhandled_exception(void) {
    exception("Unknown");
}

__interrupt void HardFault_Handler(void) {
    // Better error message for Cortex-M0 and M0+, based on:
    // https://community.arm.com/support-forums/f/embedded-and-microcontrollers-forum/3257/debugging-a-cortex-m0-hard-fault
#if defined(COMMUNITY_MODULE_STRING_ENABLE) && defined(__CORTEX_M) && __CORTEX_M == 0
    struct port_extctx interrupt_context;
    memcpy(&interrupt_context, (const void *)__get_PSP(), sizeof(struct port_extctx));

    string_t str = str_from_buffer(crash_info.msg);

    // some GCC versions error due to %b but others don't (?)
    // it is implemented in `printf`, so there's no problem
    //
    // clang-format off
    IGNORE_FORMAT_WARNING(
        str_printf(
            &str,
            "Hardfault at %ld ('%s') | Instruction=%b | xPSR=%lb",
            interrupt_context.pc,
            backtrace_function_name(interrupt_context.pc),
            *(uintptr_t*)interrupt_context.pc,
            interrupt_context.xpsr
        );
    );
    // clang-format on

    // not passing `str_get(str)` as argument
    // because `str_printf` already worked on that memory
    exception(NULL);
#endif
    exception("Hard");
}

__interrupt void BusFault_Handler(void) {
    exception("Bus");
}

__interrupt void UsageFault_Handler(void) {
    exception("Usage");
}

__interrupt void MemManage_Handler(void) {
    exception("MemMan");
}

// defined by ChibiOS, for context swap (?)
// HANDLER(NMI_Handler);

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

// copy magic from no-init variable
// then clear it, so we dont report same crash twice after restart
void keyboard_pre_init_crash(void) {
    copied_magic = magic;

    magic = 0;
}
