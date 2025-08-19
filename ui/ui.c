// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/ui.h"

#ifdef UI_DEBUG
#    include "quantum/logging/debug.h"
#    define ui_dprintf dprintf
#else
#    define ui_dprintf(...)
#endif

static inline bool ui_vector_eq(ui_vector_t lhs, ui_vector_t rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

static void ui_print_node(const ui_node_t *node, size_t indent) {
    if (node == NULL) {
        ui_dprintf("[ERROR] printing NULL\n");
        return;
    }

    ui_dprintf("%*s", indent, "");
    ui_dprintf("start: (%d, %d), size: (%d, %d)\n", node->start.x, node->start.y, node->size.x, node->size.y);

    for (size_t i = 0; i < node->children.n; ++i) {
        const ui_node_t *child = node->children.ptr + i;
        ui_print_node(child, indent + 2);
    }
}

static ui_coord_t ui_handle_font(const painter_font_handle_t font, const ui_node_t *parent, ui_coord_t scale) {
    if (parent->direction != UI_SPLIT_DIR_TOP_BOTTOM && parent->direction != UI_SPLIT_DIR_BOTTOM_TOP) {
        ui_dprintf("[ERROR] font size must be used on vertical split\n");
        return UI_COORD_MAX;
    }

    return scale * font->line_height;
}

static ui_coord_t ui_handle_image(const painter_image_handle_t image, const ui_node_t *parent, ui_coord_t scale) {
    switch (parent->direction) {
        default:
            return UI_COORD_MAX; // unreachable

        case UI_SPLIT_DIR_LEFT_RIGHT:
        case UI_SPLIT_DIR_RIGHT_LEFT:
            return scale * image->width;

        case UI_SPLIT_DIR_TOP_BOTTOM:
        case UI_SPLIT_DIR_BOTTOM_TOP:
            return scale * image->height;
    }
}

static bool ui_init_node(ui_node_t *parent) {
    // node already resolved
    switch (parent->state) {
        default:
            ui_dprintf("[ERROR] invalid value for state (%d)\n", parent->state);
            goto err;

        case UI_STATE_NONE:
            break;

        case UI_STATE_OK:
        case UI_STATE_ERR:
            ui_dprintf("[WARN] called init twice for same node\n");
            return parent->state == UI_STATE_OK;
    }

    // leaf node
    if (parent->children.n == 0) {
        // to compute size, we use:
        //   - parent.direction
        //   - child.node_size.mode
        //   - child.node_size.*
        // if we encounter dir on a leaf, error
        if (parent->direction != UI_SPLIT_DIR_NONE) {
            ui_dprintf("[ERROR] leaf node must not have split direction\n");
            goto err;
        }

        if (parent->render == NULL) {
            ui_dprintf("[DEBUG] leaf node without a render function\n");
        }

        goto ok;
    }

    if (parent->children.ptr == NULL) {
        ui_dprintf("[ERROR] parent node's children.ptr is NULL\n");
        goto err;
    }

    if (parent->render != NULL) {
        ui_dprintf("[ERROR] parent node should must not have a render function\n");
        goto err;
    }

    ui_coord_t parent_size = 0;
    switch (parent->direction) {
        default:
            ui_dprintf("[ERROR] invalid value for split (%d)\n", parent->direction);
            goto err;

        case UI_SPLIT_DIR_LEFT_RIGHT:
        case UI_SPLIT_DIR_RIGHT_LEFT:
            parent_size = parent->size.x;
            break;

        case UI_SPLIT_DIR_TOP_BOTTOM:
        case UI_SPLIT_DIR_BOTTOM_TOP:
            parent_size = parent->size.y;
            break;
    }

    ui_coord_t offset = 0;
    for (size_t i = 0; i < parent->children.n; ++i) {
        ui_node_t *child = parent->children.ptr + i;
        if (child == NULL) {
            ui_dprintf("[ERROR] child is NULL\n");
            goto err;
        }

        // compute child size
        ui_coord_t child_size = ~0;
        switch (child->node_size.mode) {
            default:
                ui_dprintf("[ERROR] invalid value for mode (%d)\n", child->node_size.mode);
                goto err;

            case UI_SPLIT_MODE_ABSOLUTE:
                child_size = child->node_size.size;
                break;

            case UI_SPLIT_MODE_RELATIVE:
                child_size = (parent_size * child->node_size.size) / 100;
                break;

            case UI_SPLIT_MODE_REMAINING:
                child_size = parent_size - offset;
                break;

            case UI_SPLIT_MODE_FONT: {
                if (child->args == NULL) {
                    ui_dprintf("[ERROR] args was NULL\n");
                    goto err;
                }

                const painter_font_handle_t font = qp_load_font_mem(*(void **)child->args);
                if (font == NULL) {
                    ui_dprintf("[ERROR] could not load font\n");
                    goto err;
                }

                child_size = ui_handle_font(font, parent, child->node_size.size);
                qp_close_font(font);
                if (child_size == UI_COORD_MAX) {
                    goto err;
                }

                break;
            }

            case UI_SPLIT_MODE_IMAGE: {
                if (child->args == NULL) {
                    ui_dprintf("[ERROR] args was NULL\n");
                    goto err;
                }

                const painter_image_handle_t image = qp_load_image_mem(*(void **)child->args);
                if (image == NULL) {
                    ui_dprintf("[ERROR] could not load image\n");
                    goto err;
                }

                child_size = ui_handle_image(image, parent, child->node_size.size);
                qp_close_image(image);
                if (child_size == UI_COORD_MAX) {
                    goto err;
                }

                break;
            }
        }

        // set child's start/size values
        switch (parent->direction) {
            default:
                goto err; // unreachable

            case UI_SPLIT_DIR_LEFT_RIGHT:
                child->start = (ui_vector_t){
                    .x = parent->start.x + offset,
                    .y = parent->start.y,
                };

                child->size = (ui_vector_t){
                    .x = child_size,
                    .y = parent->size.y,
                };

                break;

            case UI_SPLIT_DIR_RIGHT_LEFT:
                child->start = (ui_vector_t){
                    .x = parent->start.x + parent->size.x - offset - child_size,
                    .y = parent->start.y,
                };

                child->size = (ui_vector_t){
                    .x = child_size,
                    .y = parent->size.y,
                };

                break;

            case UI_SPLIT_DIR_TOP_BOTTOM:
                child->start = (ui_vector_t){
                    .x = parent->start.x,
                    .y = parent->start.y + offset,
                };

                child->size = (ui_vector_t){
                    .x = parent->size.x,
                    .y = child_size,
                };

                break;

            case UI_SPLIT_DIR_BOTTOM_TOP:
                child->start = (ui_vector_t){
                    .x = parent->start.x,
                    .y = parent->start.y + parent->size.y - offset - child_size,
                };

                child->size = (ui_vector_t){
                    .x = parent->size.x,
                    .y = child_size,
                };

                break;
        }

        // update offset
        offset += child_size;
        if (offset > parent_size) {
            ui_dprintf("[ERROR] children (%d) don't fit in parent (%d)\n", offset, parent_size);
            goto err;
        }

        // traverse child
        if (!ui_init_node(child)) {
            goto err;
        }
    }

ok:
    if (parent->init != NULL) {
        const ui_vector_t start = parent->start;
        const ui_vector_t size  = parent->size;

        if (!parent->init(parent)) {
            ui_dprintf("[ERROR] init failed\n");
            goto err;
        }

        if (!ui_vector_eq(start, parent->start) || !ui_vector_eq(size, parent->size)) {
            ui_dprintf("[ERROR] init changed computed boundaries\n");
            goto err;
        }
    }

    parent->state = UI_STATE_OK;
    return true;

err:
    parent->state = UI_STATE_ERR;
    return false;
}

static bool ui_time_eq(ui_time_t lhs, ui_time_t rhs) {
    if (lhs.type == rhs.type) {
        if (lhs.type == UI_TIME_TYPE_STOP) {
            return true;
        }

        return lhs.value == rhs.value;
    }

    return false;
}

//
// Public API
//

bool ui_time_lte(ui_time_t lhs, ui_time_t rhs) {
    assert(lhs.type == rhs.type && lhs.type != UI_TIME_TYPE_STOP);
    return lhs.value <= rhs.value;
}

ui_time_t ui_time_add(ui_time_t lhs, ui_time_t rhs) {
    assert(lhs.type == rhs.type && lhs.type != UI_TIME_TYPE_STOP);
    return (ui_time_t)UI_MILLISECONDS(lhs.value + rhs.value);
}

ui_time_t ui_time_now(void) {
    return (ui_time_t)UI_MILLISECONDS(timer_read32());
}

bool ui_init(ui_node_t *root, ui_coord_t width, ui_coord_t height) {
    if (root == NULL) {
        ui_dprintf("[ERROR] received NULL\n");
        return false;
    }

    if (root->state != UI_STATE_NONE) {
        ui_dprintf("[DEBUG] Called init twice\n");
        return root->state == UI_STATE_OK;
    }

    const ui_vector_t zero = {
        .x = 0,
        .y = 0,
    };

    if (!ui_vector_eq(root->size, zero)) {
        ui_dprintf("[ERROR] must not configure size on root node\n");
        root->state = UI_STATE_ERR;
        return false;
    }

    root->size = (ui_vector_t){
        .x = width,
        .y = height,
    };

    return ui_init_node(root);
}

bool ui_render(ui_node_t *root, painter_device_t display) {
    if (root == NULL || display == NULL) {
        ui_dprintf("[ERROR] received NULL\n");
        return false;
    }

    if (root->state != UI_STATE_OK) {
        return false;
    }

    const ui_coord_t end_x = root->start.x + root->size.x;
    const ui_coord_t end_y = root->start.y + root->size.y;

    const bool x_fits = end_x <= qp_get_width(display);
    const bool y_fits = end_y <= qp_get_height(display);
    if (!x_fits || !y_fits) {
        ui_dprintf("[ERROR] computed coords don't fit in display\n");
        return false;
    }

    // leaf node, render it
    if (root->render != NULL) {
        const ui_time_t now  = ui_time_now();
        const ui_time_t stop = UI_STOP;

        // this node halted
        if (ui_time_eq(root->next_render, stop)) {
            return true;
        }

        // delay already elapsed, draw
        if (ui_time_lte(root->next_render, now)) {
            const ui_time_t next = root->render(root, display);

            if (ui_time_eq(next, stop)) {
                root->next_render = stop;
            } else {
                root->next_render = ui_time_add(now, next);
            }
        }

        return true;
    }

    // parent node, traverse children
    for (size_t i = 0; i < root->children.n; ++i) {
        if (!ui_render(root->children.ptr + i, display)) {
            return false;
        }
    }

    return true;
}

void ui_print(const ui_node_t *root) {
    ui_print_node(root, 0);
}
