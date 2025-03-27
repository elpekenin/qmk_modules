// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/logging.h"

#include <ch.h>
#include <errno.h>
#include <platforms/timer.h>
#include <quantum/quantum.h>
#include <stdarg.h>
#include <string.h>

// stringify log levels
// clang-format off
static const char *level_str[] = {
    [LOG_NONE]  = "UNREACHABLE",
    [LOG_DEBUG] = "DEBUG",
    [LOG_INFO]  = "INFO",
    [LOG_WARN]  = "WARN",
    [LOG_ERROR] = "ERROR",
};
// clang-format on
ASSERT_LEVELS(level_str);

// stringify features
// clang-format off
static const char *feature_str[] = { // sorted alphabetically
    [UNKNOWN] = "",
    [ALLOC]   = "ALLOC",
    [MAP]     = "MAP",
    [LOGGER]  = "LOG",
    [QP]      = "QP",
    [SCROLL]  = "SCROLL",
    [SIPO]    = "SIPO",
    [SPLIT]   = "SPLIT",
    [SPI]     = "SPI",
    [TOUCH]   = "TOUCH",
};
// clang-format on
ASSERT_FEATURES(feature_str);

// logging level for each feature
// clang-format off
log_level_t feature_levels[] = { // sorted alphabetically
    [UNKNOWN] = LOG_DEBUG,
    [ALLOC]   = LOG_WARN,
    [MAP]     = LOG_WARN,
    [LOGGER]  = LOG_WARN,
    [QP]      = LOG_WARN,
    [SCROLL]  = LOG_WARN,
    [SIPO]    = LOG_WARN,
    [SPLIT]   = LOG_WARN,
    [SPI]     = LOG_WARN,
    [TOUCH]   = LOG_WARN,
};
// clang-format on
ASSERT_FEATURES(feature_levels);

log_level_t get_level_for(feature_t feature) {
    return feature_levels[feature];
}

static inline void __logging_error(void) {
    logging(LOGGER, LOG_ERROR, "%s", __func__);
}

void set_level_for(feature_t feature, log_level_t level) {
    // clang-format off
    if (
        (feature < UNKNOWN) // is this possible ?
        || (level < LOG_NONE)
        || (feature >= __N_FEATURES__)
        || (level >= __N_LEVELS__)
    ) {
        // clang-format on
        return __logging_error();
    }

    feature_levels[feature] = level;
}

void step_level_for(feature_t feature, bool increase) {
    log_level_t level = get_level_for(feature);

    // clang-format off
    if (
        ((level == LOG_NONE) && !increase)
        || (((level + 1) == __N_LEVELS__) && increase)
    ) {
        // clang-format on
        return __logging_error();
    }

    if (increase) {
        level++;
    } else {
        level--;
    }

    set_level_for(feature, level);
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

        case 'F': // %F
            return F_SPEC;

        case 'M': // %M
            return M_SPEC;

        case 'T': // %T
            return T_SPEC;

        case '%': // %%
            return PERC_SPEC;

        default:
            return INVALID_SPEC;
    }

    __logging_error();
    return INVALID_SPEC;
}

static log_level_t msg_level = LOG_NONE; // level of the text being logged

log_level_t get_message_level(void) {
    return msg_level;
}

__attribute__((weak)) const char *log_time(void) {
    static char buff[10] = {0};
    snprintf(buff, sizeof(buff), "%ld", timer_read32() / 1000);
    return buff;
}

#if defined(ENABLE_LOGGING)
static bool wrap_printf = ENABLE_LOGGING;
#else
static bool wrap_printf = true;
#endif

static MUTEX_DECL(logging_mutex);

int logging(feature_t feature, log_level_t level, const char *msg, ...) {
    int exitcode = 0;

    bool has_acquired_lock = false;

    // message filtered out, quit
    log_level_t feat_level = feature_levels[feature];
    if (level < feat_level || feat_level == LOG_NONE) {
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
    msg_level = level;

    const char *format = LOGGING_FORMAT;
    while (true) {
        // order specs alphabetically, special cases first
        switch (get_token(&format)) {
            case INVALID_SPEC: // reached when the logging format is invalid
                wrap_printf = false;
                exitcode    = -EINVAL;
                goto exit;

            case STR_END:
                msg_level = LOG_NONE;
                goto exit;

            case NO_SPEC: // print any char
                putchar_(*format);
                break;

            case F_SPEC: // print feature name
                printf("%s", feature_str[feature]);
                break;

            case LL_SPEC: // print log level (long)
                printf("%s", level_str[level]);
                break;

            case LS_SPEC: // print log level (short)
                putchar_(level_str[level][0]);
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

    if (exitcode == -EINVAL) {
        logging(LOGGER, LOG_ERROR, "Invalid logging format");
    }

    return exitcode;
}

void print_str(const char *str, const sendchar_func_t func) {
    if (func == NULL) return __logging_error();

    for (size_t i = 0; i < strlen(str); ++i) {
        func(str[i]);
    }
}

void print_u8(const uint8_t val, const sendchar_func_t func) {
    if (func == NULL) return __logging_error();

    char buff[4];
    snprintf(buff, sizeof(buff), "%d", val);
    print_str(buff, func);
}

void print_u8_array(const uint8_t *list, const size_t len, const sendchar_func_t func) {
    if (func == NULL) return __logging_error();

    func('[');
    for (size_t i = 0; i < len - 1; ++i) {
        print_u8(list[i], func);
        print_str(", ", func);
    }
    print_u8(list[len - 1], func);
    func(']');
}
