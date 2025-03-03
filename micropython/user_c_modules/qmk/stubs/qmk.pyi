# THIS FILE WAS GENERATED, DO NOT MODIFY IT

"""Utilities to interact with QMK from MicroPython."""

# ruff: noqa: F401
# the modules being imported dont really exist on the VM
# these imports are the result of having multiple `.c` files
# to organize the code (each one gets its own `.pyi` generated)
#
# this is: you can't `import qmk.keycode` nor `import _keycode`
# instead, you `import qmk` and use it as `qmk.keycode.foo()`

import _keycode as keycode
import _rgb as rgb

version: str
"""Version of QMK on which this firmware was built, as a raw string."""

version_info: tuple[int, int, int]
"""Version of QMK on which this firmware was built, as a (major, minor, patch) tuple."""

def get_highest_active_layer() -> int:
    """Get what the highest (currently active) layer is."""

def send_string(text: str, /) -> None:
    """Send a string over HID."""

def tap_code(kc: int, /) -> None:
    """Send a basic keycode over HID."""
