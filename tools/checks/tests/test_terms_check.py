import unittest

from support import TempRepositoryTest

from lemuria_checks import terms_check


class TermsCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        self.write(terms_check.TERMS_FILE, "# comment\nacmegame ; docs/*\nsecretformat ;\nrareclient ; docs/adr/0009-*\n")

    def test_a_forbidden_term_in_code_is_refused(self) -> None:
        self.write("Engine/Source/Runtime/Core/Private/a.cpp", "// reads files of AcmeGame\n")
        self.assert_refused(terms_check.run(self.root), "the term 'acmegame' is not allowed")

    def test_a_forbidden_term_is_refused_in_any_folder_outside_the_allowed_ones(self) -> None:
        for path in ("Samples/a.txt", "Frameworks/a.txt", "tests/ATest.cpp", "tools/a.py", "README.md", ".github/a.yml", "templates/a.md"):
            with self.subTest(path=path):
                self.write(path, "acmegame\n")
                self.assert_refused(terms_check.run(self.root), path)

    def test_a_term_is_allowed_in_its_allowed_places(self) -> None:
        self.write("docs/VISION.md", "A reference: AcmeGame.\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_a_term_without_allowed_places_is_refused_even_in_the_docs(self) -> None:
        self.write("docs/VISION.md", "secretformat\n")
        self.assert_refused(terms_check.run(self.root), "secretformat")

    def test_a_term_can_be_limited_to_a_few_documents(self) -> None:
        self.write("docs/adr/0009-terceiros.md", "rareclient\n")
        self.assertEqual(terms_check.run(self.root), [])
        self.write("docs/VISION.md", "rareclient\n")
        self.assert_refused(terms_check.run(self.root), "docs/VISION.md")

    def test_only_whole_words_count(self) -> None:
        self.write("Engine/a.cpp", "// acmegames and myacmegame are other words\n")
        self.assertEqual(terms_check.run(self.root), [])

    def test_files_that_stay_out_of_git_are_skipped(self) -> None:
        self.write(".claude/central.local.json", '{"path": "C:/acmegame"}')
        self.assertEqual(terms_check.run(self.root), [])

    def test_binary_files_are_skipped(self) -> None:
        (self.root / "asset.bin").write_bytes(b"\0acmegame")
        self.assertEqual(terms_check.run(self.root), [])


if __name__ == "__main__":
    unittest.main()
