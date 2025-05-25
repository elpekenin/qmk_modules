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

typedef struct {
    /**
     * How to draw the text
     */
    scrolling_text_config_t config;

    /**
     * Pixel width of current text, used to clean before next draw
     */
    int16_t width;

    /**
     * Position (offset) of first char to be drawn next iteration
     */
    uint8_t char_number;
} scrolling_text_state_t;

struct {
    /**
     * defer_exec configuration
     */
    deferred_executor_t executors[CONFIG_SCROLLING_TEXT_N_WORKERS];

    /**
     * How to draw each text
     */
    scrolling_text_state_t states[CONFIG_SCROLLING_TEXT_N_WORKERS];

    /**
     * Identifier of the task drawing each text.
     */
    deferred_token tokens[CONFIG_SCROLLING_TEXT_N_WORKERS];
} global_state = {0};

static int render_scrolling_text_state(scrolling_text_state_t *state) {
    scrolling_text_dprintf("[DEBUG] %s: entry (char #%d)\n", __func__, (int)state->char_number);

    const scrolling_text_config_t config = state->config;

    // prepare string slice
    char *slice = alloca(config.n_chars + 1); // +1 for null terminator
    if (slice == NULL) {
        scrolling_text_dprintf("[ERROR] %s: could not allocate\n", __func__);
        return -ENOMEM;
    }
    memset(slice, 0, config.n_chars + 1);

    uint8_t len = strlen(config.str);
    for (uint8_t i = 0; i < config.n_chars; ++i) {
        uint8_t index = (state->char_number + i);

        // wrap to string length (plus spaces)
        uint8_t wrapped = index % (len + config.spaces);

        // copy a char or add separator whitespaces
        if (wrapped < len) {
            slice[i] = config.str[wrapped];
        } else {
            slice[i] = ' ';
        }
    }

    int16_t width = qp_textwidth(config.font, (const char *)slice);
    // clear previous rendering if needed
    if (state->width > 0) {
        qp_rect(config.device, config.x, config.y, config.x + state->width, config.y + config.font->line_height, HSV_BLACK, true);
    }
    state->width = width;

    // draw it
    bool ret = qp_drawtext_recolor(config.device, config.x, config.y, config.font, (const char *)slice, config.fg.hsv888.h, config.fg.hsv888.s, config.fg.hsv888.v, config.bg.hsv888.h, config.bg.hsv888.s, config.bg.hsv888.v);

    // update position
    if (!ret) {
        scrolling_text_dprintf("[ERROR] %s: drawing failed\n", __func__);
        return -EIO;
    }

    state->char_number += 1;
    if (state->char_number >= (len + config.spaces)) {
        state->char_number = 0;
    }
    scrolling_text_dprintf("[DEBUG] %s: updated\n", __func__);

    return 0;
}

static uint32_t scrolling_text_callback(__unused uint32_t trigger_time, void *cb_arg) {
    scrolling_text_state_t *state = (scrolling_text_state_t *)cb_arg;

    int ret = render_scrolling_text_state(state);
    if (ret != 0) {
        // setting the device to NULL clears the scrolling slot
        state->config.device = NULL;
        return 0;
    }

    return state->config.delay;
}

deferred_token scrolling_text_start(const scrolling_text_config_t *config) {
    scrolling_text_dprintf("[DEBUG] %s: entry\n", __func__);

    size_t index = SIZE_MAX;
    for (size_t i = 0; i < CONFIG_SCROLLING_TEXT_N_WORKERS; ++i) {
        if (global_state.states[i].config.device == NULL) {
            index = i;
            break;
        }
    }

    if (index == SIZE_MAX) {
        scrolling_text_dprintf("[ERROR] %s: fail (no free slot)\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    scrolling_text_state_t *slot = &global_state.states[index];

    // make a copy of the string, to prevent issues if the original variable is removed
    // note: input is expected to end in null terminator
    uint8_t len = strlen(config->str) + 1; // add one to also allocate/copy the terminator

    void *ptr = malloc(len);
    if (ptr == NULL) {
        scrolling_text_dprintf("[ERROR] %s: fail (allocation)\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }
    // note: len is the buffer size we allocated right above (strlen(str) + 1)
    strlcpy(ptr, config->str, len);

    // prepare the scrolling state
    slot->config      = *config;
    slot->config.str  = ptr;
    slot->width       = 0;
    slot->char_number = 0;

    // Draw the first string
    if (render_scrolling_text_state(slot) != 0) {
        scrolling_text_dprintf("[ERROR] %s: fail (render 1st step)\n", __func__);
        slot->config.device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    // Set up the timer
    deferred_token token = defer_exec_advanced(global_state.executors, CONFIG_SCROLLING_TEXT_N_WORKERS, config->delay, scrolling_text_callback, slot);
    if (token == INVALID_DEFERRED_TOKEN) {
        scrolling_text_dprintf("[ERROR] %s: fail (setup executor)\n", __func__);
        slot->config.device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    global_state.tokens[index] = token;

    scrolling_text_dprintf("[DEBUG] %s: ok (deferred token = %d)\n", __func__, (int)token);
    return token;
}

void scrolling_text_extend(deferred_token scrolling_token, const char *str) {
    for (size_t i = 0; i < CONFIG_SCROLLING_TEXT_N_WORKERS; ++i) {
        if (global_state.tokens[i] == scrolling_token) {
            scrolling_text_state_t *state = &global_state.states[i];

            uint8_t cur_len = strlen(state->config.str);
            uint8_t add_len = strlen(str);
            uint8_t new_len = cur_len + add_len + 1;
            char   *new_ptr = realloc(state->config.str, new_len);

            if (new_ptr == NULL) {
                scrolling_text_dprintf("[ERROR] %s: fail (realloc)\n", __func__);
                return;
            }
            state->config.str = new_ptr;

            strlcat(new_ptr, str, new_len);

            return;
        }
    }
}

void scrolling_text_stop(deferred_token scrolling_token) {
    if (scrolling_token == INVALID_DEFERRED_TOKEN) {
        return;
    }

    for (size_t i = 0; i < CONFIG_SCROLLING_TEXT_N_WORKERS; ++i) {
        if (global_state.tokens[i] == scrolling_token) {
            // clear screen and de-allocate
            scrolling_text_state_t  *state  = &global_state.states[i];
            scrolling_text_config_t *config = &state->config;

            qp_rect(config->device, config->x, config->y, config->x + state->width, config->y + config->font->line_height, HSV_BLACK, true);

            free(config->str);

            // Cleanup the state
            config->device         = NULL;
            global_state.tokens[i] = INVALID_DEFERRED_TOKEN;

            cancel_deferred_exec_advanced(global_state.executors, CONFIG_SCROLLING_TEXT_N_WORKERS, scrolling_token);

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
    if (timer_elapsed32(timer) >= 100) {
        deferred_exec_advanced_task(global_state.executors, CONFIG_SCROLLING_TEXT_N_WORKERS, &timer);
    }

    housekeeping_task_scrolling_text_kb();
}
