import unittest

import support  # noqa: F401

from lemuria_checks import pr_summary


class PrSummaryTests(unittest.TestCase):
    def test_protected_paths_are_recognized(self) -> None:
        for path in (".claude/settings.json", ".github/workflows/ci.yml", "cmake/LemuriaModule.cmake", "AGENTS.md",
                     "tools/dev.py", "tools/checks/run.py", "tests/architecture/CMakeLists.txt", "docs/ROADMAP.md"):
            with self.subTest(path=path):
                self.assertTrue(pr_summary.is_protected(path))

    def test_ordinary_paths_are_not_protected(self) -> None:
        for path in ("src/core/lemuria_core/src/version.cpp", "docs/STATUS.md", "tests/core/version_test.cpp"):
            with self.subTest(path=path):
                self.assertFalse(pr_summary.is_protected(path))

    def test_renames_list_both_paths(self) -> None:
        self.assertEqual(pr_summary.parse_name_status("R100\ta.md\tb.md\nM\tc.md\n"),
                         [("R", "a.md"), ("R", "b.md"), ("M", "c.md")])

    def test_a_clean_change_reports_no_protected_files(self) -> None:
        summary = pr_summary.build([("M", "src/a.cpp")], "")
        self.assertIn("Travas alteradas (0)", summary)
        self.assertIn("Nenhuma.", summary)

    def test_a_protected_change_is_highlighted(self) -> None:
        summary = pr_summary.build([("M", "AGENTS.md")], "")
        self.assertIn("`AGENTS.md` (alterado)", summary)
        self.assertIn("aprovação explícita do dono", summary)

    def test_added_and_removed_tests_are_counted(self) -> None:
        diff = '+TEST_CASE("a") {\n+TEST_CASE("b") {\n-TEST_CASE("c") {\n+int other;\n'
        summary = pr_summary.build([], diff)
        self.assertIn("Testes adicionados: 2", summary)
        self.assertIn("Testes removidos: 1", summary)
        self.assertIn("testes foram removidos", summary)

    def test_deleted_test_files_are_listed(self) -> None:
        summary = pr_summary.build([("D", "tests/core/a_test.cpp"), ("D", "src/a.cpp")], "")
        self.assertIn("Arquivos de teste apagados: 1", summary)
        self.assertIn("`tests/core/a_test.cpp`", summary)


if __name__ == "__main__":
    unittest.main()
