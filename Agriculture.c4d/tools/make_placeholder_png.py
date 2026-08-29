#!/usr/bin/env python3
"""Generate a solid-colour placeholder PNG using only the Python stdlib.

Usage: make_placeholder_png.py <out_path> <width> <height> [r g b]
No external deps (no PIL). Produces an 8-bit RGB PNG.
"""
import struct
import sys
import zlib


def _chunk(typ: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + typ
        + data
        + struct.pack(">I", zlib.crc32(typ + data) & 0xFFFFFFFF)
    )


def make_png(path: str, w: int, h: int, rgb=(128, 128, 128)) -> None:
    # One filter-byte (0 = None) per scanline, then RGB pixels.
    raw = b"".join(b"\x00" + bytes(rgb) * w for _ in range(h))
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)  # 8-bit, colour type 2 (RGB)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(raw))
        + _chunk(b"IEND", b"")
    )
    with open(path, "wb") as f:
        f.write(png)


if __name__ == "__main__":
    args = sys.argv
    if len(args) < 4:
        raise SystemExit("usage: make_placeholder_png.py <path> <w> <h> [r g b]")
    rgb = (128, 128, 128)
    if len(args) >= 7:
        rgb = (int(args[4]), int(args[5]), int(args[6]))
    make_png(args[1], int(args[2]), int(args[3]), rgb)
