"""Tests of the visual proof command of tools/dev.py (CA-19, CA-20, CA-21, CA-22 and CA-23 of slice 0)."""

import contextlib
import importlib.util
import io
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import support

_SPECIFICATION = importlib.util.spec_from_file_location("lemuria_dev", support.CHECKS_FOLDER.parent / "dev.py")
dev = importlib.util.module_from_spec(_SPECIFICATION)
sys.modules[_SPECIFICATION.name] = dev
_SPECIFICATION.loader.exec_module(dev)

FAST_FRAMES = "Frame time over 600 frames: average 1.43 ms, worst 1% 2.10 ms (budget 16.6 ms): within budget\n"
SLOW_FRAMES = "Frame time over 600 frames: average 12.00 ms, worst 1% 31.25 ms (budget 16.6 ms): over budget\n"


def finished(code: int = 0, stdout: str = "", stderr: str = "") -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess([], code, stdout, stderr)


class FakeClient:
    """Stands in for the demo client: it writes the image files and answers with the frame times."""

    def __init__(self) -> None:
        self.calls: list[tuple[str, ...]] = []
        self.images = iter(lambda: b"the same image", None)
        self.frame_times = finished(0, FAST_FRAMES)
        self.drawing = finished()

    def __call__(self, *arguments: str) -> subprocess.CompletedProcess:
        self.calls.append(arguments)
        if "--measure-frame-times" in arguments:
            return self.frame_times
        Path(arguments[arguments.index("--proof-image") + 1]).write_bytes(next(self.images))
        return self.drawing

    def apis_drawn(self) -> list[str]:
        return [call[call.index("--graphics-api") + 1] for call in self.calls if "--proof-image" in call]

    def apis_measured(self) -> list[str]:
        return [call[call.index("--graphics-api") + 1] for call in self.calls if "--measure-frame-times" in call]


