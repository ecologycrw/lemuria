import struct
import tempfile
import tracemalloc
import unittest
import zlib
from pathlib import Path
from unittest import mock

import support  # noqa: F401

from lemuria_checks import png_reader
from lemuria_checks.png_reader import ImageError, read_png, read_png_file

SIGNATURE = b"\x89PNG\r\n\x1a\n"
RGB = 2
RGBA = 6

NONE = 0
SUB = 1
UP = 2
AVERAGE = 3
PAETH = 4

# A red pixel and a blue pixel, written byte by byte so the test does not depend on the code it checks.
FIXED_PNG = bytes.fromhex(
    "89504e470d0a1a0a0000000d49484452000000020000000108020000007b40e8dd"
    "0000000d4944415478da63f8cf0004ff01070001ff3d7d8c490000000049454e44ae426082"
)


def chunk(kind: bytes, body: bytes) -> bytes:
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))


def header(width: int = 1, height: int = 1, bit_depth: int = 8, color_type: int = RGB, compression: int = 0,
           filter_method: int = 0, interlace: int = 0) -> bytes:
    body = struct.pack(">IIBBBBB", width, height, bit_depth, color_type, compression, filter_method, interlace)
    return chunk(b"IHDR", body)


def pixel_data(raw: bytes) -> bytes:
    return chunk(b"IDAT", zlib.compress(raw))


END = chunk(b"IEND", b"")
ONE_PIXEL = pixel_data(bytes([NONE, 1, 2, 3]))


def png(*chunks: bytes) -> bytes:
    return SIGNATURE + b"".join(chunks)


def two_rows(first: list[int], filter_type: int, second: list[int]) -> bytes:
    """A 2 by 2 image whose first row is stored as it is and whose second row uses the given filter."""
    raw = bytes([NONE, *first, filter_type, *second])
    return png(header(2, 2), pixel_data(raw), END)


def second_row(image_bytes: bytes) -> list[int]:
    pixels = read_png(image_bytes).pixels
    return [pixels[index] for index in (8, 9, 10, 12, 13, 14)]


class ReadingTests(unittest.TestCase):
    def test_a_file_with_known_bytes_is_read(self) -> None:
        image = read_png(FIXED_PNG)

        self.assertEqual((image.width, image.height), (2, 1))
        self.assertEqual(image.pixels, bytes([255, 0, 0, 255, 0, 0, 255, 255]))

    def test_an_image_without_alpha_becomes_opaque(self) -> None:
        image = read_png(png(header(), pixel_data(bytes([NONE, 10, 20, 30])), END))

        self.assertEqual(image.pixels, bytes([10, 20, 30, 255]))

    def test_an_image_with_alpha_keeps_it(self) -> None:
        image = read_png(png(header(color_type=RGBA), pixel_data(bytes([NONE, 10, 20, 30, 40])), END))

        self.assertEqual(image.pixels, bytes([10, 20, 30, 40]))

    def test_pixel_data_can_be_split_in_several_chunks(self) -> None:
        compressed = zlib.compress(bytes([NONE, 10, 20, 30]))
        parts = chunk(b"IDAT", compressed[:3]), chunk(b"IDAT", compressed[3:])

        image = read_png(png(header(), *parts, END))

        self.assertEqual(image.pixels, bytes([10, 20, 30, 255]))

    def test_optional_chunks_are_skipped(self) -> None:
        image = read_png(png(header(), chunk(b"tEXt", b"note"), ONE_PIXEL, chunk(b"tIME", b"later"), END))

        self.assertEqual(image.pixels, bytes([1, 2, 3, 255]))

    def test_a_suggested_palette_is_skipped(self) -> None:
        image = read_png(png(header(), chunk(b"PLTE", bytes(3)), ONE_PIXEL, END))

        self.assertEqual(image.pixels, bytes([1, 2, 3, 255]))


