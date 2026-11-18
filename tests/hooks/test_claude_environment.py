"""Keeps the environment of the AI consistent with the rules written in AGENTS.md."""

from __future__ import annotations

import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "checks"))

from lemuria_checks import pr_summary  # noqa: E402

SKILLS = ("situacao", "planejar", "implementar", "verificar", "revisar", "fechar", "retrospectiva", "consolidar",
          "propor", "consultar-central")
AGENTS = ("revisor", "auditor")
HOOKS = ("guard-bash", "guard-central", "session-start", "stop-check")

# Every protected path, as written in the list of AGENTS.md.
TRAVAS = (".claude/", ".githooks/", ".github/", "cmake/", "CMakePresets.json", "vcpkg.json", "vcpkg-configuration.json",
          ".clang-format", ".clang-tidy", ".editorconfig", "AGENTS.md", "CLAUDE.md", "tools/dev.py", "tools/checks/",
          "tests/architecture/", "tests/hooks/")
OWNER_DOCUMENTS = ("docs/VISION.md", "docs/ROADMAP.md")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8-sig")


def settings() -> dict:
    return json.loads(read(".claude/settings.json"))


class SkillTests(unittest.TestCase):
    def test_every_skill_of_the_cycle_exists_with_its_own_name(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertRegex(read(f".claude/skills/{skill}/SKILL.md"), rf"(?m)^name: {skill}$")

    def test_every_skill_has_a_description(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertRegex(read(f".claude/skills/{skill}/SKILL.md"), r"(?m)^description: .{20,}$")

    def test_the_rules_mention_every_skill(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertIn(f"/{skill}", read("CLAUDE.md"))


class AgentTests(unittest.TestCase):
    def test_the_agents_exist_and_cannot_edit_files(self) -> None:
        for agent in AGENTS:
            with self.subTest(agent=agent):
                text = read(f".claude/agents/{agent}.md")
                self.assertRegex(text, rf"(?m)^name: {agent}$")
                tools = re.search(r"(?m)^tools: (.+)$", text).group(1)
                self.assertNotRegex(tools, r"\b(Edit|Write|NotebookEdit)\b")

    def test_the_reviewer_knows_every_invariant(self) -> None:
        highest = max(re.findall(r"INV-(\d{2})", read("docs/ARCHITECTURE.md")))
        self.assertIn(f"INV-01 a INV-{highest}", read(".claude/agents/revisor.md"))


class HookTests(unittest.TestCase):
    def commands(self) -> list[str]:
        return [hook["command"] for groups in settings()["hooks"].values() for group in groups for hook in group["hooks"]]

    def test_every_hook_is_connected(self) -> None:
        for hook in HOOKS:
            with self.subTest(hook=hook):
                self.assertTrue((ROOT / f".claude/hooks/{hook}.ps1").is_file())
                self.assertTrue(any(f"{hook}.ps1" in command for command in self.commands()))

    def test_hooks_are_saved_with_a_byte_order_mark(self) -> None:
        for hook in HOOKS:
            with self.subTest(hook=hook):
                self.assertTrue((ROOT / f".claude/hooks/{hook}.ps1").read_bytes().startswith(b"\xef\xbb\xbf"))

    def test_the_central_guard_watches_every_tool_that_reaches_files(self) -> None:
        groups = settings()["hooks"]["PreToolUse"]
        matcher = next(group["matcher"] for group in groups if "guard-central" in group["hooks"][0]["command"])
        for tool in ("Read", "Grep", "Glob", "Edit", "Write", "NotebookEdit", "Bash", "PowerShell"):
            with self.subTest(tool=tool):
                self.assertIn(tool, matcher.split("|"))


class PermissionTests(unittest.TestCase):
    def test_dangerous_commands_are_denied(self) -> None:
        deny = settings()["permissions"]["deny"]
        for rule in ("Bash(git push --force:*)", "Bash(git commit --no-verify:*)", "Bash(git config:*)", "Bash(gh pr merge:*)",
                     "Bash(gh auth:*)", "Bash(gh api:*)", "Read(~/.ssh/**)"):
            with self.subTest(rule=rule):
                self.assertIn(rule, deny)

    def test_editing_a_lock_asks_the_owner(self) -> None:
        ask = settings()["permissions"]["ask"]
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            rule = f"Edit({path}**)" if path.endswith("/") else f"Edit({path})"
            with self.subTest(rule=rule):
                self.assertIn(rule, ask)

    def test_the_settings_have_no_personal_paths(self) -> None:
        self.assertNotRegex(read(".claude/settings.json"), r"(?i)c:[/\\]+users|/home/")


class LockListTests(unittest.TestCase):
    def test_agents_md_lists_every_lock(self) -> None:
        for path in TRAVAS:
            with self.subTest(path=path):
                self.assertIn(f"`{path}`", read("AGENTS.md"))

    def test_code_owners_lists_every_lock(self) -> None:
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            with self.subTest(path=path):
                self.assertRegex(read(".github/CODEOWNERS"), rf"(?m)^/{re.escape(path)}\s")

    def test_the_pull_request_summary_flags_every_lock(self) -> None:
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            sample = f"{path}file" if path.endswith("/") else path
            with self.subTest(path=sample):
                self.assertTrue(pr_summary.is_protected(sample))


if __name__ == "__main__":
    unittest.main()
