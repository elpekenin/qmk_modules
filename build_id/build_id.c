// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/build_id.h"

// TODO?: assert that desc_size==128
typedef struct {
    uint32_t name_size;
    uint32_t desc_size;
    uint32_t type;
    uint8_t  data[];
} gnu_note_t;

extern gnu_note_t __gnu_build_id__;

const u128 get_build_id(void) {
    // skip the name
    const uint8_t *data = __gnu_build_id__.data;
    data += __gnu_build_id__.name_size;

    // re-interpret cast and copy contents
    u128 id = *(u128 *)data;

    return id;
}
