# THIS FILE WAS GENERATED, DO NOT MODIFY IT

"""Utilities to interact with QMK from MicroPython."""

from __future__ import annotations

import qmk_keycode as keycode  # noqa: F401

version: str
"""Version of QMK on which this firmware was built, as a raw string."""

version_info: tuple[int, int, int]
"""Version of QMK on which this firmware was built, as a (major, minor, patch) tuple."""

def tap_code(kc: int) -> None:
    """Send a basic keycode over HID."""

def send_string(text: str) -> None:
    """Send a string over HID."""
