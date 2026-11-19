"""Tests of the comparison required by CA-21 of slice 0."""

import contextlib
import io
import struct
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest import mock

import support  # noqa: F401

from lemuria_checks import cli, image_comparison
from lemuria_checks.image_comparison import ImageDifference, build_report, compare
from lemuria_checks.png_reader import ImageError, RgbaImage

Pixel = tuple[int, int, int, int]

BASE: Pixel = (100, 100, 100, 255)


def image_of(pixels: list[Pixel], width: int | None = None) -> RgbaImage:
    columns = width if width is not None else len(pixels)
    return RgbaImage(columns, len(pixels) // columns, bytes(channel for pixel in pixels for channel in pixel))


def png_file(pixels: list[Pixel]) -> bytes:
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    header = struct.pack(">IIBBBBB", len(pixels), 1, 8, 6, 0, 0, 0)
    row = bytes([0]) + bytes(channel for pixel in pixels for channel in pixel)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(row)) + chunk(b"IEND", b"")


class ToleranceTestCase(unittest.TestCase):
    """Fixes the tolerances, so these tests do not change when the measured values change."""

    def setUp(self) -> None:
        for name, value in (("CHANNEL_TOLERANCE", 2), ("CHANGED_PIXELS_TOLERANCE", 0.01)):
            patcher = mock.patch.object(image_comparison, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)


class MeasuredToleranceTests(unittest.TestCase):
    """The numbers come from docs/adr/0021-prova-visual-e-tolerancia.md."""

    PROOF_IMAGE_PIXELS = 1280 * 720

    def test_the_tolerances_are_the_ones_of_the_decision_record(self) -> None:
        self.assertEqual(image_comparison.CHANNEL_TOLERANCE, 2)
        self.assertEqual(image_comparison.CHANGED_PIXELS_TOLERANCE, 0.0001)

    def test_the_approved_image_has_a_fixed_place(self) -> None:
        self.assertEqual(image_comparison.APPROVED_IMAGE, "tests/visual/approved/proof_scene.png")

    def test_the_measured_difference_between_renderers_is_accepted(self) -> None:
        self.assertTrue(ImageDifference(17, self.PROOF_IMAGE_PIXELS, 118).accepted)

    def test_the_smallest_measured_real_change_is_refused(self) -> None:
        self.assertFalse(ImageDifference(832, self.PROOF_IMAGE_PIXELS, 200).accepted)


class ComparisonTests(ToleranceTestCase):
    def test_identical_images_have_no_difference(self) -> None:
        difference = compare(image_of([BASE] * 4), image_of([BASE] * 4))

        self.assertEqual(difference, ImageDifference(0, 4, 0))
        self.assertTrue(difference.accepted)

    def test_a_difference_within_the_channel_tolerance_is_not_a_change(self) -> None:
        difference = compare(image_of([(102, 100, 100, 255)]), image_of([BASE]))

        self.assertEqual(difference.changed_pixels, 0)
        self.assertEqual(difference.largest_channel_difference, 2)

    def test_a_difference_above_the_channel_tolerance_is_a_change(self) -> None:
        difference = compare(image_of([(103, 100, 100, 255)]), image_of([BASE]))

        self.assertEqual(difference.changed_pixels, 1)
        self.assertEqual(difference.largest_channel_difference, 3)

    def test_a_darker_pixel_counts_like_a_brighter_one(self) -> None:
        self.assertEqual(compare(image_of([(97, 100, 100, 255)]), image_of([BASE])).changed_pixels, 1)

    def test_every_channel_is_compared(self) -> None:
        for channel in range(4):
            with self.subTest(channel=channel):
                altered = list(BASE)
                altered[channel] -= 3
                changed = compare(image_of([tuple(altered)]), image_of([BASE])).changed_pixels
                self.assertEqual(changed, 1)

    def test_a_pixel_counts_once_even_with_several_channels_changed(self) -> None:
        difference = compare(image_of([(110, 120, 130, 200)]), image_of([BASE]))

        self.assertEqual(difference.changed_pixels, 1)
        self.assertEqual(difference.largest_channel_difference, 55)

    def test_only_the_pixels_that_changed_are_counted(self) -> None:
        altered = [BASE, BASE, BASE, (200, 100, 100, 255)]

        difference = compare(image_of(altered), image_of([BASE] * 4))

        self.assertEqual(difference, ImageDifference(1, 4, 100))

    def test_the_largest_difference_is_kept_when_smaller_ones_come_later(self) -> None:
        altered = [(200, 100, 100, 255), (105, 100, 100, 255)]

        difference = compare(image_of(altered), image_of([BASE] * 2))

        self.assertEqual(difference.largest_channel_difference, 100)

    def test_the_limit_of_changed_pixels_is_accepted(self) -> None:
        altered = [(200, 100, 100, 255)] + [BASE] * 99

        self.assertTrue(compare(image_of(altered), image_of([BASE] * 100)).accepted)

    def test_more_changed_pixels_than_the_limit_are_refused(self) -> None:
        altered = [(200, 100, 100, 255)] * 2 + [BASE] * 98

        self.assertFalse(compare(image_of(altered), image_of([BASE] * 100)).accepted)

    def test_images_of_different_sizes_are_refused(self) -> None:
        cases = {"width": (image_of([BASE] * 4, 4), image_of([BASE] * 2, 2)),
                 "shape": (image_of([BASE] * 4, 4), image_of([BASE] * 4, 2))}
        for name, (first, second) in cases.items():
            with self.subTest(name=name), self.assertRaises(ImageError) as caught:
                compare(first, second)
            self.assertIn("different sizes", str(caught.exception))


