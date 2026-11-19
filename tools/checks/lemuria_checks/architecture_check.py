"""Keeps the layers apart: who may include and link what (INV-02, INV-07 and INV-09).

Every C++ file is checked, including the Main.cpp of the programs.
"""

from __future__ import annotations

import re
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "architecture"

_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"](?P<header>[^>"]+)[>"]', re.MULTILINE)
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//[^\n]*")
_CMAKE_COMMENT = re.compile(r"#[^\n]*")

# Letter case is ignored: on Windows, an include written with the wrong case still finds the file.
_HEADER_GROUPS = {
    "classic": re.compile(r"^lemuria/classic/", re.IGNORECASE),
    "platform": re.compile(r"^lemuria/platform/", re.IGNORECASE),
    # Every folder of headers of the demonstration: DemoClient/, DemoProtocol/, DemoTestSupport/ and TestServer/.
    "game": re.compile(r"^(demo(_|[a-z0-9]*/)|testserver/)", re.IGNORECASE),
    "sdl": re.compile(r"^SDL3?(_net)?(/|\.h$)|^SDL_", re.IGNORECASE),
    "os": re.compile(
        r"^(windows\.h|winsock2?\.h|ws2tcpip\.h|winuser\.h|d3d\d*\.h|dxgi\d*\.h|io\.h|direct\.h|process\.h"
        r"|unistd\.h|pthread\.h|dlfcn\.h|fcntl\.h|poll\.h|netdb\.h|sys/.*|arpa/.*|netinet/.*|linux/.*|android/.*)$",
        re.IGNORECASE,
    ),
}

_TARGET_GROUPS = {
    "classic": re.compile(r"\blemuria_classic\b"),
    "platform": re.compile(r"\blemuria_platform_sdl\b"),
    # Every target of the demonstration, such as demo_client_lib, demo_test_support and test_server_lib.
    "game": re.compile(r"\b(demo_\w+|test_server\w*)\b"),
    "sdl": re.compile(r"\bSDL3(_net)?::"),
}

_REASONS = {
    "classic": "must not depend on the Classic framework (INV-02)",
    "platform": "must not depend on the platform layer (INV-02)",
    "game": "must not depend on game code (INV-02)",
    "sdl": "must not use SDL: only Engine/Source/Runtime/PlatformSDL/Private may (INV-09)",
    "os": "must not use the operating system directly: only Engine/Source/Runtime/PlatformSDL/Private may (INV-09)",
}

# What each part of the repository cannot depend on. Parts are tried in order: the first match wins.
_PARTS: tuple[tuple[str, frozenset[str]], ...] = (
    ("Engine/Source/Runtime/Core/", frozenset({"classic", "platform", "game", "sdl", "os"})),
    ("Engine/Source/Runtime/PlatformSDL/Public/", frozenset({"classic", "game", "sdl", "os"})),
    ("Engine/Source/Runtime/PlatformSDL/Tests/", frozenset({"classic", "game", "sdl", "os"})),
    ("Engine/Source/Runtime/PlatformSDL/", frozenset({"classic", "game"})),
    ("Engine/", frozenset({"classic", "game", "sdl", "os"})),
    ("Frameworks/", frozenset({"platform", "game", "sdl", "os"})),
    ("tests/architecture/custom_framework/", frozenset({"classic", "platform", "game", "sdl", "os"})),
    ("Samples/DemoGame/Source/Protocol/", frozenset({"classic", "platform", "sdl", "os"})),
    # The test server needs TCP and a real clock, and only the platform layer offers them (docs/adr/0018-*.md).
    ("Samples/DemoGame/Source/TestServer/", frozenset({"classic", "sdl", "os"})),
    ("Samples/", frozenset({"sdl", "os"})),
    ("tests/", frozenset({"sdl", "os"})),
)

_SCANNED_FOLDERS = ("Engine", "Frameworks", "Samples", "tests")


def _forbidden_for(path: str) -> frozenset[str]:
    for prefix, forbidden in _PARTS:
        if path.startswith(prefix):
            return forbidden
    return frozenset()


def _without_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", text))


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for folder in _SCANNED_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h"):
            _check_includes(root, file, findings)
        for file in iter_files(root / folder, "CMakeLists.txt", ".cmake"):
            _check_targets(root, file, findings)
    return findings


def _check_includes(root: Path, file: Path, findings: list[Finding]) -> None:
    where = relative(root, file)
    forbidden = _forbidden_for(where)
    for match in _INCLUDE.finditer(_without_comments(read_text(file))):
        header = match.group("header")
        for group in sorted(forbidden):
            if _HEADER_GROUPS[group].search(header):
                findings.append(Finding(NAME, where, f"includes <{header}>: this code {_REASONS[group]}"))


def _check_targets(root: Path, file: Path, findings: list[Finding]) -> None:
    where = relative(root, file)
    forbidden = _forbidden_for(where)
    text = _CMAKE_COMMENT.sub("", read_text(file))
    for group in sorted(forbidden & _TARGET_GROUPS.keys()):
        if _TARGET_GROUPS[group].search(text):
            findings.append(Finding(NAME, where, f"links a forbidden target: this code {_REASONS[group]}"))
