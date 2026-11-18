"""Refuses dangerous C++ that the compiler accepts (INV-05 and INV-11, docs/CONVENTIONS.md)."""

from __future__ import annotations

import re
from pathlib import Path

from .findings import Finding
from .repository import iter_files, read_text, relative

NAME = "cpp-rules"

_SCANNED_FOLDERS = ("Engine", "Frameworks", "Samples", "tests")
_EXEMPT_FOLDER = "tests/architecture/sanitizer_canary/"
_PLATFORM_FOLDER = "Engine/Source/Runtime/PlatformSDL/Private/"
# Counting allocations (INV-13) requires replacing the global allocation functions. Every other rule still applies.
_ALLOCATION_COUNTER_FOLDER = "tests/architecture/frame_allocations/"

_MEMORY_RULES = (
    (re.compile(r"\bnew\b"), "'new' is forbidden: use std::make_unique or a container (INV-11)"),
    (re.compile(r"\b(malloc|calloc|realloc|free)\s*\("), "manual memory management is forbidden (INV-11)"),
)

_STRING_RULES = (
    (
        re.compile(r"\b(strcpy|strncpy|strcat|strncat|sprintf|vsprintf|gets|scanf|sscanf|fscanf)\s*\("),
        "C string functions without bounds are forbidden: use std::string or std::span",
    ),
)

_RULES = _MEMORY_RULES + _STRING_RULES

_PLATFORM_ONLY_RULES = (
    (re.compile(r"\b(rand|srand)\s*\("), "random numbers must come from an injected source (INV-05)"),
    (re.compile(r"\brandom_device\b"), "random numbers must come from an injected source (INV-05)"),
    (re.compile(r"\b(system_clock|steady_clock|high_resolution_clock)\b"), "time must come from an injected clock (INV-05)"),
    (re.compile(r"\breinterpret_cast\b"), "never reinterpret bytes: read external data with a bounds-checked reader (INV-12)"),
)

_DELETE = re.compile(r"\bdelete\b")
_CODE_PARTS = re.compile(
    r"""
    (?P<raw>R"(?P<delimiter>[^()\s]{0,16})\(.*?\)(?P=delimiter)")
    | (?P<block>/\*.*?\*/)
    | (?P<line>//[^\n]*)
    | (?P<string>"(?:\\.|[^"\\\n])*")
    | (?P<char>'(?:\\.|[^'\\\n])+')
    """,
    re.DOTALL | re.VERBOSE,
)


def strip_comments_and_strings(text: str) -> str:
    """Blanks comments and string contents, keeping the line numbers."""

    def blank(match: re.Match[str]) -> str:
        return "".join("\n" if character == "\n" else " " for character in match.group(0))

    return _CODE_PARTS.sub(blank, text)


def _line_of(text: str, position: int) -> int:
    return text.count("\n", 0, position) + 1


def _is_real_delete(code: str, position: int) -> bool:
    """"= delete" removes a function and is allowed."""
    return not code[:position].rstrip().endswith("=")


def _rules_for(where: str) -> tuple[tuple[re.Pattern[str], str], ...]:
    if where.startswith(_PLATFORM_FOLDER):
        return _RULES
    if where.startswith(_ALLOCATION_COUNTER_FOLDER):
        return _STRING_RULES + _PLATFORM_ONLY_RULES
    return _RULES + _PLATFORM_ONLY_RULES


def check_file(where: str, text: str) -> list[Finding]:
    code = strip_comments_and_strings(text)
    findings = [
        Finding(NAME, where, f"line {_line_of(code, match.start())}: {message}")
        for pattern, message in _rules_for(where)
        for match in pattern.finditer(code)
    ]
    if where.startswith(_ALLOCATION_COUNTER_FOLDER):
        return findings
    findings += [
        Finding(NAME, where, f"line {_line_of(code, match.start())}: 'delete' is forbidden: use std::unique_ptr (INV-11)")
        for match in _DELETE.finditer(code)
        if _is_real_delete(code, match.start())
    ]
    return findings


def run(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for folder in _SCANNED_FOLDERS:
        for file in iter_files(root / folder, ".cpp", ".hpp", ".h"):
            where = relative(root, file)
            if not where.startswith(_EXEMPT_FOLDER):
                findings.extend(check_file(where, read_text(file)))
    return findings
