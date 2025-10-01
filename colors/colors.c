// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/colors.h"

int to_rgb(color_t color, rgb_t *rgb) {
    switch (color.type) {
        case COLOR_TYPE_RGB:
            *rgb = color.rgb;
            break;

        case COLOR_TYPE_HSV:
            *rgb = hsv_to_rgb(color.hsv);
            break;

        case COLOR_TYPE_HUE:
            *rgb = hsv_to_rgb((hsv_t){
                .h = color.hsv.h,
                .s = rgb_matrix_get_sat(),
                .v = rgb_matrix_get_val(),
            });
            break;

        case COLOR_TYPE_WHITE:
            *rgb = hsv_to_rgb((hsv_t){
                .h = 0,
                .s = 0,
                .v = rgb_matrix_get_val(),
            });
            break;

        case COLOR_TYPE_TRNS:
            return -ENODATA;

        // unknown variant
        default:
            return -EINVAL;
    }

    return 0;
}
