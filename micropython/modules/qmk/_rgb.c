//! Interact with RGB LEDs.

#ifdef RGB_MATRIX_ENABLE

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include "qmk.h"

typedef struct _RGB_t {
    // All objects start with the base.
    mp_obj_base_t base;

    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_t;

static inline mp_int_t validate_color(mp_arg_val_t color_in) {
    mp_int_t color = color_in.u_int;
    if (x < 0 || x > 255) {
        mp_raise_ValueError(MP_ERROR_TEXT("color must be 0-255"));
    }

    return color;
}

// RGB.__new__ + RGB.__init__
static mp_obj_t qmk_rgb_RGB_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse arguments
    enum { ARG_r, ARG_g, ARG_b };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_r, MP_ARG_INT | MP_ARG_REQUIRED,  {.u_rom_obj = MP_ROM_NONE } },
        { MP_QSTR_g, MP_ARG_INT | MP_ARG_REQUIRED,  {.u_rom_obj = MP_ROM_NONE } },
        { MP_QSTR_b, MP_ARG_INT | MP_ARG_REQUIRED,  {.u_rom_obj = MP_ROM_NONE } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t r = validate_color(args[ARG_r]);
    mp_int_t g = validate_color(args[ARG_g]);
    mp_int_t b = validate_color(args[ARG_b]);

    // create new object
    RGB_t *self = mp_obj_malloc(RGB_t, type);
    self->r = r;
    self->g = g;
    self->b = b;

    // return it
    return MP_OBJ_FROM_PTR(self);
}

// This defines the type(Timer) object.
MP_DEFINE_CONST_OBJ_TYPE(
    qmk_type_RGB,
    MP_QSTR_RGB,
    MP_TYPE_FLAG_NONE,
    make_new, qmk_rgb_RGB_make_new
);

static mp_obj_t qmk_rgb_set_color(mp_obj_t index_in, mp_obj_t rgb_in) {
    mp_int_t index = mp_obj_get_int(index_in);

    if (!mp_obj_is_type(rgb_in, &qmk_type_RGB)) {
        mp_raise_TypeError(MP_ERROR_TEXT("color is not RGB"));
    }
    RGB_t *rgb = MP_OBJ_TO_PTR(rgb_in);

    rgb_matrix_set_color(index, rgb->r, rgb->g, rgb->b);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(qmk_rgb_set_color_obj, qmk_rgb_set_color);

static const mp_rom_map_elem_t mp_qmk_rgb_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_qmk_dot_rgb)},

    //| class RGB:
    //|     """Represent a color."""
    //|
    //|     def __init__(self, r: int, g: int, b: int) -> None:
    //|         """Create instance from the given color channels."""
    //|
    { MP_ROM_QSTR(MP_QSTR_RGB), MP_ROM_PTR(&qmk_type_RGB)},
    //| def set_color(index: int, rgb: RGB, /) -> None:
    //|     """Configure a LED's color."""
    //|
    { MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&qmk_rgb_set_color_obj)},
};
static MP_DEFINE_CONST_DICT(mp_qmk_rgb_globals, mp_qmk_rgb_globals_table);

const mp_obj_module_t mp_qmk_rgb = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_qmk_rgb_globals,
};

#endif
