//! Interact with RGB LEDs.

// TODO:
//   * more color constants
//   * HSV to RGB
//   * get/set mode
//   * get/set global settings

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include "qmk.h"

#if defined(RGB_MATRIX_ENABLE) || COLLECTING_QSTR == 1
typedef struct _qmk_rgb_t {
    mp_obj_base_t base;
    rgb_t         inner;
} qmk_rgb_t;

typedef enum {
    none,
    r,
    g,
    b,
} color_channel_t;

static inline mp_int_t validate_color(mp_int_t color) {
    if (color < 0 || color > 255) {
        mp_raise_ValueError(MP_ERROR_TEXT("color must be 0-255"));
    }

    return color;
}

// RGB.__getattr__ + RGB.__setattr__
static void qmk_rgb_RGB_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    color_channel_t channel = none;

    if (attr == MP_QSTR_r)
        channel = r;
    else if (attr == MP_QSTR_g)
        channel = g;
    else if (attr == MP_QSTR_b)
        channel = b;

    // attribute not found, continue lookup in locals dict.
    if (channel == none) {
        dest[1] = MP_OBJ_SENTINEL;
        return;
    }

    qmk_rgb_t *self = MP_OBJ_TO_PTR(self_in);

    // read operation
    if (dest[0] == MP_OBJ_NULL) {
        mp_uint_t value;
        switch (channel) {
            case r:
                value = self->inner.r;
                break;

            case g:
                value = self->inner.g;
                break;

            case b:
                value = self->inner.b;
                break;

            case none:
                MP_UNREACHABLE;
        }

        dest[0] = mp_obj_new_int_from_uint(value);
        return;
    }

    if (dest[0] == MP_OBJ_SENTINEL) {
        // delete -> no-op
        if (dest[1] == MP_OBJ_NULL) {
            return;
        }

        // write
        mp_uint_t value = validate_color(mp_obj_get_int(dest[1]));
        switch (channel) {
            case r:
                self->inner.r = value;
                break;

            case g:
                self->inner.g = value;
                break;

            case b:
                self->inner.b = value;
                break;

            case none:
                MP_UNREACHABLE;
        }

        dest[0] = MP_OBJ_NULL;
    }
}

// RGB.__new__ + RGB.__init__
static mp_obj_t qmk_rgb_RGB_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse arguments
    enum { ARG_r, ARG_g, ARG_b };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_r, MP_ARG_INT | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_g, MP_ARG_INT | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_b, MP_ARG_INT | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_NONE}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t r = validate_color(args[ARG_r].u_int);
    mp_int_t g = validate_color(args[ARG_g].u_int);
    mp_int_t b = validate_color(args[ARG_b].u_int);

    // create new object
    qmk_rgb_t *self = mp_obj_malloc(qmk_rgb_t, type);
    self->inner     = (rgb_t){r, g, b};

    // return it
    return MP_OBJ_FROM_PTR(self);
}

// clang-format off
MP_DEFINE_CONST_OBJ_TYPE(
    qmk_rgb_RGB,
    MP_QSTR_RGB,
    MP_TYPE_FLAG_NONE,
    attr, qmk_rgb_RGB_attr,
    make_new, qmk_rgb_RGB_make_new
);
// clang-format on

static const qmk_rgb_t qmk_rgb_RED = {
    .base  = {&qmk_rgb_RGB},
    .inner = (rgb_led_t){RGB_RED},
};

static const qmk_rgb_t qmk_rgb_GREEN = {
    .base  = {&qmk_rgb_RGB},
    .inner = (rgb_led_t){RGB_GREEN},
};

static const qmk_rgb_t qmk_rgb_BLUE = {
    .base  = {&qmk_rgb_RGB},
    .inner = (rgb_led_t){RGB_BLUE},
};

static mp_obj_t qmk_rgb_set_color(mp_obj_t index_in, mp_obj_t rgb_in) {
    mp_int_t index = mp_obj_get_int(index_in);

    if (!mp_obj_is_type(rgb_in, &qmk_rgb_RGB)) {
        mp_raise_TypeError(MP_ERROR_TEXT("color is not RGB"));
    }
    qmk_rgb_t *rgb = MP_OBJ_TO_PTR(rgb_in);

    rgb_matrix_set_color(index, rgb->inner.r, rgb->inner.g, rgb->inner.b);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(qmk_rgb_set_color_obj, qmk_rgb_set_color);

static const mp_rom_map_elem_t qmk_rgb_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_qmk_dot_rgb)},

    //| RED: RGB
    {MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_PTR(&qmk_rgb_RED)},
    //| GREEN: RGB
    {MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_PTR(&qmk_rgb_GREEN)},
    //| BLUE: RGB
    {MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_PTR(&qmk_rgb_BLUE)},
    //|
    //| class RGB:
    //|     """Represent a color."""
    //|
    //|     r: int
    //|     g: int
    //|     b: int
    //|
    //|     def __init__(self, r: int, g: int, b: int) -> None:
    //|         """Create instance from the given color channels."""
    //|
    {MP_ROM_QSTR(MP_QSTR_RGB), MP_ROM_PTR(&qmk_rgb_RGB)},
    //| def set_color(index: int, rgb: RGB, /) -> None:
    //|     """Configure a LED's color."""
    //|
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&qmk_rgb_set_color_obj)},
};
static MP_DEFINE_CONST_DICT(qmk_rgb_globals, qmk_rgb_globals_table);

const mp_obj_module_t qmk_rgb = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&qmk_rgb_globals,
};
#endif
