"""Tests of the numbers required by CA-20 of slice 0."""

import contextlib
import dataclasses
import io
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

import support  # noqa: F401

from lemuria_checks import cli, image_measures
from lemuria_checks.image_measures import ImageMeasures, build_report, measure
from lemuria_checks.png_reader import ImageError, RgbaImage

Color = tuple[int, int, int]

# 20% deep shadow, 85% dark and every pixel warm: inside all the required ranges.
GOOD_COLORS: list[Color] = [(40, 20, 10)] * 20 + [(60, 40, 20)] * 65 + [(120, 100, 50)] * 15
GOOD_MEASURES = ImageMeasures(
    median_brightness=0.15, deep_shadow_share=0.20, dark_share=0.85, highlight_share=0.005, warm_share=0.90,
    median_saturation=0.65,
)


def image_of(colors: list[Color], alpha: int = 255) -> RgbaImage:
    pixels = bytes(channel for color in colors for channel in (*color, alpha))
    return RgbaImage(len(colors), 1, pixels)


def gray(level: int) -> Color:
    return level, level, level


def png_file(colors: list[Color]) -> bytes:
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    header = struct.pack(">IIBBBBB", len(colors), 1, 8, 2, 0, 0, 0)
    row = bytes([0]) + bytes(channel for color in colors for channel in color)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(row)) + chunk(b"IEND", b"")


class BrightnessTests(unittest.TestCase):
    def test_each_channel_has_its_weight(self) -> None:
        cases = {(255, 0, 0): 0.2126, (0, 255, 0): 0.7152, (0, 0, 255): 0.0722, (255, 255, 255): 1.0}
        for color, expected in cases.items():
            with self.subTest(color=color):
                self.assertAlmostEqual(measure(image_of([color])).median_brightness, expected)

    def test_the_median_is_the_middle_value(self) -> None:
        measures = measure(image_of([gray(200), gray(10), gray(20)]))
        self.assertAlmostEqual(measures.median_brightness, 20 / 255)

    def test_the_median_of_an_even_count_is_the_mean_of_the_two_middle_values(self) -> None:
        measures = measure(image_of([gray(0), gray(0), gray(255), gray(255)]))
        self.assertAlmostEqual(measures.median_brightness, 0.5)

    def test_repeated_colors_count_once_per_pixel(self) -> None:
        measures = measure(image_of([gray(10)] * 3 + [gray(200)] * 2))
        self.assertAlmostEqual(measures.median_brightness, 10 / 255)


class ShareTests(unittest.TestCase):
    def test_deep_shadow_is_below_a_tenth(self) -> None:
        self.assertEqual(measure(image_of([gray(25)])).deep_shadow_share, 1.0)
        self.assertEqual(measure(image_of([gray(26)])).deep_shadow_share, 0.0)

    def test_dark_is_below_a_quarter(self) -> None:
        self.assertEqual(measure(image_of([gray(63)])).dark_share, 1.0)
        self.assertEqual(measure(image_of([gray(64)])).dark_share, 0.0)

    def test_a_point_of_light_is_above_six_tenths(self) -> None:
        self.assertEqual(measure(image_of([gray(153)])).highlight_share, 0.0)
        self.assertEqual(measure(image_of([gray(154)])).highlight_share, 1.0)

    def test_a_warm_color_has_red_at_least_one_and_a_quarter_times_the_blue(self) -> None:
        self.assertEqual(measure(image_of([(125, 0, 100)])).warm_share, 1.0)
        self.assertEqual(measure(image_of([(124, 0, 100)])).warm_share, 0.0)

    def test_black_follows_the_same_rule_and_counts_as_warm(self) -> None:
        self.assertEqual(measure(image_of([(0, 0, 0)])).warm_share, 1.0)

    def test_a_share_is_the_part_of_all_pixels(self) -> None:
        measures = measure(image_of([gray(0)] * 3 + [gray(255)]))
        self.assertEqual(measures.deep_shadow_share, 0.75)


class SaturationTests(unittest.TestCase):
    def test_saturation_compares_the_largest_and_smallest_channels(self) -> None:
        self.assertAlmostEqual(measure(image_of([(200, 100, 50)])).median_saturation, 0.75)

    def test_black_and_gray_have_no_saturation(self) -> None:
        self.assertEqual(measure(image_of([(0, 0, 0)])).median_saturation, 0.0)
        self.assertEqual(measure(image_of([gray(90)])).median_saturation, 0.0)

    def test_the_median_is_used(self) -> None:
        colors = [(100, 100, 100), (100, 50, 50), (100, 0, 0)]
        self.assertAlmostEqual(measure(image_of(colors)).median_saturation, 0.5)


