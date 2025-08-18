// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/keylog.h"

#include <ctype.h>
#include <string.h>
#include <tmk_core/protocol/host.h> // keyboard_led_state

#include "compiler_support.h"
#include "quantum.h"
#include "util.h"

STATIC_ASSERT(CM_ENABLED(STRING), "Must enable 'elpekenin/string'");
#include "elpekenin/string.h" // is_utf8_continuation

STATIC_ASSERT(CM_ENABLED(GENERICS), "Must enable 'elpekenin/generics'");
#include "elpekenin/generics.h"

static char keylog[KEYLOG_SIZE + 1] = {
    [0 ... KEYLOG_SIZE - 1] = ' ',
    [KEYLOG_SIZE]           = '\0',
}; // extra space for terminator

// if keylog's type is ever changed, memmove needs update
STATIC_ASSERT(sizeof(keylog) == KEYLOG_SIZE + 1, "memmove will use wrong size");

typedef enum {
    NO_MODS,
    SHIFT,
    AL_GR,
    // ... implement more when needed
    N_MODS,
} active_mods_t;

typedef struct PACKED {
    const char *raw;
    const char *strings[N_MODS];
} replacements_t;

#define replacement(r, no_mods, shift, al_gr) \
    (replacements_t) {                        \
        .raw     = (r),                       \
        .strings = {                          \
            [NO_MODS] = (no_mods),            \
            [SHIFT]   = (shift),              \
            [AL_GR]   = (al_gr),              \
        },                                    \
    }

// TODO: introspection
// clang-format off
static const replacements_t replacements[] = {
    replacement("0",       NULL,  "=",  NULL),
    replacement("1",       NULL,  "!",  "|" ),
    replacement("2",       NULL,  "\"", "@" ),
    replacement("3",       NULL,  NULL, "#" ), // · breaks keylog
    replacement("4",       NULL,  "$",  "~" ),
    replacement("5",       NULL,  "%",  NULL),
    replacement("6",       NULL,  "&",  NULL),
    replacement("7",       NULL,  "/",  NULL),
    replacement("8",       NULL,  "(",  NULL),
    replacement("9",       NULL,  ")",  NULL),
    replacement("_______", "__",  NULL, NULL),
    replacement("AT",      "@",   NULL, NULL),
    replacement("BSLS",    "\\",  NULL, NULL),
    replacement("CAPS",    "↕",   NULL, NULL),
    replacement("COMM",    ",",   ";",  NULL),
    replacement("DB_TOGG", "DBG", NULL, NULL),
    replacement("DOT",     ".",   ":",  NULL),
    replacement("DOWN",    "↓",   NULL, NULL),
    replacement("ENT",     "↲",   NULL, NULL),
    replacement("GRV",     "`",   "^",  NULL),
    replacement("HASH",    "#",   NULL, NULL),
    replacement("LBRC",    "[",   NULL, NULL),
    replacement("LCBR",    "{",   NULL, NULL),
    replacement("LEFT",    "←",   NULL, NULL),
    replacement("LOWR",    "▼",   NULL, NULL),
    replacement("MINS",    "-",   "_",  NULL),
    replacement("NTIL",    "´",   NULL, NULL),
    replacement("R_SPC",   " ",   NULL, NULL),
    replacement("RBRC",    "]",   NULL, NULL),
    replacement("RCBR",    "}",   NULL, NULL),
    replacement("RIGHT",   "→",   NULL, NULL),
    replacement("PLUS",    "+",   "*",  NULL),
    replacement("PIPE",    "|",   NULL, NULL),
    replacement("QUOT",    "'",   "?",  NULL),
    replacement("SPC",     " ",   NULL, NULL),
    replacement("TAB",     "⇥",   NULL, NULL),
    replacement("TILD",    "~",   NULL, NULL),
    replacement("UP",      "↑",   NULL, NULL),
    replacement("UPPR",    "▲",   NULL, NULL),
    replacement("VOLU",    "♪",   "♪",  NULL),
    replacement("R_SPC",   " ",   NULL, NULL),
};
// clang-format on

static const char *prefixes[] = {"KC_", "RGB_", "QK_", "ES_", "TD_", "TL_"};

static void skip_prefix(const char **str) {
    for (size_t i = 0; i < ARRAY_SIZE(prefixes); ++i) {
        const char *const prefix = prefixes[i];

        const size_t len = strlen(prefix);
        if (strncmp(*str, prefix, len) == 0) {
            *str += len;
            return;
        }
    }
}

OptionImpl(replacements_t);

static Option(replacements_t) find_replacement(const char *str) {
    for (size_t i = 0; i < ARRAY_SIZE(replacements); ++i) {
        const replacements_t replacement = replacements[i];

        if (strcmp(replacement.raw, str) == 0) {
            return Some(replacements_t, replacement);
        }
    }

    return None(replacements_t);
}

