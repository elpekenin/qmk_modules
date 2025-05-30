// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

//! Registry of QMK keycodes.

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "qmk.h"

static inline mp_int_t validate_keycode(const mp_obj_t kc_in) {
    mp_int_t kc = mp_obj_get_int(kc_in);
    if (kc > QK_MODS_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("keycode too big"));
    }

    return kc;
}

static mp_obj_t qmk_c(const mp_obj_t kc_in) {
    mp_int_t kc = validate_keycode(kc_in);
    return MP_OBJ_NEW_SMALL_INT(C(kc));
}
static MP_DEFINE_CONST_FUN_OBJ_1(qmk_c_obj, qmk_c);

static mp_obj_t qmk_s(const mp_obj_t kc_in) {
    mp_int_t kc = validate_keycode(kc_in);
    return MP_OBJ_NEW_SMALL_INT(S(kc));
}
static MP_DEFINE_CONST_FUN_OBJ_1(qmk_s_obj, qmk_s);

static mp_obj_t qmk_a(const mp_obj_t kc_in) {
    mp_int_t kc = validate_keycode(kc_in);
    return MP_OBJ_NEW_SMALL_INT(A(kc));
}
static MP_DEFINE_CONST_FUN_OBJ_1(qmk_a_obj, qmk_a);

static mp_obj_t qmk_g(const mp_obj_t kc_in) {
    mp_int_t kc = validate_keycode(kc_in);
    return MP_OBJ_NEW_SMALL_INT(G(kc));
}
static MP_DEFINE_CONST_FUN_OBJ_1(qmk_g_obj, qmk_g);

static const mp_rom_map_elem_t qmk_keycode_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_qmk_dot_keycode)},

    //| KC_A: int
    {MP_ROM_QSTR(MP_QSTR_KC_A), MP_ROM_INT(KC_A)},
    //| KC_B: int
    {MP_ROM_QSTR(MP_QSTR_KC_B), MP_ROM_INT(KC_B)},
    //| KC_C: int
    {MP_ROM_QSTR(MP_QSTR_KC_C), MP_ROM_INT(KC_C)},
    //| KC_D: int
    {MP_ROM_QSTR(MP_QSTR_KC_D), MP_ROM_INT(KC_D)},
    //| KC_E: int
    {MP_ROM_QSTR(MP_QSTR_KC_E), MP_ROM_INT(KC_E)},
    //| KC_F: int
    {MP_ROM_QSTR(MP_QSTR_KC_F), MP_ROM_INT(KC_F)},
    //| KC_G: int
    {MP_ROM_QSTR(MP_QSTR_KC_G), MP_ROM_INT(KC_G)},
    //| KC_H: int
    {MP_ROM_QSTR(MP_QSTR_KC_H), MP_ROM_INT(KC_H)},
    //| KC_I: int
    {MP_ROM_QSTR(MP_QSTR_KC_I), MP_ROM_INT(KC_I)},
    //| KC_J: int
    {MP_ROM_QSTR(MP_QSTR_KC_J), MP_ROM_INT(KC_J)},
    //| KC_K: int
    {MP_ROM_QSTR(MP_QSTR_KC_K), MP_ROM_INT(KC_K)},
    //| KC_L: int
    {MP_ROM_QSTR(MP_QSTR_KC_L), MP_ROM_INT(KC_L)},
    //| KC_M: int
    {MP_ROM_QSTR(MP_QSTR_KC_M), MP_ROM_INT(KC_M)},
    //| KC_N: int
    {MP_ROM_QSTR(MP_QSTR_KC_N), MP_ROM_INT(KC_N)},
    //| KC_O: int
    {MP_ROM_QSTR(MP_QSTR_KC_O), MP_ROM_INT(KC_O)},
    //| KC_P: int
    {MP_ROM_QSTR(MP_QSTR_KC_P), MP_ROM_INT(KC_P)},
    //| KC_Q: int
    {MP_ROM_QSTR(MP_QSTR_KC_Q), MP_ROM_INT(KC_Q)},
    //| KC_R: int
    {MP_ROM_QSTR(MP_QSTR_KC_R), MP_ROM_INT(KC_R)},
    //| KC_S: int
    {MP_ROM_QSTR(MP_QSTR_KC_S), MP_ROM_INT(KC_S)},
    //| KC_T: int
    {MP_ROM_QSTR(MP_QSTR_KC_T), MP_ROM_INT(KC_T)},
    //| KC_U: int
    {MP_ROM_QSTR(MP_QSTR_KC_U), MP_ROM_INT(KC_U)},
    //| KC_V: int
    {MP_ROM_QSTR(MP_QSTR_KC_V), MP_ROM_INT(KC_V)},
    //| KC_W: int
    {MP_ROM_QSTR(MP_QSTR_KC_W), MP_ROM_INT(KC_W)},
    //| KC_X: int
    {MP_ROM_QSTR(MP_QSTR_KC_X), MP_ROM_INT(KC_X)},
    //| KC_Y: int
    {MP_ROM_QSTR(MP_QSTR_KC_Y), MP_ROM_INT(KC_Y)},
    //| KC_Z: int
    {MP_ROM_QSTR(MP_QSTR_KC_Z), MP_ROM_INT(KC_Z)},

    //|
    //| def C(kc: int, /) -> int:  # noqa: N802  # name mimics QMK's
    //|     """Return control + `kc` combination."""
    //|
    {MP_ROM_QSTR(MP_QSTR_C), MP_ROM_PTR(&qmk_c_obj)},
    //| def S(kc: int, /) -> int:  # noqa: N802  # name mimics QMK's
    //|     """Return shift + `kc` combination."""
    //|
    {MP_ROM_QSTR(MP_QSTR_S), MP_ROM_PTR(&qmk_s_obj)},
    //| def G(kc: int, /) -> int:  # noqa: N802  # name mimics QMK's
    //|     """Return gui + `kc` combination."""
    //|
    {MP_ROM_QSTR(MP_QSTR_A), MP_ROM_PTR(&qmk_a_obj)},
    //| def A(kc: int, /) -> int:  # noqa: N802  # name mimics QMK's
    //|     """Return alt + `kc` combination."""
    //|
    {MP_ROM_QSTR(MP_QSTR_G), MP_ROM_PTR(&qmk_g_obj)},
};
static MP_DEFINE_CONST_DICT(qmk_keycode_globals, qmk_keycode_globals_table);

const mp_obj_module_t qmk_keycode = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&qmk_keycode_globals,
};
