#!/usr/bin/env python3

"""Tool to generate stub files for the bindings here.

This only serves as some documentation, because the code can't even be
imported (.pyi)
"""

from __future__ import annotations

import argparse
from pathlib import Path

HERE = Path.cwd()

DOCSTRING = "//!"
CONTENT = "//|"


def _file(raw: str) -> Path:
    path = Path(raw)

    if not path.exists():
        msg = f"'{raw}' does not exist."
        raise argparse.ArgumentTypeError(msg)

    if not path.is_file():
        msg = f"'{raw}' is not a file."
        raise argparse.ArgumentTypeError(msg)

    return path


class Error(Exception):
    def __init__(self, msg: str) -> None:
        super().__init__()
        self.msg = msg

    def __str__(self) -> str:
        return self.msg


class _Pyi:
    def __init__(self) -> None:
        self._docstring: list[str] = []
        self._content: list[str] = []

    def get_docstring(self) -> str | None:
        if not self._docstring:
            return None

        return "\n".join(self._docstring)

    def get_content(self) -> str | None:
        if not self._content:
            return None

        return "\n".join(self._content)

    def is_empty(self) -> bool:
        return not (self._docstring or self._content)

    @staticmethod
    def strip_marker(line: str, marker: str) -> str | None:
        if not line.startswith(marker):
            return None

        offset = len(marker)

        # empty line, just the marker
        if len(line) == offset:
            return ""

        if not line[offset] == " ":
            msg = "Whitespace between marker and content in mandatory."
            raise Error(msg)

        return line[offset + 1 :]

    def process(self, line: str) -> None:
        for marker, target in (
            (DOCSTRING, self._docstring),
            (CONTENT, self._content),
        ):
            text = self.strip_marker(line, marker)
            if text is None:
                continue

            if marker is DOCSTRING and self._content:
                msg = "Docstring comments must precede any content ones."
                raise Error(msg)

            target.append(text)
            return

    def write(self, file: Path) -> None:
        if self.is_empty():
            return

        lines = [
            "# THIS FILE WAS GENERATED, DO NOT MODIFY IT",
            "",
        ]

        docstring = self.get_docstring()
        if docstring is not None:
            lines.extend(
                [
                    f'"""{docstring}\n"""',
                    "",
                ],
            )

        lines.extend(
            [
                "from __future__ import annotations",
                "",
            ],
        )

        content = self.get_content()
        if content is not None:
            lines.append(content)

        stub = file.with_suffix(".pyi")
        stub.write_text("\n".join(lines))


def _generate(file: Path) -> None:
    assert file.is_file()

    pyi = _Pyi()
    with file.open() as f:
        for line in f.readlines():
            pyi.process(line[:-1])  # remove trailing newline

    pyi.write(file)


def main() -> int:
    """Entrypoint."""
    parser = argparse.ArgumentParser(
        prog=__name__,
        description=__doc__,
    )

    parser.add_argument(
        "files",
        type=_file,
        nargs="*",
    )

    args = parser.parse_args()
    files: list[Path] = args.files

    for file in files:
        _generate(file)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
