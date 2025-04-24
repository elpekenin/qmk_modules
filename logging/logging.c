// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/logging.h"

#include <ch.h>
#include <errno.h>
#include <platforms/timer.h>
#include <quantum/quantum.h>
#include <stdarg.h>
#include <string.h>
#include <sys/cdefs.h>

// stringify log levels
// clang-format off
static const char *level_str[] = {
    [LOG_DEBUG] = "DEBUG",
    [LOG_INFO]  = "INFO",
    [LOG_WARN]  = "WARN",
    [LOG_ERROR] = "ERROR",
    [LOG_NONE]  = "UNREACHABLE",
};
// clang-format on
ASSERT_LEVELS(level_str);

static struct {
    log_level_t filter;
    log_level_t message;
} level = {
    .filter  = LOG_WARN, // by default show warns and errors
    .message = LOG_NONE,
};

log_level_t get_logging_level(void) {
    return level.filter;
}

void set_logging_level(log_level_t new_level) {
    if (new_level < LOG_DEBUG || new_level > LOG_NONE) {
        logging(LOG_ERROR, "Invalid logging level: %d", new_level);
    }

    level.filter = new_level;
}

void step_logging_level(bool increase) {
    if (level.filter == LOG_NONE && increase) {
        logging(LOG_ERROR, "Logging disabled, can't filter further");
        return;
    }

    if (level.filter == LOG_DEBUG && !increase) {
        logging(LOG_ERROR, "Logging everything, can't be more permissive");
        return;
    }

    if (increase) {
        level.filter++;
    } else {
        level.filter--;
    }
}

// internals
static token_t get_token(const char **str) {
    if (**str == '\0') { // null terminator
        return STR_END;
    }

    if (**str != '%') { // no specifier, regular text
        return NO_SPEC;
    }

    (*str)++;
    switch (**str) {
        case 'L':
            (*str)++;
            switch (**str) {
                case 'L': // %LL
                    return LL_SPEC;

                case 'S': // %LS
                    return LS_SPEC;

                default:
                    return INVALID_SPEC;
            }

        case 'M': // %M
            return M_SPEC;

        case 'T': // %T
            return T_SPEC;

        case '%': // %%
            return PERC_SPEC;

        default:
            return INVALID_SPEC;
    }

    __unreachable();
}

log_level_t get_current_message_level(void) {
    return level.message;
}

__weak_symbol const char *log_time(void) {
    static char buff[10] = {0};
    snprintf(buff, sizeof(buff), "%ld", timer_read32() / 1000);
    return buff;
}

static bool wrap_printf = true;

static MUTEX_DECL(logging_mutex);

int logging(log_level_t msg_level, const char *msg, ...) {
    int exitcode = 0;

    bool has_acquired_lock = false;

    // message filtered out, quit
    if (msg_level < level.filter) {
        goto exit;
    }

    va_list args;
    if (!wrap_printf) {
        va_start(args, msg);
        vprintf(msg, args);
        va_end(args);
        putchar_('\n');
        goto exit;
    }

    // (try) lock before running actual logic
    if (!chMtxTryLock(&logging_mutex)) {
        exitcode = -EBUSY;
        goto exit;
    }
    has_acquired_lock = true;

    // set msg lvel
    level.message = msg_level;

    const char *format = LOGGING_FORMAT;
    while (true) {
        // order specs alphabetically, special cases first
        switch (get_token(&format)) {
            case INVALID_SPEC: // reached when the logging format is invalid
                wrap_printf = false;
                exitcode    = -EINVAL;
                goto exit;

            case STR_END:
                level.message = LOG_NONE;
                goto exit;

            case NO_SPEC: // print any char
                putchar_(*format);
                break;

            case LL_SPEC: // print log level (long)
                printf("%s", level_str[msg_level]);
                break;

            case LS_SPEC: // print log level (short)
                putchar_(level_str[msg_level][0]);
                break;

            case M_SPEC: // print actual message
                va_start(args, msg);
                vprintf(msg, args);
                va_end(args);
                break;

            case PERC_SPEC: // print a '%'
                putchar_('%');
                break;

            case T_SPEC: // print current time
                printf("%s", log_time());
                break;
        }

        format++;
    }

exit:
    if (has_acquired_lock) {
        chMtxUnlock(&logging_mutex);
    }

    return exitcode;
}
