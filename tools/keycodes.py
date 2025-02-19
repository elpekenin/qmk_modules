#!/usr/bin/env python3

"""Generate globals table for all char keycodes."""

# TODO(elpekenin): Support numbers, symbols and whatnot

import sys

TEMPLATE = """\
    //| KC_{0}: int
    {{ MP_ROM_QSTR(MP_QSTR_KC_{0}), MP_ROM_INT(KC_{0}) }},\
"""

for ord_ in range(ord("A"), ord("Z") + 1):
    chr_ = chr(ord_)
    sys.stdout.write(TEMPLATE.format(chr_) + "\n")