class FilterTests(unittest.TestCase):
    def test_sub_adds_the_pixel_on_the_left(self) -> None:
        data = two_rows([10, 20, 30, 40, 51, 60], SUB, [1, 2, 3, 5, 5, 5])
        self.assertEqual(second_row(data), [1, 2, 3, 6, 7, 8])

    def test_sub_wraps_around_at_256(self) -> None:
        data = two_rows([0] * 6, SUB, [250, 2, 3, 10, 5, 5])
        self.assertEqual(second_row(data), [250, 2, 3, 4, 7, 8])

    def test_sub_looks_four_bytes_back_when_there_is_alpha(self) -> None:
        raw = bytes([SUB, 1, 2, 3, 4, 5, 5, 5, 5])
        image = read_png(png(header(2, 1, color_type=RGBA), pixel_data(raw), END))
        self.assertEqual(image.pixels, bytes([1, 2, 3, 4, 6, 7, 8, 9]))

    def test_up_adds_the_pixel_above(self) -> None:
        data = two_rows([10, 20, 30, 40, 51, 60], UP, [1, 2, 3, 5, 5, 5])
        self.assertEqual(second_row(data), [11, 22, 33, 45, 56, 65])

    def test_average_adds_the_rounded_down_mean_of_left_and_above(self) -> None:
        data = two_rows([10, 20, 30, 40, 51, 60], AVERAGE, [1, 2, 3, 5, 5, 5])
        self.assertEqual(second_row(data), [6, 12, 18, 28, 36, 44])

    def test_paeth_picks_the_closest_neighbor(self) -> None:
        data = two_rows([10, 100, 30, 12, 70, 200], PAETH, [100, 30, 3, 5, 5, 5])
        self.assertEqual(second_row(data), [110, 130, 33, 115, 105, 205])

    def test_paeth_prefers_left_then_above_when_there_is_a_tie(self) -> None:
        data = two_rows([50, 50, 0, 60, 30, 0], PAETH, [236, 10, 0, 1, 1, 0])
        self.assertEqual(second_row(data), [30, 60, 0, 31, 31, 0])

    def test_filters_on_the_first_row_see_zeros_above(self) -> None:
        cases = {UP: [4, 8, 12, 4, 8, 12], AVERAGE: [4, 8, 12, 6, 12, 18], PAETH: [4, 8, 12, 8, 16, 24]}
        for filter_type, expected in cases.items():
            with self.subTest(filter_type=filter_type):
                raw = bytes([filter_type, 4, 8, 12, 4, 8, 12])
                pixels = read_png(png(header(2, 1), pixel_data(raw), END)).pixels
                self.assertEqual(list(pixels[:3]) + list(pixels[4:7]), expected)


