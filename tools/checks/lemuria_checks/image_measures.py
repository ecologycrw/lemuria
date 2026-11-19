"""Measures a final image against the numbers of docs/VISUAL.md.

The report is in Portuguese because the owner of the product reads it.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

from .png_reader import ImageError, RgbaImage, read_png_file

Color = tuple[int, int, int]

# Brightness is kept as a whole number, so a color on a limit never lands on the wrong side by rounding.
_RED_WEIGHT = 2126
_GREEN_WEIGHT = 7152
_BLUE_WEIGHT = 722
_FULL_BRIGHTNESS = 10_000 * 255
_DEEP_SHADOW_LIMIT = _FULL_BRIGHTNESS // 10
_DARK_LIMIT = _FULL_BRIGHTNESS // 4
_POINT_OF_LIGHT_LIMIT = _FULL_BRIGHTNESS * 3 // 5


@dataclass(frozen=True)
class ImageMeasures:
    median_brightness: float
    deep_shadow_share: float
    dark_share: float
    highlight_share: float
    warm_share: float
    median_saturation: float


@dataclass(frozen=True)
class MeasureRule:
    """A number of the report. Without limits, the number is shown but not required."""

    attribute: str
    label: str
    is_share: bool
    low: float | None = None
    high: float | None = None

    def accepts(self, value: float) -> bool:
        return self.low is None or self.high is None or self.low <= value <= self.high


# The ranges of slice 0 (specs/0000-primeiros-passos/spec.md). The points of light are required from slice 2.
RULES: tuple[MeasureRule, ...] = (
    MeasureRule("median_brightness", "Brilho típico da imagem (mediana)", False, 0.11, 0.19),
    MeasureRule("deep_shadow_share", "Parte da imagem em sombra profunda", True, 0.12, 0.40),
    MeasureRule("dark_share", "Parte da imagem em tons escuros", True, 0.78, 0.93),
    MeasureRule("warm_share", "Parte da imagem em tons quentes", True, 0.75, 1.0),
    MeasureRule("median_saturation", "Saturação mediana", False, 0.50, 0.80),
    MeasureRule("highlight_share", "Parte da imagem em pontos de luz", True),
)


def measure(image: RgbaImage) -> ImageMeasures:
    pixel_count = image.width * image.height
    if image.pixels[3::4].count(255) != pixel_count:
        raise ImageError("the image must be opaque")
    colors = Counter(zip(image.pixels[0::4], image.pixels[1::4], image.pixels[2::4]))

    def share(is_counted: Callable[[Color], bool]) -> float:
        return sum(count for color, count in colors.items() if is_counted(color)) / pixel_count

    return ImageMeasures(
        median_brightness=_median(colors, _brightness) / _FULL_BRIGHTNESS,
        deep_shadow_share=share(lambda color: _brightness(color) < _DEEP_SHADOW_LIMIT),
        dark_share=share(lambda color: _brightness(color) < _DARK_LIMIT),
        highlight_share=share(lambda color: _brightness(color) > _POINT_OF_LIGHT_LIMIT),
        warm_share=share(_is_warm),
        median_saturation=_median(colors, _saturation),
    )


def _brightness(color: Color) -> int:
    return _RED_WEIGHT * color[0] + _GREEN_WEIGHT * color[1] + _BLUE_WEIGHT * color[2]


def _is_warm(color: Color) -> bool:
    return color[0] * 4 >= color[2] * 5


def _saturation(color: Color) -> float:
    largest = max(color)
    return (largest - min(color)) / largest if largest > 0 else 0.0


def _median(colors: Counter[Color], value_of: Callable[[Color], float]) -> float:
    pixels_with_value: Counter[float] = Counter()
    for color, count in colors.items():
        pixels_with_value[value_of(color)] += count

    total = sum(pixels_with_value.values())
    lower_middle = None
    seen = 0
    for value in sorted(pixels_with_value):
        seen += pixels_with_value[value]
        if lower_middle is None and seen > (total - 1) // 2:
            lower_middle = value
        if seen > total // 2:
            return (lower_middle + value) / 2
    raise ImageError("the image has no pixels")


def _decimal(value: float, places: int) -> str:
    return f"{value:.{places}f}".replace(".", ",")


def _range_text(rule: MeasureRule) -> str:
    if not rule.is_share:
        return f"{_decimal(rule.low, 2)} a {_decimal(rule.high, 2)}"
    low = f"{rule.low * 100:.0f}%"
    return f"{low} ou mais" if rule.high == 1.0 else f"{low} a {rule.high * 100:.0f}%"


def _report_line(rule: MeasureRule, value: float) -> str:
    shown = f"{_decimal(value * 100, 2)}%" if rule.is_share else _decimal(value, 3)
    if rule.low is None:
        return f"  {'--':<6}{rule.label}: {shown} (não exigida nesta fatia)"
    status = "OK" if rule.accepts(value) else "FORA"
    return f"  {status:<6}{rule.label}: {shown} (faixa: {_range_text(rule)})"


def build_report(image_name: str, measures: ImageMeasures) -> tuple[str, bool]:
    """Returns the text of the report and whether every required number is inside its range."""
    values = [(rule, getattr(measures, rule.attribute)) for rule in RULES]
    passed = all(rule.accepts(value) for rule, value in values)
    result = "todos os números estão dentro das faixas exigidas" if passed else "há números fora das faixas exigidas"
    lines = [f"Números da imagem: {image_name}", *(_report_line(rule, value) for rule, value in values),
             f"Resultado: {result}."]
    return "\n".join(lines) + "\n", passed


def run(path: Path) -> int:
    try:
        measures = measure(read_png_file(path))
    except ImageError as error:
        print(f"The image could not be measured: {error}")
        return 1
    report, passed = build_report(str(path), measures)
    print(report, end="")
    return 0 if passed else 1
