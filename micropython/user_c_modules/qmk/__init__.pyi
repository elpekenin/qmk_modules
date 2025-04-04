# THIS FILE WAS GENERATED

# Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
# SPDX-License-Identifier: GPL-2.0-or-later

"""Utilities to interact with QMK from MicroPython."""


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
