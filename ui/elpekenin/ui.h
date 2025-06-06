// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Composable UI design.
 */

// -- barrier --

#pragma once

// #if !defined(QUANTUM_PAINTER_ENABLE)
// #    error Quantum painter must be enabled to use ui
// #endif

#include <errno.h>
#include <quantum/painter/qp.h>
#include <quantum/util.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint16_t ui_coord_t;

typedef enum {
    UI_SPLIT_MODE_NONE,
    UI_SPLIT_MODE_ABSOLUTE,
    UI_SPLIT_MODE_RELATIVE,
    UI_SPLIT_MODE_REMAINING,
    UI_SPLIT_MODE_FONT,
    UI_SPLIT_MODE_IMAGE,
} ui_split_mode_t;

typedef enum {
    UI_SPLIT_DIR_NONE,
    UI_SPLIT_DIR_HORIZONTAL,
    UI_SPLIT_DIR_VERTICAL,
} ui_split_direction_t;

typedef struct {
    ui_split_mode_t mode;
    ui_coord_t      size;
} ui_node_size_t;

typedef struct {
    struct _ui_node_t *ptr;
    size_t             n;
} ui_children_t;

typedef enum {
    UI_STATE_NONE,
    UI_STATE_OK,
    UI_STATE_ERR,
} ui_state_t;

typedef struct {
    ui_coord_t x;
    ui_coord_t y;
} ui_vector_t;

typedef struct _ui_node_t {
    // internals
    const ui_children_t        children;
    const ui_node_size_t       node_size;
    const ui_split_direction_t direction;
    ui_state_t                 state;

    // computed size
    ui_vector_t start;
    ui_vector_t size;

    // initialize
    bool (*const init)(struct _ui_node_t *);

    // rendering
    void *const args;
    void (*const render)(const struct _ui_node_t *, painter_device_t);
} ui_node_t;

//

#define UI_ABSOLUTE(x)                             \
    (ui_node_size_t) {                             \
        .mode = UI_SPLIT_MODE_ABSOLUTE, .size = x, \
    }

#define UI_RELATIVE(x)                             \
    (ui_node_size_t) {                             \
        .mode = UI_SPLIT_MODE_RELATIVE, .size = x, \
    }

/**
 * Size equal to font's height. Can only be used within vertical split node.
 *
 * .. warning::
 *    Accessed node->args as a font to load.
 */
#define UI_FONT()                   \
    (ui_node_size_t) {              \
        .mode = UI_SPLIT_MODE_FONT, \
    }

/**
 * Size equal to image's width/height (depending on parent's split direction).
 *
 * .. warning::
 *    Accessed node->args as an image to load.
 */
#define UI_IMAGE()                   \
    (ui_node_size_t) {               \
        .mode = UI_SPLIT_MODE_IMAGE, \
    }

#define UI_REMAINING()                   \
    (ui_node_size_t) {                   \
        .mode = UI_SPLIT_MODE_REMAINING, \
    }

//

#define UI_CHILDREN(x)                \
    (ui_children_t) {                 \
        .ptr = x, .n = ARRAY_SIZE(x), \
    }

//

bool ui_init(ui_node_t *root, ui_coord_t width, ui_coord_t height);
bool ui_render(const ui_node_t *parent, painter_device_t display);

// for debugging
void ui_print(const ui_node_t *root);
