"""Refuses names of other games and of their file formats (INV-16)."""

from __future__ import annotations

import fnmatch
import re
from dataclasses import dataclass
from pathlib import Path

from .findings import Finding
from .repository import iter_files, relative

NAME = "terms"
TERMS_FILE = "tools/checks/forbidden_terms.txt"
LOCAL_FILES = frozenset({".claude/central.local.json", ".claude/settings.local.json"})


@dataclass(frozen=True)
class Term:
    text: str
    allowed: tuple[str, ...]
    pattern: re.Pattern[str]


def load_terms(root: Path) -> list[Term]:
    terms: list[Term] = []
    file = root / TERMS_FILE
    if not file.is_file():
        return terms
    for line in file.read_text(encoding="utf-8").splitlines():
        content = line.strip()
        if not content or content.startswith("#"):
            continue
        text, _, allowed = content.partition(";")
        text = text.strip().lower()
        places = tuple(place.strip() for place in allowed.split(",") if place.strip())
        pattern = re.compile(rf"(?<![a-z0-9]){re.escape(text)}(?![a-z0-9])", re.IGNORECASE)
        terms.append(Term(text, places, pattern))
    return terms


def _is_allowed(term: Term, where: str) -> bool:
    return any(fnmatch.fnmatch(where, place) for place in term.allowed)


def run(root: Path) -> list[Finding]:
    terms = load_terms(root)
    findings: list[Finding] = []
    for file in iter_files(root):
        where = relative(root, file)
        if where == TERMS_FILE or where in LOCAL_FILES:
            continue
        content = file.read_bytes()
        if b"\0" in content:
            continue
        text = content.decode("utf-8", errors="replace")
        for term in terms:
            match = term.pattern.search(text) if not _is_allowed(term, where) else None
            if match:
                line = text.count("\n", 0, match.start()) + 1
                findings.append(Finding(NAME, where, f"line {line}: the term '{term.text}' is not allowed here (INV-16)"))
    return findings
