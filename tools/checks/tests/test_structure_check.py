import shutil
import unittest

from support import TempRepositoryTest

from lemuria_checks import structure_check

CORE = "Engine/Source/Runtime/Core"


class StructureCheckTests(TempRepositoryTest):
    def setUp(self) -> None:
        super().setUp()
        for required in structure_check.REQUIRED_FILES:
            self.write(required, "")
        self.write("CMakeLists.txt", f"add_subdirectory({CORE})\n")
        self.write(f"{CORE}/CMakeLists.txt", "add_subdirectory(Tests)\n")
        self.write(f"{CORE}/Private/version.cpp", "")
        self.write(f"{CORE}/Public/lemuria/core/version.hpp", "")
        self.write(f"{CORE}/Tests/version_test.cpp", "")

    def test_a_valid_repository_has_no_findings(self) -> None:
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_missing_required_file_is_refused(self) -> None:
        (self.root / "docs/VISION.md").unlink()
        self.assert_refused(structure_check.run(self.root), "docs/VISION.md")

    def test_a_module_must_be_named_in_pascal_case(self) -> None:
        self.write("Engine/Source/Runtime/lemuria_physics/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "modules must be named")

    def test_a_framework_follows_the_same_module_rules(self) -> None:
        self.write("Frameworks/Classic/CMakeLists.txt", "")
        findings = structure_check.run(self.root)
        self.assert_refused(findings, "needs a Private folder")
        self.assert_refused(findings, "Public/lemuria/classic")
        self.assert_refused(findings, "needs tests")

    def test_a_module_must_be_added_to_the_build(self) -> None:
        self.write("CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the top-level")

    def test_a_module_needs_a_private_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Private")
        self.assert_refused(structure_check.run(self.root), "needs a Private folder")

    def test_a_module_needs_public_headers_in_the_right_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Public")
        self.assert_refused(structure_check.run(self.root), "Public/lemuria/core")

    def test_a_module_needs_tests(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Tests")
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_an_empty_tests_folder_is_not_enough(self) -> None:
        (self.root / f"{CORE}/Tests/version_test.cpp").unlink()
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_the_tests_of_a_module_must_be_added_to_its_build(self) -> None:
        self.write(f"{CORE}/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the build of the module")

    def test_the_tests_of_a_sample_must_be_added_to_its_build(self) -> None:
        self.write("Samples/DemoGame/Source/Protocol/CMakeLists.txt", "")
        self.write("Samples/DemoGame/Source/Protocol/Tests/a_test.cpp", "")
        self.assert_refused(structure_check.run(self.root), "not added to the build of the module")

    def test_the_retired_folders_cannot_come_back(self) -> None:
        for folder in ("src", "samples", "include"):
            with self.subTest(folder=folder):
                self.write(f"{folder}/a.cpp", "")
                self.assert_refused(structure_check.run(self.root), "this folder was retired")
                shutil.rmtree(self.root / folder)

    def test_cpp_files_must_be_snake_case(self) -> None:
        self.write(f"{CORE}/Private/FrameClock.cpp", "")
        self.assert_refused(structure_check.run(self.root), "snake_case")

    def test_test_files_must_end_with_test(self) -> None:
        self.write(f"{CORE}/Tests/version.cpp", "")
        self.assert_refused(structure_check.run(self.root), "_test.cpp")


if __name__ == "__main__":
    unittest.main()
