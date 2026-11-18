"""Reads the PNG files of the visual proof, refusing anything unexpected (INV-12).

Only plain 8-bit images are accepted: red, green and blue, with or without alpha.
"""

from __future__ import annotations

import struct
import zlib
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

# The budget of what this tool reads (INV-14): more than a 4K image is not a proof image.
MAX_PIXELS = 3840 * 2160
MAX_FILE_BYTES = 64 * 1024 * 1024

_SIGNATURE = b"\x89PNG\r\n\x1a\n"
_HEADER_SIZE = 13
_CHANNELS_OF_COLOR_TYPE = {2: 3, 6: 4}


class ImageError(Exception):
    """The file is not an image this tool can read."""


@dataclass(frozen=True)
class RgbaImage:
    """Four bytes per pixel (red, green, blue, alpha), row by row from the top."""

    width: int
    height: int
    pixels: bytes


@dataclass(frozen=True)
class _Header:
    width: int
    height: int
    channels: int


def read_png_file(path: Path) -> RgbaImage:
    try:
        if path.stat().st_size > MAX_FILE_BYTES:
            raise ImageError(f"'{path}' is too large")
        data = path.read_bytes()
    except OSError as error:
        raise ImageError(f"could not read '{path}': {error}") from error
    return read_png(data)


def read_png(data: bytes) -> RgbaImage:
    if len(data) > MAX_FILE_BYTES:
        raise ImageError("the file is too large")
    if not data.startswith(_SIGNATURE):
        raise ImageError("the file is not a PNG")
    header, compressed = _read_chunks(data)
    filtered = _unpack(compressed, header.height * (1 + header.width * header.channels))
    pixels = _undo_filters(filtered, header)
    return RgbaImage(header.width, header.height, _to_rgba(pixels, header.channels))


def _read_chunks(data: bytes) -> tuple[_Header, bytes]:
    header: _Header | None = None
    compressed = bytearray()
    position = len(_SIGNATURE)
    while True:
        kind, body, position = _next_chunk(data, position)
        if kind == b"IHDR":
            if header is not None:
                raise ImageError("the file has more than one header")
            header = _parse_header(body)
        elif header is None:
            raise ImageError("the file must start with its header")
        elif kind == b"IDAT":
            compressed += body
        elif kind == b"IEND":
            break
        elif kind[:1].isupper() and kind != b"PLTE":
            raise ImageError(f"unknown required chunk '{kind.decode('ascii')}'")
    if position != len(data):
        raise ImageError("the file has data after the end of the image")
    if not compressed:
        raise ImageError("the file has no pixel data")
    return header, bytes(compressed)


def _next_chunk(data: bytes, position: int) -> tuple[bytes, bytes, int]:
    body_start = position + 8
    if body_start > len(data):
        raise ImageError("the file was cut short")
    length, kind = struct.unpack_from(">I4s", data, position)
    body_end = body_start + length
    if body_end + 4 > len(data):
        raise ImageError("the file was cut short")
    if not kind.isalpha():
        raise ImageError("the file has an invalid chunk name")
    body = data[body_start:body_end]
    if zlib.crc32(kind + body) != struct.unpack_from(">I", data, body_end)[0]:
        raise ImageError(f"the chunk '{kind.decode('ascii')}' is damaged")
    return kind, body, body_end + 4


def _parse_header(body: bytes) -> _Header:
    if len(body) != _HEADER_SIZE:
        raise ImageError("the header has the wrong size")
    width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(">IIBBBBB", body)
    if width == 0 or height == 0:
        raise ImageError("the image has no area")
    if width * height > MAX_PIXELS:
        raise ImageError("the image is too large")
    if bit_depth != 8 or color_type not in _CHANNELS_OF_COLOR_TYPE:
        raise ImageError("unsupported kind of image: only 8-bit RGB and RGBA are read")
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise ImageError("unsupported way of storing the image")
    return _Header(width, height, _CHANNELS_OF_COLOR_TYPE[color_type])


def _unpack(compressed: bytes, expected_size: int) -> bytes:
    """Stops right after the expected size, so a small file cannot unpack into a huge one."""
    unpacker = zlib.decompressobj()
    try:
        unpacked = unpacker.decompress(compressed, expected_size + 1)
    except zlib.error as error:
        raise ImageError(f"the pixel data could not be unpacked: {error}") from error
    if len(unpacked) > expected_size or unpacker.unused_data:
        raise ImageError("the file has too much pixel data")
    if len(unpacked) < expected_size or not unpacker.eof:
        raise ImageError("the file has too little pixel data")
    return unpacked


def _undo_filters(filtered: bytes, header: _Header) -> bytes:
    row_size = header.width * header.channels
    previous = bytearray(row_size)
    pixels = bytearray()
    for row_start in range(0, len(filtered), row_size + 1):
        undo = _ROW_FILTERS.get(filtered[row_start])
        if undo is None:
            raise ImageError(f"unknown row filter {filtered[row_start]}")
        row = bytearray(filtered[row_start + 1:row_start + 1 + row_size])
        undo(row, previous, header.channels)
        pixels += row
        previous = row
    return bytes(pixels)


def _undo_none(_row: bytearray, _previous: bytearray, _channels: int) -> None:
    """The row is stored as it is."""


def _undo_sub(row: bytearray, _previous: bytearray, channels: int) -> None:
    for index in range(channels, len(row)):
        row[index] = (row[index] + row[index - channels]) & 0xFF


def _undo_up(row: bytearray, previous: bytearray, _channels: int) -> None:
    for index, above in enumerate(previous):
        row[index] = (row[index] + above) & 0xFF


def _undo_average(row: bytearray, previous: bytearray, channels: int) -> None:
    for index, above in enumerate(previous):
        left = row[index - channels] if index >= channels else 0
        row[index] = (row[index] + (left + above) // 2) & 0xFF


def _undo_paeth(row: bytearray, previous: bytearray, channels: int) -> None:
    for index, above in enumerate(previous):
        left = row[index - channels] if index >= channels else 0
        above_left = previous[index - channels] if index >= channels else 0
        row[index] = (row[index] + _closest_neighbor(left, above, above_left)) & 0xFF


def _closest_neighbor(left: int, above: int, above_left: int) -> int:
    estimate = left + above - above_left
    left_distance = abs(estimate - left)
    above_distance = abs(estimate - above)
    above_left_distance = abs(estimate - above_left)
    if left_distance <= above_distance and left_distance <= above_left_distance:
        return left
    return above if above_distance <= above_left_distance else above_left


_ROW_FILTERS: dict[int, Callable[[bytearray, bytearray, int], None]] = {
    0: _undo_none,
    1: _undo_sub,
    2: _undo_up,
    3: _undo_average,
    4: _undo_paeth,
}


def _to_rgba(pixels: bytes, channels: int) -> bytes:
    if channels == 4:
        return pixels
    rgba = bytearray(b"\xff") * (len(pixels) // 3 * 4)
    rgba[0::4] = pixels[0::3]
    rgba[1::4] = pixels[1::3]
    rgba[2::4] = pixels[2::3]
    return bytes(rgba)
