//! Utilities to interact with QMK from MicroPython.

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

// these headers are available when building QMK
// but not when MicroPy collects QSTRs from the file
#if __has_include("quantum.h")
#    include "quantum.h"
#    include "version.h"
#endif

extern mp_obj_module_t mp_qmk_keycode;

static const MP_DEFINE_STR_OBJ(mp_qmk_version, QMK_VERSION);

// TODO: send PR upstream to get these numbers generated on `version.h` as well, rather than hardcoding
static const mp_rom_obj_tuple_t mp_qmk_version_info = {
    .base = { &mp_type_tuple },
    .len = 3,
    .items = {
        MP_ROM_INT(0),
        MP_ROM_INT(27),
        MP_ROM_INT(12),
    },
};

static mp_obj_t mp_qmk_tap_code(const mp_obj_t kc_obj) {
    mp_int_t kc = mp_obj_get_int(kc_obj);

    if (kc > QK_MODS_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("keycode too big"));
    }

    tap_code16(kc);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_qmk_tap_code_obj, mp_qmk_tap_code);

static mp_obj_t mp_qmk_send_string(const mp_obj_t str_obj) {
    mp_check_self(mp_obj_is_str_or_bytes(str_obj));

    GET_STR_DATA_LEN(str_obj, str, len);

    send_string((const char *)str); // FIXME: ugh
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_qmk_send_string_obj, mp_qmk_send_string);

static const mp_rom_map_elem_t mp_qmk_globals_table[] = {
    //| import qmk_keycode as keycode  # noqa: F401
    //|
    { MP_ROM_QSTR(MP_QSTR_keycode), MP_ROM_PTR(&mp_qmk_keycode) },

    //| version: str
    //| """Version of QMK on which this firmware was built, as a raw string."""
    //|
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_qmk_version) },
    //| version_info: tuple[int, int, int]
    //| """Version of QMK on which this firmware was built, as a (major, minor, patch) tuple."""
    //|
    { MP_ROM_QSTR(MP_QSTR_version_info), MP_ROM_PTR(&mp_qmk_version_info) },
    //| def tap_code(kc: int) -> None:
    //|     """Send a basic keycode over HID."""
    //|
    { MP_ROM_QSTR(MP_QSTR_tap_code), MP_ROM_PTR(&mp_qmk_tap_code_obj) },
    //| def send_string(text: str) -> None:
    //|     """Send a string over HID."""
    //|
    { MP_ROM_QSTR(MP_QSTR_send_string), MP_ROM_PTR(&mp_qmk_send_string_obj) },
};
static MP_DEFINE_CONST_DICT(mp_qmk_globals, mp_qmk_globals_table);

const mp_obj_module_t mp_qmk = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_qmk_globals,
};

MP_REGISTER_MODULE(MP_QSTR_qmk, mp_qmk);
