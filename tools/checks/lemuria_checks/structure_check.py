"""Keeps the repository organized: required files, module layout and file names."""

from __future__ import annotations

import re
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "structure"

REQUIRED_FILES = (
    "AGENTS.md", "CLAUDE.md", "README.md", "LICENSE", "SECURITY.md", "CMakeLists.txt", "CMakePresets.json",
    "vcpkg.json", "vcpkg-configuration.json", ".clang-format", ".clang-tidy", ".editorconfig", ".gitattributes",
    ".gitignore", "docs/ARCHITECTURE.md", "docs/CONVENTIONS.md", "docs/GLOSSARY.md", "docs/PROPOSALS.md",
    "docs/ROADMAP.md", "docs/STATUS.md", "docs/TECH_DEBT.md", "docs/VISION.md", "docs/WORKFLOW.md",
    "docs/adr/README.md", ".claude/settings.json",
)

_LIBRARY_AREAS = {"lemuria_core": "core", "lemuria_platform_sdl": "platform", "lemuria_classic": "classic"}
_TEST_FOLDERS = {"lemuria_core": "core", "lemuria_platform_sdl": "platform_sdl", "lemuria_classic": "classic"}
_MODULE_FOLDERS = ("src/core", "src/framework")
_CPP_NAME = re.compile(r"^[a-z][a-z0-9]*(_[a-z0-9]+)*\.(cpp|hpp)$")
_MODULE_NAME = re.compile(r"^lemuria_[a-z0-9]+(_[a-z0-9]+)*$")
_CODE_FOLDERS = ("src", "samples", "tools/test_server", "tests")
_MODULE_TEST_FOLDERS = ("tests/core", "tests/platform_sdl", "tests/classic", "tests/demo")


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for required in REQUIRED_FILES:
        if not (root / required).is_file():
            findings.append(Finding(NAME, required, "this required file is missing"))
    _check_modules(root, findings)
    _check_file_names(root, findings)
    _check_test_names(root, findings)
    return findings


def _check_modules(root: Path, findings: list[Finding]) -> None:
    top_level = read_text(root / "CMakeLists.txt") if (root / "CMakeLists.txt").is_file() else ""
    for parent in _MODULE_FOLDERS:
        folder = root / parent
        if not folder.is_dir():
            continue
        for module in sorted(entry for entry in folder.iterdir() if entry.is_dir()):
            _check_module(root, parent, module, top_level, findings)


def _check_module(root: Path, parent: str, module: Path, top_level: str, findings: list[Finding]) -> None:
    where = f"{parent}/{module.name}"
    if not _MODULE_NAME.match(module.name):
        findings.append(Finding(NAME, where, "modules must be named lemuria_<area>"))
        return
    if not (module / "CMakeLists.txt").is_file():
        findings.append(Finding(NAME, where, "the module needs a CMakeLists.txt"))
    if not (module / "src").is_dir():
        findings.append(Finding(NAME, where, "the module needs a src folder"))
    area = _LIBRARY_AREAS.get(module.name, module.name.removeprefix("lemuria_"))
    if not (module / "include" / "lemuria" / area).is_dir():
        findings.append(Finding(NAME, where, f"public headers must live in include/lemuria/{area}"))
    if f"add_subdirectory({where})" not in top_level:
        findings.append(Finding(NAME, where, "the module is not added to the top-level CMakeLists.txt"))
    test_folder = _TEST_FOLDERS.get(module.name, module.name.removeprefix("lemuria_"))
    if not (root / "tests" / test_folder).is_dir():
        findings.append(Finding(NAME, where, f"the module needs tests in tests/{test_folder}"))


def _check_file_names(root: Path, findings: list[Finding]) -> None:
    for folder in _CODE_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h", ".cc", ".cxx"):
            if not _CPP_NAME.match(file.name):
                findings.append(Finding(NAME, relative(root, file), "C++ files must be snake_case, with .cpp or .hpp"))


def _check_test_names(root: Path, findings: list[Finding]) -> None:
    for folder in _MODULE_TEST_FOLDERS:
        for file in iter_files(root / folder, ".cpp"):
            if not file.name.endswith("_test.cpp"):
                findings.append(Finding(NAME, relative(root, file), "test files must end with _test.cpp"))
