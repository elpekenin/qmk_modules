# THIS FILE WAS GENERATED, DO NOT MODIFY IT

"""Utilities to interact with QMK from MicroPython."""

# ruff: noqa: F401
# the modules being imported dont really exist on the VM
# these imports are the result of having multiple `.c` files
# to organize the code (each one gets its own `.pyi` generated)

import _keycode as keycode
import _rgb as rgb

version: str
"""Version of QMK on which this firmware was built, as a raw string."""

version_info: tuple[int, int, int]
"""Version of QMK on which this firmware was built, as a (major, minor, patch) tuple."""

def tap_code(kc: int, /) -> None:
    """Send a basic keycode over HID."""

def send_string(text: str, /) -> None:
    """Send a string over HID."""
