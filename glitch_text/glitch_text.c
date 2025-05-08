// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/glitch_text.h"

#include <errno.h>
#include <quantum/quantum.h>
#include <string.h>
#include <sys/cdefs.h>

#if defined(COMMUNITY_MODULE_RNG_ENABLE)
#    include "elpekenin/rng.h"
#else
#    error Must enable 'elpekenin/rng'
#endif

#ifdef GLITCH_TEXT_DEBUG
#    include "quantum/logging/debug.h"
#    define glitch_text_dprintf dprintf
#else
#    define glitch_text_dprintf(...)
#endif

static deferred_executor_t glitch_text_executors[CONCURRENT_GLITCH_TEXTS] = {0};
static glitch_text_state_t glitch_text_states[CONCURRENT_GLITCH_TEXTS]    = {0};

static uint16_t gen_random_pos(uint16_t max, uint64_t *mask) {
    uint16_t pos = 0;

    do { // dont mess already-done char
        pos = rng_min_max(0, max);
    } while ((*mask & (1 << pos)));

    *mask |= (1 << pos);
    return pos;
}

static uint32_t glitch_text_callback(__unused uint32_t trigger_time, void *cb_arg) {
    glitch_text_state_t *state = (glitch_text_state_t *)cb_arg;

    // strings converged, draw and quit
    if (state->phase == DONE) {
        // free this slot for reuse
        state->phase = NOT_RUNNING;

        strlcpy(state->curr, state->dest, sizeof(state->curr));

        // keep terminator untouched
        memset(state->dest, ' ', sizeof(state->dest) - 1);

        state->callback(state->curr, true);
        return 0;
    }

    char chr = '\0';
    do { // dont want a terminator mid-string
        chr = rng_min_max('!', '~');
    } while (chr == '\0');

    // all bits that should be set are set, change state
    uint64_t full_mask = (1 << (state->len - 1)) - 1;
    if ((state->mask & full_mask) == full_mask) {
        state->mask = 0;
        switch (state->phase) {
            case FILLING:
                state->phase = COPYING;
                break;

            case COPYING:
                state->phase = DONE;
                break;

            case DONE:
                break;

            case NOT_RUNNING:
                __unreachable();
        }
    }

    // this is an index, -1 prevents out of bounds str[len]
    uint16_t pos = gen_random_pos(state->len - 1, &state->mask);

    switch (state->phase) {
        case FILLING:
            state->curr[pos] = chr;
            break;

        case COPYING:
            state->curr[pos] = state->dest[pos];
            break;

        case DONE:
            break;

        case NOT_RUNNING:
            __unreachable();
    }

    state->callback(state->curr, false);
    return 30;
}

int glitch_text_start(const char *text, callback_fn_t callback) {
    if (callback == NULL || text == NULL) {
        glitch_text_dprintf("[ERROR] %s: NULL pointer\n", __func__);
        return -EINVAL;
    }

    size_t len = strlen(text);

    glitch_text_state_t *glitch_state = NULL;

    if (len > sizeof(glitch_state->dest)) {
        glitch_text_dprintf("[ERROR] %s: text too long\n", __func__);
        return -EINVAL;
    }

    for (glitch_text_state_t *state = glitch_text_states; state < &glitch_text_states[CONCURRENT_GLITCH_TEXTS]; ++state) {
        if (state->phase == NOT_RUNNING) {
            break;
        }
    }

    if (glitch_state == NULL) {
        glitch_text_dprintf("[ERROR] %s: fail (no free slot)\n", __func__);
        return -ENOMEM;
    }

    // kick off the animation
    strlcpy(glitch_state->dest, text, sizeof(glitch_state->dest));
    glitch_state->phase    = FILLING;
    glitch_state->mask     = 0;
    glitch_state->len      = len;
    glitch_state->callback = callback;
    defer_exec(10, glitch_text_callback, glitch_state);

    return 0;
}

//
// QMK hooks
//

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

void housekeeping_task_glitch_text(void) {
    static uint32_t timer = 0;

    // drawing every 100ms sounds good enough for me (10 frames/second)
    // faster would likely not be readable
    if (timer_elapsed32(timer) < 100) {
        return;
    }

    deferred_exec_advanced_task(glitch_text_executors, CONCURRENT_GLITCH_TEXTS, &timer);
}