class ReportTests(ToleranceTestCase):
    def test_the_report_of_an_accepted_image(self) -> None:
        report, accepted = build_report("approved.png", ImageDifference(1, 100, 2))

        self.assertTrue(accepted)
        self.assertIn("approved.png", report)
        self.assertIn("Pontos diferentes: 1 de 100 (1,000%; limite: 1,000%)", report)
        self.assertIn("Maior diferença de cor: 2 de 255 (tolerada: até 2 por canal)", report)
        self.assertIn("dentro da tolerância", report)

    def test_the_report_of_a_refused_image(self) -> None:
        report, accepted = build_report("approved.png", ImageDifference(50, 100, 90))

        self.assertFalse(accepted)
        self.assertIn("além da tolerância", report)


class CommandTests(ToleranceTestCase):
    def setUp(self) -> None:
        super().setUp()
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name)
        self.image = self.root / "image.png"
        self.reference = self.root / "reference.png"

    def run_command(self, *arguments: str) -> tuple[int, str]:
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            code = cli.main(["compare-image", "--root", str(self.root), *arguments])
        return code, output.getvalue()

    def compare_files(self) -> tuple[int, str]:
        return self.run_command("--image", str(self.image), "--reference", str(self.reference))

    def test_the_approved_image_is_accepted(self) -> None:
        self.image.write_bytes(png_file([BASE] * 4))
        self.reference.write_bytes(png_file([BASE] * 4))

        code, output = self.compare_files()

        self.assertEqual(code, 0, output)
        self.assertIn("dentro da tolerância", output)

    def test_an_altered_image_is_refused(self) -> None:
        self.image.write_bytes(png_file([(200, 0, 0, 255)] * 4))
        self.reference.write_bytes(png_file([BASE] * 4))

        code, output = self.compare_files()

        self.assertEqual(code, 1)
        self.assertIn("além da tolerância", output)

    def test_without_an_approved_image_it_warns_and_does_not_fail(self) -> None:
        self.image.write_bytes(png_file([BASE] * 4))

        code, output = self.compare_files()

        self.assertEqual(code, 0, output)
        self.assertIn("Aviso: ainda não há imagem aprovada", output)

    def test_the_approved_folder_is_the_default_reference(self) -> None:
        self.image.write_bytes(png_file([BASE] * 4))
        approved = self.root / image_comparison.APPROVED_IMAGE
        approved.parent.mkdir(parents=True)
        approved.write_bytes(png_file([(200, 0, 0, 255)] * 4))

        code, output = self.run_command("--image", str(self.image))

        self.assertEqual(code, 1)
        self.assertIn(str(approved), output)

    def test_a_missing_image_fails_even_without_an_approved_image(self) -> None:
        code, output = self.compare_files()

        self.assertEqual(code, 1)
        self.assertIn("could not read", output)

    def test_a_damaged_approved_image_fails_instead_of_being_skipped(self) -> None:
        self.image.write_bytes(png_file([BASE] * 4))
        self.reference.write_bytes(b"this is not an image")

        code, output = self.compare_files()

        self.assertEqual(code, 1)
        self.assertIn("not a PNG", output)

    def test_images_of_different_sizes_fail(self) -> None:
        self.image.write_bytes(png_file([BASE] * 4))
        self.reference.write_bytes(png_file([BASE] * 2))

        code, output = self.compare_files()

        self.assertEqual(code, 1)
        self.assertIn("different sizes", output)

    def test_the_command_needs_an_image(self) -> None:
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as caught:
            self.run_command()

        self.assertEqual(caught.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