class RefusalTests(unittest.TestCase):
    def assert_refused(self, data: bytes, expected: str) -> None:
        with self.assertRaises(ImageError) as caught:
            read_png(data)
        self.assertIn(expected, str(caught.exception))

    def test_a_file_that_is_not_a_png_is_refused(self) -> None:
        self.assert_refused(b"GIF89a and more bytes", "not a PNG")
        self.assert_refused(b"", "not a PNG")

    def test_a_cut_file_is_refused(self) -> None:
        whole = png(header(), ONE_PIXEL, END)
        for size in (len(SIGNATURE) + 3, len(SIGNATURE) + 10, len(whole) - 1):
            with self.subTest(size=size):
                self.assert_refused(whole[:size], "cut short")

    def test_a_chunk_that_claims_more_bytes_than_the_file_has_is_refused(self) -> None:
        huge = struct.pack(">I4s", 0xFFFFFFFF, b"IDAT")
        self.assert_refused(png(header(), huge), "cut short")

    def test_a_damaged_chunk_is_refused(self) -> None:
        damaged = bytearray(png(header(), ONE_PIXEL, END))
        damaged[len(SIGNATURE) + 10] ^= 0xFF
        self.assert_refused(bytes(damaged), "damaged")

    def test_a_chunk_with_an_invalid_name_is_refused(self) -> None:
        self.assert_refused(png(header(), chunk(b"ID\x00T", b""), END), "invalid chunk name")

    def test_the_header_must_come_first_and_only_once(self) -> None:
        self.assert_refused(png(ONE_PIXEL, header(), END), "must start with")
        self.assert_refused(png(header(), header(), ONE_PIXEL, END), "more than one header")

    def test_a_file_without_an_end_is_refused(self) -> None:
        self.assert_refused(png(header(), ONE_PIXEL), "cut short")

    def test_data_after_the_end_is_refused(self) -> None:
        self.assert_refused(png(header(), ONE_PIXEL, END, b"extra"), "after the end")

    def test_a_file_without_pixels_is_refused(self) -> None:
        self.assert_refused(png(header(), END), "no pixel data")

    def test_an_unknown_required_chunk_is_refused(self) -> None:
        self.assert_refused(png(header(), chunk(b"ABCD", b""), ONE_PIXEL, END), "unknown required chunk")

    def test_a_header_of_the_wrong_size_is_refused(self) -> None:
        self.assert_refused(png(chunk(b"IHDR", bytes(12)), ONE_PIXEL, END), "header")

    def test_an_image_without_area_is_refused(self) -> None:
        self.assert_refused(png(header(width=0), ONE_PIXEL, END), "no area")
        self.assert_refused(png(header(height=0), ONE_PIXEL, END), "no area")

    def test_an_image_above_the_budget_is_refused(self) -> None:
        self.assert_refused(png(header(width=png_reader.MAX_PIXELS + 1), ONE_PIXEL, END), "too large")
        self.assert_refused(png(header(width=0xFFFFFFFF, height=0xFFFFFFFF), ONE_PIXEL, END), "too large")

    def test_unsupported_kinds_of_image_are_refused(self) -> None:
        cases = {
            "16 bits": header(bit_depth=16),
            "grayscale": header(color_type=0),
            "palette": header(color_type=3),
            "grayscale with alpha": header(color_type=4),
            "compression": header(compression=1),
            "filter method": header(filter_method=1),
            "interlaced": header(interlace=1),
        }
        for name, unsupported in cases.items():
            with self.subTest(name=name):
                self.assert_refused(png(unsupported, ONE_PIXEL, END), "unsupported")

    def test_an_unknown_row_filter_is_refused(self) -> None:
        self.assert_refused(png(header(), pixel_data(bytes([5, 1, 2, 3])), END), "unknown row filter")

    def test_too_little_pixel_data_is_refused(self) -> None:
        self.assert_refused(png(header(), pixel_data(bytes([NONE, 1, 2])), END), "too little pixel data")

    def test_too_much_pixel_data_is_refused(self) -> None:
        self.assert_refused(png(header(), pixel_data(bytes(1_000_000)), END), "too much pixel data")

    def test_a_small_file_cannot_unpack_into_a_huge_amount_of_memory(self) -> None:
        small_file = png(header(), pixel_data(bytes(16_000_000)), END)

        tracemalloc.start()
        self.addCleanup(tracemalloc.stop)
        self.assert_refused(small_file, "too much pixel data")
        _, peak = tracemalloc.get_traced_memory()

        self.assertLess(peak, 1_000_000)

    def test_pixel_data_that_cannot_be_unpacked_is_refused(self) -> None:
        self.assert_refused(png(header(), chunk(b"IDAT", b"not compressed"), END), "could not be unpacked")

    def test_an_unfinished_compressed_stream_is_refused(self) -> None:
        compressed = zlib.compress(bytes([NONE, 1, 2, 3]))
        self.assert_refused(png(header(), chunk(b"IDAT", compressed[:-4]), END), "too little pixel data")

    def test_bytes_after_the_compressed_stream_are_refused(self) -> None:
        compressed = zlib.compress(bytes([NONE, 1, 2, 3])) + b"extra"
        self.assert_refused(png(header(), chunk(b"IDAT", compressed), END), "too much pixel data")

    def test_a_file_above_the_budget_is_refused(self) -> None:
        whole = png(header(), ONE_PIXEL, END)
        with mock.patch.object(png_reader, "MAX_FILE_BYTES", len(whole) - 1):
            self.assert_refused(whole, "too large")


class FileTests(unittest.TestCase):
    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.folder = Path(folder.name)

    def test_a_file_is_read_from_the_disk(self) -> None:
        path = self.folder / "image.png"
        path.write_bytes(FIXED_PNG)

        self.assertEqual(read_png_file(path).width, 2)

    def test_a_missing_file_is_refused(self) -> None:
        with self.assertRaises(ImageError) as caught:
            read_png_file(self.folder / "missing.png")
        self.assertIn("could not read", str(caught.exception))

    def test_a_file_above_the_budget_is_not_loaded(self) -> None:
        path = self.folder / "image.png"
        path.write_bytes(FIXED_PNG)

        with mock.patch.object(png_reader, "MAX_FILE_BYTES", 4), mock.patch.object(Path, "read_bytes") as read_bytes:
            with self.assertRaises(ImageError):
                read_png_file(path)
        read_bytes.assert_not_called()


if __name__ == "__main__":
    unittest.main()