class ImageRulesTests(unittest.TestCase):
    def test_an_image_with_transparency_is_refused(self) -> None:
        with self.assertRaises(ImageError) as caught:
            measure(image_of([gray(10)], alpha=254))
        self.assertIn("opaque", str(caught.exception))


class RangeTests(unittest.TestCase):
    def test_the_ranges_are_the_ones_the_slice_requires(self) -> None:
        ranges = {rule.attribute: (rule.low, rule.high) for rule in image_measures.RULES}

        self.assertEqual(ranges, {
            "median_brightness": (0.11, 0.19),
            "deep_shadow_share": (0.12, 0.40),
            "dark_share": (0.78, 0.93),
            "warm_share": (0.75, 1.0),
            "median_saturation": (0.50, 0.80),
            "highlight_share": (None, None),
        })

    def test_the_limits_of_each_range_are_accepted(self) -> None:
        for rule in image_measures.RULES:
            if rule.low is None:
                continue
            for value in (rule.low, rule.high):
                with self.subTest(rule=rule.attribute, value=value):
                    measures = dataclasses.replace(GOOD_MEASURES, **{rule.attribute: value})
                    self.assertTrue(build_report("image.png", measures)[1])

    def test_a_value_outside_any_range_is_refused(self) -> None:
        for rule in image_measures.RULES:
            if rule.low is None:
                continue
            for value in (rule.low - 0.001, rule.high + 0.001):
                with self.subTest(rule=rule.attribute, value=value):
                    measures = dataclasses.replace(GOOD_MEASURES, **{rule.attribute: value})
                    self.assertFalse(build_report("image.png", measures)[1])

    def test_points_of_light_are_not_required_in_this_slice(self) -> None:
        for value in (0.0, 1.0):
            measures = dataclasses.replace(GOOD_MEASURES, highlight_share=value)
            self.assertTrue(build_report("image.png", measures)[1])


class ReportTests(unittest.TestCase):
    def test_the_report_shows_each_number_with_its_range(self) -> None:
        report, passed = build_report("image.png", GOOD_MEASURES)

        self.assertTrue(passed)
        self.assertIn("image.png", report)
        self.assertIn("OK    Brilho típico da imagem (mediana): 0,150 (faixa: 0,11 a 0,19)", report)
        self.assertIn("OK    Parte da imagem em sombra profunda: 20,00% (faixa: 12% a 40%)", report)
        self.assertIn("OK    Parte da imagem em tons quentes: 90,00% (faixa: 75% ou mais)", report)
        self.assertIn("--    Parte da imagem em pontos de luz: 0,50% (não exigida nesta fatia)", report)
        self.assertIn("dentro das faixas", report)

    def test_the_report_points_out_what_is_outside_the_range(self) -> None:
        measures = dataclasses.replace(GOOD_MEASURES, dark_share=0.5)

        report, passed = build_report("image.png", measures)

        self.assertFalse(passed)
        self.assertIn("FORA  Parte da imagem em tons escuros: 50,00% (faixa: 78% a 93%)", report)
        self.assertIn("fora das faixas", report)


class CommandTests(unittest.TestCase):
    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.path = Path(folder.name) / "image.png"

    def run_command(self, *arguments: str) -> tuple[int, str]:
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            code = cli.main(["measure-image", *arguments])
        return code, output.getvalue()

    def test_an_image_inside_the_ranges_passes(self) -> None:
        self.path.write_bytes(png_file(GOOD_COLORS))

        code, output = self.run_command("--image", str(self.path))

        self.assertEqual(code, 0, output)
        self.assertIn("dentro das faixas", output)

    def test_an_image_outside_the_ranges_fails(self) -> None:
        self.path.write_bytes(png_file([gray(255)] * 4))

        code, output = self.run_command("--image", str(self.path))

        self.assertEqual(code, 1)
        self.assertIn("FORA", output)

    def test_a_blank_black_image_fails(self) -> None:
        self.path.write_bytes(png_file([gray(0)] * 4))

        code, _ = self.run_command("--image", str(self.path))

        self.assertEqual(code, 1)

    def test_a_damaged_file_fails_with_a_clear_message(self) -> None:
        self.path.write_bytes(b"this is not an image")

        code, output = self.run_command("--image", str(self.path))

        self.assertEqual(code, 1)
        self.assertIn("not a PNG", output)

    def test_a_missing_file_fails_with_a_clear_message(self) -> None:
        code, output = self.run_command("--image", str(self.path))

        self.assertEqual(code, 1)
        self.assertIn("could not read", output)

    def test_the_command_needs_an_image(self) -> None:
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as caught:
            self.run_command()

        self.assertEqual(caught.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