static void maybe_symbol(const char **str) {
    const Option(replacements_t) maybe_replacement = find_replacement(*str);
    if (!maybe_replacement.is_some) {
        return;
    }

    replacements_t replacement = unwrap(maybe_replacement);

    const char *target = NULL;
    switch (get_mods()) {
        case 0:
            target = replacement.strings[NO_MODS];
            break;

        case MOD_BIT_LSHIFT:
        case MOD_BIT_RSHIFT:
            target = replacement.strings[SHIFT];
            break;

        case MOD_BIT_RALT:
            target = replacement.strings[AL_GR];
            break;

        default:
            // nothing to be done here
            return;
    }

    // we may get here with a combination with no replacement, eg shift+arrows
    // dont want to assign str to NULL
    if (target != NULL) {
        *str = target;
    }
}

// convert to lowercase based on shift/caps
// overengineered so it can also work on strings and whatnot on future
static void apply_casing(const char **str) {
    // not a single char
    if (strlen(*str) > 1) {
        return;
    }

    // not a letter
    if (!isalpha((unsigned char)**str)) {
        return;
    }

    uint8_t mods  = get_mods();
    bool    shift = mods & MOD_MASK_SHIFT;
    bool    caps  = host_keyboard_led_state().caps_lock;

    // if writing uppercase, string already is, just return
    if (shift ^ caps) {
        return;
    }

    char *lowercase_letters[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

    *str = lowercase_letters[**str - 'A'];
}

static void keylog_clear(void) {
    // spaces (not 0) so `qp_drawtext` actually renders something
    memset(keylog, ' ', KEYLOG_SIZE);
    keylog[KEYLOG_SIZE] = '\0';
}

static void keylog_shift_right_one_byte(void) {
    memmove(keylog + 1, keylog, KEYLOG_SIZE - 1);
    keylog[0] = ' ';
}

static void keylog_shift_right(void) {
    // pop all utf-continuation bytes
    while (is_utf8_continuation(keylog[KEYLOG_SIZE - 1])) {
        keylog_shift_right_one_byte();
    }

    // this is either an ascii char or the heading byte of utf
    keylog_shift_right_one_byte();
}

static void keylog_shift_left(size_t len) {
    memmove(keylog, keylog + len, KEYLOG_SIZE - len);

    size_t counter = 0;
    while (is_utf8_continuation(keylog[0])) {
        memmove(keylog, keylog + 1, KEYLOG_SIZE - 1);
        ++counter;
    }

    // pad buffer to the right, to align after a utf8 symbol is deleted
    memmove(keylog + counter, keylog, KEYLOG_SIZE - counter);
    memset(keylog, ' ', counter);
}

static void keylog_append(const char *str) {
    size_t len = strlen(str);

    keylog_shift_left(len);
    for (size_t i = 0; i < len; ++i) {
        keylog[KEYLOG_SIZE - len + i] = str[i];
    }
}

const char *get_keylog(void) {
    return keylog;
}

void keycode_repr(const char **str) {
    skip_prefix(str);
    maybe_symbol(str);
}

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui/utils.h"

bool keylog_init(ui_node_t *self) {
    return ui_font_fits(self);
}

ui_time_t keylog_render(const ui_node_t *self, painter_device_t display) {
    keylog_args_t *args = self->args;

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    const char *str = get_keylog();

    // trim heading chars until it fits
    uint16_t width = ~0;
    for (size_t i = 0; i < KEYLOG_SIZE; ++i) {
        width = qp_textwidth(font, str);
        if (width == 0) {
            goto err;
        }

        if (width <= self->size.x) {
            break;
        }

        str++;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, str);

err:
    qp_close_font(font);

exit:
    return args->interval;
}
#endif

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(0, 1, 0);

bool process_record_keylog(uint16_t keycode, keyrecord_t *record) {
    // prevent keylog processing, but not the keycode's logic
    if (!process_record_keylog_kb(keycode, record)) {
        return true;
    }

    // nothing on release (for now)
    if (!record->event.pressed) {
        return true;
    }

    // dont want to show some keycodes
    // clang-format off
    if ((IS_QK_LAYER_TAP(keycode) && !record->tap.count)
        || keycode >= QK_USER  // dont want my custom keycodes on keylog
        || IS_RGB_KEYCODE(keycode)
        || IS_QK_LAYER_MOD(keycode)
        || IS_QK_MOMENTARY(keycode)
        || IS_QK_DEF_LAYER(keycode)
        || IS_MODIFIER_KEYCODE(keycode)
       )
    {
        // clang-format on
        return true;
    }

    const char *str = get_keycode_string(keycode);

    // skip keycodes that fallback to 0x...
    const char *const prefix = "0x";
    if (strncmp(str, prefix, strlen(prefix)) == 0) {
        return true;
    }

    uint8_t mods = get_mods();
    bool    ctrl = mods & MOD_MASK_CTRL;

    // delete from tail
    if (strstr(str, "BSPC") != NULL) {
        // ctrl + backspace clears whole log
        if (ctrl) {
            keylog_clear();
        } else {
            // backspace = remove last char
            keylog_shift_right();
        }
        return true;
    }

    // convert string into symbols
    keycode_repr(&str);

    // casing is separate so that drawing keycodes on screen is always uppercase
    apply_casing(&str);

    keylog_append(str);

    return true;
}
