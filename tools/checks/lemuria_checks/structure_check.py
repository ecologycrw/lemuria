"""Keeps the repository organized: required files, module layout and file names (docs/adr/0014-organizacao-do-repositorio.md)."""

from __future__ import annotations

import re
from collections.abc import Iterator
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

_LIBRARY_AREAS = {"Core": "core", "PlatformSDL": "platform", "Classic": "classic"}
_MODULE_FOLDERS = ("Engine/Source/Runtime", "Frameworks")
_PRODUCT_FOLDERS = ("Engine", "Frameworks", "Samples")
_RETIRED_FOLDERS = frozenset({"src", "samples", "include"})
_CPP_NAME = re.compile(r"^[a-z][a-z0-9]*(_[a-z0-9]+)*\.(cpp|hpp)$")
_MODULE_NAME = re.compile(r"^[A-Z][A-Za-z0-9]*$")
_CODE_FOLDERS = (*_PRODUCT_FOLDERS, "tests")


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for required in REQUIRED_FILES:
        if not (root / required).is_file():
            findings.append(Finding(NAME, required, "this required file is missing"))
    _check_retired_folders(root, findings)
    _check_modules(root, findings)
    _check_file_names(root, findings)
    _check_test_folders(root, findings)
    return findings


def _check_retired_folders(root: Path, findings: list[Finding]) -> None:
    # The names are compared exactly: on Windows, "samples" would also find the "Samples" folder.
    folders = {entry.name for entry in root.iterdir() if entry.is_dir()}
    for folder in sorted(folders & _RETIRED_FOLDERS):
        findings.append(Finding(NAME, folder, "this folder was retired: use Engine, Frameworks or Samples"))


def _check_modules(root: Path, findings: list[Finding]) -> None:
    top_level = read_text(root / "CMakeLists.txt") if (root / "CMakeLists.txt").is_file() else ""
    for parent in _MODULE_FOLDERS:
        folder = root / parent
        if not folder.is_dir():
            continue
        for module in sorted(entry for entry in folder.iterdir() if entry.is_dir()):
            _check_module(parent, module, top_level, findings)


def _check_module(parent: str, module: Path, top_level: str, findings: list[Finding]) -> None:
    where = f"{parent}/{module.name}"
    if not _MODULE_NAME.match(module.name):
        findings.append(Finding(NAME, where, "modules must be named in PascalCase"))
        return
    if not (module / "CMakeLists.txt").is_file():
        findings.append(Finding(NAME, where, "the module needs a CMakeLists.txt"))
    if not (module / "Private").is_dir():
        findings.append(Finding(NAME, where, "the module needs a Private folder"))
    area = _LIBRARY_AREAS.get(module.name, module.name.lower())
    if not (module / "Public" / "lemuria" / area).is_dir():
        findings.append(Finding(NAME, where, f"public headers must live in Public/lemuria/{area}"))
    if f"add_subdirectory({where})" not in top_level:
        findings.append(Finding(NAME, where, "the module is not added to the top-level CMakeLists.txt"))
    if not any(iter_files(module / "Tests", "_test.cpp")):
        findings.append(Finding(NAME, where, "the module needs tests in its Tests folder"))


def _check_file_names(root: Path, findings: list[Finding]) -> None:
    for folder in _CODE_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h", ".cc", ".cxx"):
            if not _CPP_NAME.match(file.name):
                findings.append(Finding(NAME, relative(root, file), "C++ files must be snake_case, with .cpp or .hpp"))


def _test_folders(root: Path) -> Iterator[Path]:
    for folder in _PRODUCT_FOLDERS:
        if (root / folder).is_dir():
            yield from sorted(path for path in (root / folder).rglob("Tests") if path.is_dir())


def _check_test_folders(root: Path, findings: list[Finding]) -> None:
    for tests in _test_folders(root):
        owner = tests.parent / "CMakeLists.txt"
        if not owner.is_file() or "add_subdirectory(Tests)" not in read_text(owner):
            findings.append(Finding(NAME, relative(root, tests), "the tests are not added to the build of the module"))
        for file in iter_files(tests, ".cpp"):
            if not file.name.endswith("_test.cpp"):
                findings.append(Finding(NAME, relative(root, file), "test files must end with _test.cpp"))