class VisualProofTestCase(unittest.TestCase):
    is_windows = True

    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.folder = Path(folder.name)
        self.client = FakeClient()
        self.image_checks: list[list[str]] = []
        self.output = io.StringIO()
        for name, value in (("VISUAL_FOLDER", self.folder), ("IS_WINDOWS", self.is_windows),
                            ("run_client", self.client), ("run", self.record_check)):
            patcher = mock.patch.object(dev, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def record_check(self, command: list[str], _environment: object = None) -> None:
        self.image_checks.append(list(command[2:]))

    def proof(self, measure_smoothness: bool = False) -> None:
        with contextlib.redirect_stdout(self.output):
            dev.visual_proof(measure_smoothness)

    def image(self, api: str) -> str:
        return str(self.folder / f"proof_scene_{api}.png")


class WindowsProofTests(VisualProofTestCase):
    def test_each_graphics_api_draws_the_image_twice(self) -> None:
        self.proof()

        self.assertEqual(self.client.apis_drawn(), ["direct3d12", "direct3d12", "vulkan", "vulkan"])

    def test_two_different_images_of_the_same_scene_are_refused(self) -> None:
        self.client.images = iter([b"first", b"second"])

        with self.assertRaises(dev.StepFailed) as caught:
            self.proof()

        self.assertIn("changed between two runs", str(caught.exception))

    def test_only_the_final_image_of_each_api_stays_in_the_folder(self) -> None:
        self.proof()

        names = sorted(path.name for path in self.folder.iterdir())
        self.assertEqual(names, ["proof_scene_direct3d12.png", "proof_scene_vulkan.png"])

    def test_a_client_that_cannot_draw_stops_the_proof(self) -> None:
        self.client.drawing = finished(1, stderr="could not open the GPU")

        with self.assertRaises(dev.StepFailed) as caught:
            self.proof()

        self.assertIn("could not open the GPU", str(caught.exception))
        self.assertEqual(self.image_checks, [])

    def test_the_image_is_measured_and_compared(self) -> None:
        self.proof()

        self.assertEqual(self.image_checks, [
            ["measure-image", "--image", self.image("direct3d12")],
            ["compare-image", "--image", self.image("vulkan"), "--reference", self.image("direct3d12")],
            ["compare-image", "--image", self.image("direct3d12")],
        ])

    def test_the_place_of_the_image_is_shown_before_the_checks(self) -> None:
        with mock.patch.object(dev, "run", side_effect=dev.StepFailed("numbers out of range")):
            with self.assertRaises(dev.StepFailed):
                self.proof()

        self.assertIn(self.image("direct3d12"), self.output.getvalue())

    def test_smoothness_is_measured_with_each_api_when_asked(self) -> None:
        self.proof(measure_smoothness=True)

        self.assertEqual(self.client.apis_measured(), ["direct3d12", "vulkan"])
        self.assertIn("OK    Tempo médio do quadro: 1,43 ms (limite: 16,6 ms)", self.output.getvalue())

    def test_smoothness_is_not_measured_unless_asked(self) -> None:
        self.proof(measure_smoothness=False)

        self.assertEqual(self.client.apis_measured(), [])

    def test_slow_frames_are_shown_and_fail_the_proof(self) -> None:
        self.client.frame_times = finished(1, SLOW_FRAMES)

        with self.assertRaises(dev.StepFailed) as caught:
            self.proof(measure_smoothness=True)

        self.assertIn("took too long", str(caught.exception))
        self.assertIn("FORA  Tempo do pior 1% dos quadros: 31,25 ms (limite: 16,6 ms)", self.output.getvalue())

    def test_a_measurement_that_cannot_finish_fails_with_its_reason(self) -> None:
        self.client.frame_times = finished(1, stderr="the window must stay visible during the measurement")

        with self.assertRaises(dev.StepFailed) as caught:
            self.proof(measure_smoothness=True)

        self.assertIn("must stay visible", str(caught.exception))


class LinuxProofTests(VisualProofTestCase):
    is_windows = False

    def test_only_vulkan_is_drawn(self) -> None:
        self.proof()

        self.assertEqual(self.client.apis_drawn(), ["vulkan", "vulkan"])

    def test_the_image_is_measured_and_compared_with_the_approved_one(self) -> None:
        self.proof()

        self.assertEqual(self.image_checks, [
            ["measure-image", "--image", self.image("vulkan")],
            ["compare-image", "--image", self.image("vulkan")],
        ])


class FrameTimeTextTests(unittest.TestCase):
    def test_the_numbers_are_described_in_the_words_of_the_owner(self) -> None:
        self.assertEqual(dev.describe_frame_times(FAST_FRAMES), [
            "  OK    Tempo médio do quadro: 1,43 ms (limite: 16,6 ms)",
            "  OK    Tempo do pior 1% dos quadros: 2,10 ms (limite: 16,6 ms)",
        ])

    def test_each_number_is_judged_on_its_own(self) -> None:
        self.assertEqual(dev.describe_frame_times(SLOW_FRAMES), [
            "  OK    Tempo médio do quadro: 12,00 ms (limite: 16,6 ms)",
            "  FORA  Tempo do pior 1% dos quadros: 31,25 ms (limite: 16,6 ms)",
        ])

    def test_a_number_at_the_limit_is_accepted(self) -> None:
        text = "average 16.6 ms, worst 1% 16.6 ms (budget 16.6 ms)"

        self.assertTrue(all(line.startswith("  OK") for line in dev.describe_frame_times(text)))

    def test_a_text_that_cannot_be_read_is_refused(self) -> None:
        with self.assertRaises(dev.StepFailed):
            dev.describe_frame_times("something else")


class CommandTests(unittest.TestCase):
    def test_the_gpu_test_also_runs_the_proof_without_measuring_smoothness(self) -> None:
        with mock.patch.object(dev, "build"), mock.patch.object(dev, "run"), \
                mock.patch.object(dev, "build_environment", return_value={}), \
                mock.patch.object(dev, "visual_proof") as visual_proof:
            dev.gpu_test()

        visual_proof.assert_called_once_with(measure_smoothness=False)

    def test_the_visual_command_builds_first_and_measures_smoothness_on_windows(self) -> None:
        with mock.patch.object(dev, "IS_WINDOWS", True), mock.patch.object(dev, "build") as build, \
                mock.patch.object(dev, "visual_proof") as visual_proof:
            dev.visual()

        build.assert_called_once_with("dev")
        visual_proof.assert_called_once_with(measure_smoothness=True)

    def test_the_visual_command_does_not_measure_smoothness_on_linux(self) -> None:
        with mock.patch.object(dev, "IS_WINDOWS", False), mock.patch.object(dev, "build"), \
                mock.patch.object(dev, "visual_proof") as visual_proof:
            dev.visual()

        visual_proof.assert_called_once_with(measure_smoothness=False)

    def test_the_command_line_offers_the_visual_command(self) -> None:
        with mock.patch.object(dev, "visual") as visual, contextlib.redirect_stdout(io.StringIO()):
            code = dev.main(["visual"])

        self.assertEqual(code, 0)
        visual.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
