// Copyright 2025 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/graph.h"

typedef bool (*draw_fn_t)(const graph_config_t *config, const graph_line_t *line);

//

static inline uint8_t scale_value(uint8_t value, uint8_t from, uint8_t to) {
    return (value * from / to);
}

static inline uint16_t in_range(uint16_t value, uint16_t start, uint16_t end) {
    return MIN(MAX(start, value), end);
}

static point_t get_end(const graph_config_t *config) {
    return (point_t){
        .x = config->start.x + config->size.x,
        .y = config->start.y + config->size.y,
    };
}

static bool draw_line(const graph_config_t *config, const graph_line_t *line) {
    const uint16_t step = config->size.x / config->data_points;

    const point_t end = get_end(config);

    // -1 because we will also access next point on each iteration
    for (uint8_t n = 0; n < config->data_points - 1; ++n) {
        const uint16_t x1 = config->start.x + (step * n);
        const uint16_t y1 = end.y - scale_value(line->data[n], config->size.y - 1, line->max_value);

        const uint16_t x2 = config->start.x + (step * n + 1);
        const uint16_t y2 = end.y - scale_value(line->data[n + 1], config->size.y - 1, line->max_value);

        if (!qp_line(config->device, x1, y1, x2, y2, line->color.h, line->color.s, line->color.v)) {
            return false;
        }
    }

    return true;
}

static bool draw_point(const graph_config_t *config, const graph_line_t *line) {
    const uint16_t step = config->size.x / config->data_points;

    const point_t end = get_end(config);

    for (uint8_t n = 0; n < config->data_points - 1; ++n) {
        const uint16_t x = config->start.x + (step * n);
        const uint16_t y = end.y - scale_value(line->data[n], config->size.y - 1, line->max_value);

        if (!qp_setpixel(config->device, x, y, line->color.h, line->color.s, line->color.v)) {
            return false;
        }
    }

    return true;
}

static bool draw_square_line(const graph_config_t *config, const graph_line_t *line) {
    const uint16_t step = config->size.x / config->data_points;

    const point_t end = get_end(config);

    // -1 because we will also access next point on each iteration
    for (uint8_t n = 0; n < config->data_points - 1; ++n) {
        const uint16_t x1 = config->start.x + (step * n);
        const uint16_t y1 = end.y - scale_value(line->data[n], config->size.y - 1, line->max_value);

        const uint16_t x2 = config->start.x + (step * n + 1);
        const uint16_t y2 = end.y - scale_value(line->data[n + 1], config->size.y - 1, line->max_value);

        if (!qp_line(config->device, x1, y1, x2, y1, line->color.h, line->color.s, line->color.v)) {
            return false;
        }

        if (!qp_line(config->device, x2, y1, x2, y2, line->color.h, line->color.s, line->color.v)) {
            return false;
        }
    }

    return true;
}

static const draw_fn_t draw_functions[] = {
    [LINE]         = draw_line,
    [POINT]        = draw_point,
    [SQUARED_LINE] = draw_square_line,
};

bool qp_draw_graph(const graph_config_t *config, const graph_line_t *lines) {
    // if there are more segments than the graph width is wide in pixels, reject drawing
    if (config->data_points >= config->size.x) {
        return false;
    }

    // clear the graph area for redrawing
    if (!qp_rect(config->device, config->start.x, config->start.y, config->start.x + config->size.x, config->start.y + config->size.y, config->background.h, config->background.s, config->background.v, true)) {
        return false;
    }

    // Draw graph axes
    if (!qp_line(config->device, config->start.x, config->start.y, config->start.x, config->start.y + config->size.y, config->axis.h, config->axis.s, config->axis.v)) {
        return false;
    }

    if (!qp_line(config->device, config->start.x, config->start.y + config->size.y, config->start.x + config->size.x, config->start.y + config->size.y, config->axis.h, config->axis.s, config->axis.v)) {
        return false;
    }

    const graph_line_t *line = lines;
    while (line->data != NULL) {
        const draw_fn_t function = draw_functions[line->mode];
        if (!function(config, line)) {
            return false;
        }

        line++;
    }

    return true;
}
