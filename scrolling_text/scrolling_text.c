// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/scrolling_text.h"

#include "color.h"

#if defined(COMMUNITY_MODULE_GENERICS_ENABLE)
#    include "elpekenin/generics.h"
#else
#    error Must enable 'elpekenin/generics'
#endif

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
     * Copy of input text
     */
    char *str;

    /**
     * Pixel width of current text, used to clean before next draw
     */
    int16_t width;

    /**
     * Position (offset) of first char to be drawn next iteration
     */
    size_t char_number;
} scrolling_text_state_t;

static struct {
    /**
     * defer_exec configuration
     */
    deferred_executor_t executors[SCROLLING_TEXT_N_WORKERS];

    /**
     * How to draw each text
     */
    scrolling_text_state_t states[SCROLLING_TEXT_N_WORKERS];

    /**
     * Identifier of the task drawing each text.
     */
    deferred_token tokens[SCROLLING_TEXT_N_WORKERS];
} scrolling_text = {0};

//
// Allocation routines
//

static void scrolling_text_free(scrolling_text_config_t config, void *ptr) {
#if !SCROLLING_TEXT_USE_ALLOCATOR
    free(ptr);
#else
    free_with(config.allocator, ptr);
#endif
}

static void *scrolling_text_malloc(scrolling_text_config_t config, size_t size) {
#if !SCROLLING_TEXT_USE_ALLOCATOR
    return malloc(size);
#else
    return malloc_with(config.allocator, size);
#endif
}

static void *scrolling_text_realloc(scrolling_text_config_t config, void *ptr, size_t size) {
#if !SCROLLING_TEXT_USE_ALLOCATOR
    return realloc(ptr, size);
#else
    return realloc_with(config.allocator, ptr, size);
#endif
}

//
// Utils
//

static bool free_slot(scrolling_text_state_t state) {
    return state.config.device == NULL;
}

static void clear(scrolling_text_state_t *state) {
    const size_t index = state - scrolling_text.states;
    cancel_deferred_exec_advanced(scrolling_text.executors, SCROLLING_TEXT_N_WORKERS, scrolling_text.tokens[index]);

    // remove text
    qp_rect(state->config.device, state->config.x, state->config.y, state->config.x + state->width, state->config.y + state->config.font->line_height, HSV_BLACK, true);

    scrolling_text_free(state->config, state->str);
    state->config.device = NULL;
}

//
// Rendering
//

