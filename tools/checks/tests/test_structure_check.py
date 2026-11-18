import unittest

from support import TempRepositoryTest

from lemuria_checks import structure_check


class StructureCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        for required in structure_check.REQUIRED_FILES:
            self.write(required, "")
        self.write("CMakeLists.txt", "add_subdirectory(src/core/lemuria_core)\n")
        self.write("src/core/lemuria_core/CMakeLists.txt", "")
        self.write("src/core/lemuria_core/src/version.cpp", "")
        self.write("src/core/lemuria_core/include/lemuria/core/version.hpp", "")
        self.write("tests/core/version_test.cpp", "")

    def test_a_valid_repository_has_no_findings(self) -> None:
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_missing_required_file_is_refused(self) -> None:
        (self.root / "docs/VISION.md").unlink()
        self.assert_refused(structure_check.run(self.root), "docs/VISION.md")

    def test_a_module_needs_the_lemuria_prefix(self) -> None:
        self.write("src/core/physics/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "modules must be named")

    def test_a_module_must_be_added_to_the_build(self) -> None:
        self.write("CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the top-level")

    def test_a_module_needs_public_headers_in_the_right_folder(self) -> None:
        (self.root / "src/core/lemuria_core/include/lemuria/core/version.hpp").unlink()
        (self.root / "src/core/lemuria_core/include/lemuria/core").rmdir()
        self.assert_refused(structure_check.run(self.root), "include/lemuria/core")

    def test_a_module_needs_tests(self) -> None:
        (self.root / "tests/core/version_test.cpp").unlink()
        (self.root / "tests/core").rmdir()
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_cpp_files_must_be_snake_case(self) -> None:
        self.write("src/core/lemuria_core/src/FrameClock.cpp", "")
        self.assert_refused(structure_check.run(self.root), "snake_case")

    def test_test_files_must_end_with_test(self) -> None:
        self.write("tests/core/version.cpp", "")
        self.assert_refused(structure_check.run(self.root), "_test.cpp")


if __name__ == "__main__":
    unittest.main()
