#!/usr/bin/env python3

"""Generate globals table for all char keycodes."""

# TODO: Support numbers, symbols and whatnot

TEMPLATE = """\
    //| KC_{0}: int
    {{ MP_ROM_QSTR(MP_QSTR_KC_{0}), MP_ROM_INT(KC_{0}) }},\
"""

for ord_ in range(ord("A"), ord("Z") + 1):
    chr_ = chr(ord_)
    print(TEMPLATE.format(chr_))
