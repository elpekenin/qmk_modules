// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/glitch_text.h"

#include <errno.h>
#include <quantum/quantum.h>
#include <string.h>
#include <sys/cdefs.h>

#if defined(COMMUNITY_MODULE_GENERICS_ENABLE)
#    include "elpekenin/generics.h"
#else
#    error Must enable 'elpekenin/generics'
#endif

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

#define MAX_TEXT_SIZE 64

/**
 * State of a glitch text.
 */
typedef enum {
    /** */
    NOT_RUNNING,
    /** */
    FILLING,
    /** */
    COPYING,
    /** */
    DONE,
} anim_phase_t;

/**
 * Internal information about a glitch text.
 */
typedef struct {
    /**
     * User configuration.
     */
    glitch_text_config_t config;

    /**
     * Current animation phase.
     */
    anim_phase_t phase;

    /**
     * Target text: what to draw after animation is complete.
     */
    char *dest;

    /**
     * Text to display at the moment.
     */
    char *curr;

    /**
     * Bitmask used internally to control chars to change.
     */
    uint64_t mask;

    /**
     * Length of the string.
     */
    size_t len;
} glitch_text_state_t;

static struct {
    /**
     * defer_exec configuration
     */
    deferred_executor_t executors[GLITCH_TEXT_N_WORKERS];

    /**
     * How to draw each text
     */
    glitch_text_state_t states[GLITCH_TEXT_N_WORKERS];
} glitch_text = {0};

//
// Allocation routines
//

static void glitch_text_free(glitch_text_config_t config, void *ptr) {
#if !GLITCH_TEXT_USE_ALLOCATOR
    free(ptr);
#else
    free_with(config.allocator, ptr);
#endif
}

static void *glitch_text_malloc(glitch_text_config_t config, size_t size) {
#if !GLITCH_TEXT_USE_ALLOCATOR
    return malloc(size);
#else
    return malloc_with(config.allocator, size);
#endif
}

//
// Utils
//

static void clear(glitch_text_state_t *state) {
    state->phase = NOT_RUNNING;
    glitch_text_free(state->config, state->dest);
    glitch_text_free(state->config, state->curr);
    state->dest = state->curr = NULL;
}

//
// Rendering
//

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
        state->config.callback(state->dest, true);
        clear(state);
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

    state->config.callback(state->curr, false);

    return state->config.delay;
}

static bool free_slot(glitch_text_state_t state) {
    return state.phase == NOT_RUNNING;
}

//
// Public API
//

int glitch_text_start(const glitch_text_config_t *config, const char *text) {
    if (config == NULL || config->allocator == NULL || text == NULL || config->callback == NULL) {
        glitch_text_dprintf("[ERROR] %s: NULL pointer\n", __func__);
        return -EINVAL;
    }

    const size_t len = strlen(text) + 1; // also terminator
    if (len > MAX_TEXT_SIZE) {
        glitch_text_dprintf("[ERROR] %s: text too long\n", __func__);
        return -EINVAL;
    }

    glitch_text_state_t *slot = find_array(glitch_text.states, free_slot);
    if (slot == NULL) {
        glitch_text_dprintf("[ERROR] %s: no free slot\n", __func__);
        return -ENOMEM;
    }

    char *dest = glitch_text_malloc(*config, len);
    if (dest == NULL) {
        glitch_text_dprintf("[ERROR] %s: couldn't allocate\n", __func__);
        return -ENOMEM;
    }

    char *curr = glitch_text_malloc(*config, len);
    if (curr == NULL) {
        glitch_text_free(*config, dest);
        glitch_text_dprintf("[ERROR] %s: couldn't allocate\n", __func__);
        return -ENOMEM;
    }

    // fill up new pointers
    memcpy(dest, text, len);
    memset(curr, ' ', len);
    curr[len] = '\0';

    // prepare state
    slot->config = *config;
    slot->dest   = dest;
    slot->curr   = curr;
    slot->phase  = FILLING;
    slot->mask   = 0;
    slot->len    = len;

    // kick off animation
    deferred_token token = defer_exec_advanced(glitch_text.executors, GLITCH_TEXT_N_WORKERS, config->delay, glitch_text_callback, slot);
    if (token == INVALID_DEFERRED_TOKEN) {
        glitch_text_dprintf("[ERROR] %s: couldn't setup executor\n", __func__);
        clear(slot);
        return -EAGAIN;
    }

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
    if (timer_elapsed32(timer) >= GLITCH_TEXT_TASK_INTERVAL) {
        deferred_exec_advanced_task(glitch_text.executors, GLITCH_TEXT_N_WORKERS, &timer);
    }

    housekeeping_task_glitch_text_kb();
}