__warn_unused static int render_scrolling_text_state(scrolling_text_state_t *state) {
    scrolling_text_dprintf("[DEBUG] %s: entry (char #%d)\n", __func__, (int)state->char_number);

    const scrolling_text_config_t config = state->config;

    // prepare string slice
    char *slice = alloca(config.n_chars + 1); // +1 for null terminator
    if (slice == NULL) {
        scrolling_text_dprintf("[ERROR] %s: could not allocate\n", __func__);
        return -ENOMEM;
    }
    memset(slice, 0, config.n_chars + 1);

    size_t len = strlen(state->str);
    for (size_t i = 0; i < config.n_chars; ++i) {
        size_t index = (state->char_number + i);

        // wrap to string length (plus spaces)
        size_t wrapped = index % (len + config.spaces);

        // copy a char or add separator whitespaces
        if (wrapped < len) {
            slice[i] = state->str[wrapped];
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
    bool ret = qp_drawtext_recolor(config.device, config.x, config.y, config.font, (const char *)slice, config.fg.h, config.fg.s, config.fg.v, config.bg.h, config.bg.s, config.bg.v);

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

    if (render_scrolling_text_state(state) != 0) {
        clear(state);
        return 0;
    }

    return state->config.delay;
}

//
// Public API
//

deferred_token scrolling_text_start(const scrolling_text_config_t *config, const char *text) {
    scrolling_text_dprintf("[DEBUG] %s: entry\n", __func__);

    if (text == NULL) {
        scrolling_text_dprintf("[ERROR] %s: NULL str\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    scrolling_text_state_t *slot = find_array(scrolling_text.states, free_slot);
    if (slot == NULL) {
        scrolling_text_dprintf("[ERROR] %s: no free slot\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    // make a copy of the string, to prevent issues if the original variable is removed
    // note: input is expected to end in null terminator
    size_t len = strlen(text) + 1; // add one to also allocate/copy the terminator

    void *ptr = scrolling_text_malloc(*config, len);
    if (ptr == NULL) {
        scrolling_text_dprintf("[ERROR] %s: couldn't allocate\n", __func__);
        return INVALID_DEFERRED_TOKEN;
    }

    // note: len is the buffer size we allocated right above (strlen(str) + 1)
    memcpy(ptr, text, len);

    // prepare the scrolling state
    slot->config      = *config;
    slot->str         = ptr;
    slot->width       = 0;
    slot->char_number = 0;

    // Draw the first string
    if (render_scrolling_text_state(slot) != 0) {
        scrolling_text_dprintf("[ERROR] %s: couldn't render 1st step\n", __func__);
        slot->config.device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    // Set up the timer
    deferred_token token = defer_exec_advanced(scrolling_text.executors, SCROLLING_TEXT_N_WORKERS, config->delay, scrolling_text_callback, slot);
    if (token == INVALID_DEFERRED_TOKEN) {
        scrolling_text_dprintf("[ERROR] %s: couldn't setup executor\n", __func__);
        slot->config.device = NULL; // disregard the allocated scrolling slot
        return INVALID_DEFERRED_TOKEN;
    }

    const size_t index           = slot - scrolling_text.states;
    scrolling_text.tokens[index] = token;

    scrolling_text_dprintf("[DEBUG] %s: deferred token = %d\n", __func__, (int)token);
    return token;
}

void scrolling_text_extend(deferred_token scrolling_token, const char *str) {
    if (scrolling_token == INVALID_DEFERRED_TOKEN) {
        return;
    }

    bool filter(deferred_token token) {
        return token == scrolling_token;
    }

    deferred_token *token = find_array(scrolling_text.tokens, filter);
    if (token == NULL) {
        scrolling_text_dprintf("[ERROR] %s: did't find token=%d\n", __func__, scrolling_token);
    }

    const size_t                  index = token - scrolling_text.tokens;
    scrolling_text_state_t *const state = &scrolling_text.states[index];

    size_t cur_len = strlen(state->str);
    size_t add_len = strlen(str);
    size_t new_len = cur_len + add_len + 1;

    char *new_ptr = scrolling_text_realloc(state->config, state->str, new_len);
    if (new_ptr == NULL) {
        scrolling_text_dprintf("[ERROR] %s: couldn't realloc\n", __func__);
        return;
    }

    strlcat(new_ptr, str, new_len);
    state->str = new_ptr;
}

void scrolling_text_stop(deferred_token scrolling_token) {
    if (scrolling_token == INVALID_DEFERRED_TOKEN) {
        return;
    }

    bool filter(deferred_token token) {
        return token == scrolling_token;
    }

    deferred_token *token = find_array(scrolling_text.tokens, filter);
    if (token == NULL) {
        scrolling_text_dprintf("[ERROR] %s: did't find token=%d\n", __func__, scrolling_token);
    }

    const size_t index = token - scrolling_text.tokens;

    // clear screen and de-allocate
    scrolling_text_state_t *state = &scrolling_text.states[index];
    clear(state);
}

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

void housekeeping_task_scrolling_text(void) {
    static uint32_t timer = 0;

    // drawing every 100ms sounds good enough for me (10 frames/second)
    // faster would likely not be readable
    if (timer_elapsed32(timer) >= SCROLLING_TEXT_TASK_INTERVAL) {
        deferred_exec_advanced_task(scrolling_text.executors, SCROLLING_TEXT_N_WORKERS, &timer);
    }

    housekeeping_task_scrolling_text_kb();
}
