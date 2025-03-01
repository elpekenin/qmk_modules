# THIS FILE WAS GENERATED, DO NOT MODIFY IT

"""Interact with RGB LEDs."""

class RGB:
    """Represent a color."""

    r: int
    g: int
    b: int

    def __init__(self, r: int, g: int, b: int) -> None:
        """Create instance from the given color channels."""

RED: RGB
GREEN: RGB
BLUE: RGB

def set_color(index: int, rgb: RGB, /) -> None:
    """Configure a LED's color."""
