"""Tests of the demo command of tools/dev.py, which opens the test server and two windows of the game (slice 0)."""

import contextlib
import importlib.util
import io
import re
import socket
import sys
import unittest
from unittest import mock

import support

_SPECIFICATION = importlib.util.spec_from_file_location("lemuria_dev_demo", support.CHECKS_FOLDER.parent / "dev.py")
dev = importlib.util.module_from_spec(_SPECIFICATION)
sys.modules[_SPECIFICATION.name] = dev
_SPECIFICATION.loader.exec_module(dev)

PROTOCOL_HEADER = (support.CHECKS_FOLDER.parent.parent / "Samples" / "DemoGame" / "Source" / "Protocol" / "Public"
                   / "DemoProtocol" / "Protocol.hpp")


class FakeProgram:
    """Stands in for a started program: it is still running unless told otherwise."""

    def __init__(self, exit_code: int | None = None) -> None:
        self.returncode = exit_code

    def poll(self) -> int | None:
        return self.returncode


class DemoTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.events: list[str] = []
        self.started: list[tuple[list[str], str]] = []
        self.server = FakeProgram()
        self.listening = iter([False, True])
        self.output = io.StringIO()
        for name, value in (("IS_WINDOWS", True), ("build", self.record_build), ("is_listening", self.answer_listening),
                            ("start_program", self.record_start), ("time", mock.Mock(wraps=dev.time))):
            patcher = mock.patch.object(dev, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        dev.time.sleep = mock.Mock()

    def record_build(self, kind: str = "dev") -> None:
        self.events.append(f"build {kind}")

    def answer_listening(self, port: int) -> bool:
        self.events.append(f"probe {port}")
        return next(self.listening)

    def record_start(self, command: list[str], creation_flag: str) -> FakeProgram:
        self.started.append((command, creation_flag))
        self.events.append("start " + command[0].replace("\\", "/").rsplit("/", 1)[-1])
        return self.server if "test_server" in command[0] else FakeProgram()

    def open_demo(self) -> None:
        with contextlib.redirect_stdout(self.output):
            dev.demo()


class DemoTests(DemoTestCase):
    def test_it_builds_then_opens_the_server_and_two_windows_of_the_game(self) -> None:
        self.open_demo()

        self.assertEqual(self.events, ["build dev", "probe 24000", "start test_server.exe", "probe 24000",
                                       "start demo_client.exe", "start demo_client.exe"])

    def test_every_program_uses_the_port_of_the_demonstration(self) -> None:
        self.open_demo()

        for command, _flag in self.started:
            self.assertEqual(command[1:], ["--port", "24000"])

    def test_the_server_gets_a_window_the_owner_can_close_and_the_game_gets_no_console(self) -> None:
        self.open_demo()

        self.assertEqual([flag for _command, flag in self.started],
                         ["CREATE_NEW_CONSOLE", "CREATE_NO_WINDOW", "CREATE_NO_WINDOW"])

    def test_the_game_waits_until_the_server_listens(self) -> None:
        self.listening = iter([False, False, False, True])

        self.open_demo()

        self.assertEqual(self.events, ["build dev", "probe 24000", "start test_server.exe", "probe 24000",
                                       "probe 24000", "probe 24000", "start demo_client.exe",
                                       "start demo_client.exe"])

    def test_a_port_in_use_stops_the_demo_before_opening_anything(self) -> None:
        self.listening = iter([True])

        with self.assertRaisesRegex(dev.StepFailed, "Port 24000 is already in use"):
            self.open_demo()
        self.assertEqual(self.started, [])

    def test_a_server_that_closes_at_once_stops_the_demo_without_opening_the_game(self) -> None:
        self.server = FakeProgram(exit_code=1)
        self.listening = iter([False, False])

        with self.assertRaisesRegex(dev.StepFailed, "closed as soon as it started, with exit code 1"):
            self.open_demo()
        self.assertEqual(len(self.started), 1)

    def test_a_server_that_never_listens_stops_the_demo_without_opening_the_game(self) -> None:
        # Never listening, but a wait without a limit fails here instead of hanging.
        self.listening = iter([False] * 100)
        clock = iter(range(0, 1000, 5))
        dev.time.monotonic = mock.Mock(side_effect=lambda: float(next(clock)))

        with self.assertRaisesRegex(dev.StepFailed, "did not start listening on port 24000"):
            self.open_demo()
        self.assertEqual(len(self.started), 1)

    def test_the_owner_is_told_how_to_see_both_windows_and_how_to_close_everything(self) -> None:
        self.open_demo()

        message = self.output.getvalue()
        self.assertIn("arraste uma delas para o lado", message)
        self.assertIn("feche as janelas do jogo e a janela do servidor de teste", message)

    def test_the_demo_is_refused_outside_windows(self) -> None:
        with mock.patch.object(dev, "IS_WINDOWS", False):
            with self.assertRaisesRegex(dev.StepFailed, "TD-0006"):
                self.open_demo()
        self.assertEqual(self.events, [])


class ListeningTests(unittest.TestCase):
    def test_a_port_with_a_program_listening_is_seen(self) -> None:
        with socket.create_server(("127.0.0.1", 0)) as server:
            self.assertTrue(dev.is_listening(server.getsockname()[1]))

    def test_a_port_without_a_program_is_seen_as_free(self) -> None:
        with socket.create_server(("127.0.0.1", 0)) as server:
            port = server.getsockname()[1]
        self.assertFalse(dev.is_listening(port))


class AgreementTests(unittest.TestCase):
    def test_the_port_is_the_one_the_demonstration_uses(self) -> None:
        match = re.search(r"kDefaultServerPort = ([\d']+);", PROTOCOL_HEADER.read_text(encoding="utf-8"))

        self.assertIsNotNone(match)
        self.assertEqual(int(match.group(1).replace("'", "")), dev.DEMO_PORT)

    def test_the_command_line_offers_the_demo_command(self) -> None:
        with mock.patch.object(dev, "demo") as demo, contextlib.redirect_stdout(io.StringIO()):
            code = dev.main(["demo"])

        self.assertEqual(code, 0)
        demo.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
