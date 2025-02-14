//! Utilities to interact with QMK from MicroPython

#include "py/obj.h"
#include "py/objstr.h"

// this header is available when building QMK
// but not when MicroPy collects QSTRs from files
#if __has_include("version.h")
#    include "version.h"
#endif

//| version: str
//| """Version of QMK on which this firmware was built."""
//|
static const MP_DEFINE_STR_OBJ(mp_obj_qmk_version, QMK_VERSION);

static const mp_rom_map_elem_t mp_module_qmk_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_obj_qmk_version) },
};
static MP_DEFINE_CONST_DICT(mp_module_qmk_globals, mp_module_qmk_globals_table);

const mp_obj_module_t mp_module_qmk = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_qmk_globals,
};

MP_REGISTER_MODULE(MP_QSTR_qmk, mp_module_qmk);
