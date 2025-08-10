// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Composable design of GUIs over QP, based on a hierarchy of nodes.
 *
 * This system was designed for flexibility, because computing each element's position and size by hand was tedious and prone to errors.
 * It also wasn't dynamic, in the sense that if some element of the screen wasn't being drawn (eg: feature disabled), sizes wouldn't adapt and leave a gap.
 *
 * As a convenience, a handful of builtin integrations are provided, eg: uptime, QMK's version, current layer, ...
 * Check them out under ``modules/elpekenin/ui/elpekenin/ui`` folder. You can ``#include "elpekenin/ui/<some_file>.h"`` and use it on your keyboard.
 * On a similar fashion, some of my modules also implement this kind of integration when the UI module is enabled.
 */

// -- barrier --

#pragma once

#if !defined(QUANTUM_PAINTER_ENABLE)
#    error Quantum painter must be enabled to use ui
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "qp.h"
#include "util.h"

typedef uint16_t ui_coord_t;
#define UI_COORD_MAX ((ui_coord_t)~0)

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
    UI_SPLIT_DIR_LEFT_RIGHT,
    UI_SPLIT_DIR_RIGHT_LEFT,
    UI_SPLIT_DIR_TOP_BOTTOM,
    UI_SPLIT_DIR_BOTTOM_TOP,
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
    uint32_t    next_render;
    void *const args;
    uint32_t (*const render)(const struct _ui_node_t *, painter_device_t);
} ui_node_t;

/**
 * A node can have children, this is represented by:
 *    - ``.children = UI_CHILDREN(nodes)``: Where ``nodes`` is an array of ``ui_node_t``'s
 *
 * To compute the size of each child, parents must specify how their size will be shared between all children.
 *    - ``.split_direction = UI_SPLIT_DIR_{LEFT_RIGHT,RIGHT_LEFT}``: All children are as tall as parent, and size splits horizontally
 *    - ``.split_direction = UI_SPLIT_DIR_{TOP_BOTTOM,BOTTOM_TOP}``: All children are as wide as parent, and size splits vertically
 */
#define UI_CHILDREN(x)                \
    (ui_children_t) {                 \
        .ptr = x, .n = ARRAY_SIZE(x), \
    }

/**
 * Every node must define how big they want to be by setting ``.node_size`` to the following macros:
 */

/**
 * ``x`` pixels in size
 */
#define UI_ABSOLUTE(x)                             \
    (ui_node_size_t) {                             \
        .mode = UI_SPLIT_MODE_ABSOLUTE, .size = x, \
    }

/**
 * ``x`` % of parent's size
 */
#define UI_RELATIVE(x)                             \
    (ui_node_size_t) {                             \
        .mode = UI_SPLIT_MODE_RELATIVE, .size = x, \
    }

/**
 * `x` times the font's height. Can only be used within vertical split node.
 *
 * .. warning::
 *   Executes ``qp_load_font_mem(*(void**)node->args)`` to compute size.
 *   That is, the node's ``args`` **must** point to a structure whose first element
 *   is a font's array.
 */
#define UI_FONT(x)                             \
    (ui_node_size_t) {                         \
        .mode = UI_SPLIT_MODE_FONT, .size = x, \
    }

/**
 * `x` times a image's width/height (depending on parent's split direction).
 *
 * .. warning::
 *   Executes``qp_load_image_mem(*(void**)node->args)`` to compute size.
 *   That is, the node's ``args`` **must** point to a structure whose first element
 *   is an image's array.
 */
#define UI_IMAGE(x)                             \
    (ui_node_size_t) {                          \
        .mode = UI_SPLIT_MODE_IMAGE, .size = x, \
    }

/**
 * Claim the parent's remaining (not used by siblings) size.
 */
#define UI_REMAINING()                   \
    (ui_node_size_t) {                   \
        .mode = UI_SPLIT_MODE_REMAINING, \
    }

#define MILLISECONDS(x) ((uint32_t)x)
#define SECONDS(x) (x * MILLISECONDS(1000))
#define MINUTES(x) (x * SECONDS(60))
#define HOURS(x) (x * MINUTES(60))
#define DAYS(x) (x * HOURS(24))
#define UI_STOP ((uint32_t)~0)

/**
 * Once you've declared a node tree, use this function to compute all nodes' size/position.
 *
 * If the input can't be resolved (eg children don't fit into parent), function will flag the node as invalid, and return false.
 *
 * .. hint::
 *   If a node must run some validation (eg: its computed height >= font used), it can provide an ``.init`` function.
 *   Returning ``false`` from it means that requirements weren't met, causing tree's resolution to fail.
 */
bool ui_init(ui_node_t *root, ui_coord_t width, ui_coord_t height);

/**
 * You shall use this function to render all nodes.
 *
 * Each node describes how it's rendered by providing a ``.render`` function.
 * If it needs to track some state, it may use the ``.args`` field to store a pointer into whichever structure.
 * Return value is the time (in ms) before calling it again. Return ``UI_STOP`` as a flag for "do not repeat"
 *
 * You want run this function periodically (ie: from ``housekeeping_task_user``).
 *
 * .. warning::
 *   There isn't any kind of limitation to the area in which each node can draw.
 *   This means that nodes are expected to respect the boundaries provided.
 *
 *   For example
 *
 *   .. code-block:: c
 *
 *       if (!ui_text_fits(font, text)) {
 *           qp_close_font(font);
 *           return SECONDS(1);
 *       }
 *
 */
bool ui_render(ui_node_t *root, painter_device_t display);

// for debugging
void ui_print(const ui_node_t *root);
