// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/scrolling_text.h"

#include <errno.h>
#include <quantum/color.h>

#ifdef SCROLLING_TEXT_DEBUG
#    include "quantum/logging/debug.h"
#    define scrolling_text_dprintf dprintf
#else
#    define scrolling_text_dprintf(...)
#endif

static deferred_executor_t    scrolling_text_executors[CONCURRENT_SCROLLING_TEXTS] = {0};
static scrolling_text_state_t scrolling_text_states[CONCURRENT_SCROLLING_TEXTS]    = {0};

static int render_scrolling_text_state(scrolling_text_state_t *state) {
    scrolling_text_dprintf("[DEBUG] %s: entry (char #%d)\n", __func__, (int)state->char_number);

    // prepare string slice
    char *slice = alloca(state->n_chars + 1); // +1 for null terminator
    if (slice == NULL) {
        scrolling_text_dprintf("[ERROR] %s: could not allocate\n", __func__);
        return -ENOMEM;
    }
    memset(slice, 0, state->n_chars + 1);

    uint8_t len = strlen(state->str);
    for (int8_t i = 0; i < state->n_chars; ++i) {
        uint8_t index = (state->char_number + i);

        // wrap to string length (plus spaces)
        uint8_t wrapped = index % (len + state->spaces);

        // copy a char or add separator whitespaces
        if (wrapped < len) {
            slice[i] = state->str[wrapped];
        } else {
            slice[i] = ' ';
        }
    }

    int16_t width = qp_textwidth(state->font, (const char *)slice);
    // clear previous rendering if needed
    if (state->width > 0) {
        qp_rect(state->device, state->x, state->y, state->x + state->width, state->y + state->font->line_height, HSV_BLACK, true);
    }
    state->width = width;

    // draw it
    bool ret = qp_drawtext_recolor(state->device, state->x, state->y, state->font, (const char *)slice, state->fg.hsv888.h, state->fg.hsv888.s, state->fg.hsv888.v, state->bg.hsv888.h, state->bg.hsv888.s, state->bg.hsv888.v);

    // update position
    if (ret) {
        ++state->char_number;
        if (state->char_number == len + state->spaces) {
            state->char_number = 0;
        }
        scrolling_text_dprintf("[DEBUG] %s: updated\n", __func__);
    } else {
        scrolling_text_dprintf("[ERROR] %s: drawing failed\n", __func__);
    }

    return ret;
}

static uint32_t scrolling_text_callback(uint32_t trigger_time, void *cb_arg) {
    scrolling_text_state_t *state = (scrolling_text_state_t *)cb_arg;
    int                     ret   = render_scrolling_text_state(state);

    // Setting the device to NULL clears the scrolling slot
    if (ret != 0) {
        state->device = NULL;
        return 0;
    }

    return state->delay;
}

deferred_token draw_scrolling_text(painter_device_t device, uint16_t x, uint16_t y, painter_font_handle_t font, const char *str, uint8_t n_chars, uint32_t delay) {
    return draw_scrolling_text_recolor(device, x, y, font, str, n_chars, delay, 0, 0, 255, 0, 0, 0);
}

deferred_token draw_scrolling_text_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_handle_t font, const char *str, uint8_t n_chars, uint32_t delay, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    scrolling_text_dprintf("[DEBUG] %s: entry\n", __func__);

    scrolling_text_state_t *scrolling_state = NULL;
    for (scrolling_text_state_t *state = scrolling_text_states; state < &scrolling_text_states[CONCURRENT_SCROLLING_TEXTS]; ++state) {
        if (state->device == NULL) {
            scrolling_state = state;
            break;
        }
    }

    if (scrolling_state == NULL) {
        scrolling_text_dprintf("[ERROR] %s: fail (no free slot)\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    // make a copy of the string, to prevent issues if the original variable is removed
    // note: input is expected to end in null terminator
    uint8_t len          = strlen(str) + 1; // add one to also allocate/copy the terminator
    scrolling_state->str = malloc(len);
    if (scrolling_state->str == NULL) {
        scrolling_text_dprintf("[ERROR] %s: fail (allocation)\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    // note, len is the buffer size we allocated right above (strlen(str) + 1)
    strlcpy(scrolling_state->str, str, len);

    // Prepare the scrolling state
    scrolling_state->device      = device;
    scrolling_state->x           = x;
    scrolling_state->y           = y;
    scrolling_state->font        = font;
    scrolling_state->n_chars     = n_chars;
    scrolling_state->delay       = delay;
    scrolling_state->char_number = 0;
    scrolling_state->spaces      = 5; // TODO: Receive as argument?
    scrolling_state->fg          = (qp_pixel_t){.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
    scrolling_state->bg          = (qp_pixel_t){.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};

    // Draw the first string
    if (render_scrolling_text_state(scrolling_state) != 0) {
        scrolling_text_dprintf("[ERROR] %s: fail (render 1st step)\n", __func__);

        scrolling_state->device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    // Set up the timer
    scrolling_state->defer_token = defer_exec_advanced(scrolling_text_executors, CONCURRENT_SCROLLING_TEXTS, delay, scrolling_text_callback, scrolling_state);
    if (scrolling_state->defer_token == INVALID_DEFERRED_TOKEN) {
        scrolling_text_dprintf("[ERROR] %s: fail (setup executor)\n", __func__);

        scrolling_state->device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    scrolling_text_dprintf("[DEBUG] %s: ok (deferred token = %d)\n", __func__, (int)scrolling_state->defer_token);
    return scrolling_state->defer_token;
}

void extend_scrolling_text(deferred_token scrolling_token, const char *str) {
    for (scrolling_text_state_t *state = scrolling_text_states; state < &scrolling_text_states[CONCURRENT_SCROLLING_TEXTS]; ++state) {
        if (state->defer_token == scrolling_token) {
            uint8_t cur_len = strlen(state->str);
            uint8_t new_len = strlen(str);
            uint8_t len     = cur_len + new_len + 1;
            char   *new_pos = realloc(state->str, len);

            if (new_pos == NULL) {
                scrolling_text_dprintf("[ERROR] %s: fail (realloc)\n", __func__);
                return;
            }
            state->str = new_pos;

            strlcat(state->str, str, len);

            return;
        }
    }
}

void stop_scrolling_text(deferred_token scrolling_token) {
    if (scrolling_token == INVALID_DEFERRED_TOKEN) {
        return;
    }

    for (scrolling_text_state_t *state = scrolling_text_states; state < &scrolling_text_states[CONCURRENT_SCROLLING_TEXTS]; ++state) {
        if (state->defer_token == scrolling_token) {
            // Clear screen and de-allocate
            qp_rect(state->device, state->x, state->y, state->x + state->width, state->y + state->font->line_height, HSV_BLACK, true);

            free(state->str);

            // Cleanup the state
            state->device      = NULL;
            state->defer_token = INVALID_DEFERRED_TOKEN;

            cancel_deferred_exec_advanced(scrolling_text_executors, CONCURRENT_SCROLLING_TEXTS, scrolling_token);

            return;
        }
    }

    scrolling_text_dprintf("[ERROR] %s: Unknown scrolling token\n", __func__);
}

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

void housekeeping_task_scrolling_text(void) {
    static uint32_t timer = 0;

    // drawing every 100ms sounds good enough for me (10 frames/second)
    // faster would likely not be readable
    if (timer_elapsed32(timer) < 100) {
        return;
    }

    deferred_exec_advanced_task(scrolling_text_executors, CONCURRENT_SCROLLING_TEXTS, &timer);
}
