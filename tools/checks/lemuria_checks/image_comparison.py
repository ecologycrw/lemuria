"""Compares a final image with the image the owner approved (docs/VISUAL.md).

The report is in Portuguese because the owner of the product reads it.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .png_reader import ImageError, RgbaImage, read_png_file

APPROVED_IMAGE = "tests/visual/approved/proof_scene.png"

# Measured between renderers and against small real changes (docs/adr/0021-prova-visual-e-tolerancia.md).
CHANNEL_TOLERANCE = 2
CHANGED_PIXELS_TOLERANCE = 0.0001

_BYTES_PER_PIXEL = 4


@dataclass(frozen=True)
class ImageDifference:
    changed_pixels: int
    total_pixels: int
    largest_channel_difference: int

    @property
    def changed_share(self) -> float:
        return self.changed_pixels / self.total_pixels

    @property
    def accepted(self) -> bool:
        return self.changed_share <= CHANGED_PIXELS_TOLERANCE


def compare(image: RgbaImage, reference: RgbaImage) -> ImageDifference:
    if (image.width, image.height) != (reference.width, reference.height):
        raise ImageError(
            f"the images have different sizes: {image.width} x {image.height} and {reference.width} x {reference.height}"
        )
    changed = 0
    largest = 0
    for offset in range(0, len(image.pixels), _BYTES_PER_PIXEL):
        pixel = image.pixels[offset:offset + _BYTES_PER_PIXEL]
        reference_pixel = reference.pixels[offset:offset + _BYTES_PER_PIXEL]
        if pixel == reference_pixel:
            continue
        difference = max(abs(first - second) for first, second in zip(pixel, reference_pixel))
        largest = max(largest, difference)
        changed += difference > CHANNEL_TOLERANCE
    return ImageDifference(changed, image.width * image.height, largest)


def _percent(share: float) -> str:
    return f"{share * 100:.3f}%".replace(".", ",")


def build_report(reference_name: str, difference: ImageDifference) -> tuple[str, bool]:
    """Returns the text of the report and whether the image is within the tolerance."""
    result = "a imagem está dentro da tolerância" if difference.accepted else "a imagem mudou além da tolerância"
    lines = [
        f"Comparação com a imagem de referência: {reference_name}",
        f"  Pontos diferentes: {difference.changed_pixels} de {difference.total_pixels} "
        f"({_percent(difference.changed_share)}; limite: {_percent(CHANGED_PIXELS_TOLERANCE)})",
        f"  Maior diferença de cor: {difference.largest_channel_difference} de 255 "
        f"(tolerada: até {CHANNEL_TOLERANCE} por canal)",
        f"Resultado: {result}.",
    ]
    return "\n".join(lines) + "\n", difference.accepted


def run(image_path: Path, reference_path: Path) -> int:
    try:
        image = read_png_file(image_path)
        if not reference_path.exists():
            print(f"Aviso: ainda não há imagem aprovada em {reference_path}. A comparação foi pulada.")
            return 0
        difference = compare(image, read_png_file(reference_path))
    except ImageError as error:
        print(f"The images could not be compared: {error}")
        return 1
    report, accepted = build_report(str(reference_path), difference)
    print(report, end="")
    return 0 if accepted else 1
