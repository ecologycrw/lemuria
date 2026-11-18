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
        self.write("vcpkg.json", '{"dependencies": ["catch2", {"name": "sdl3"}]}')
        self.write(structure_check.THIRD_PARTY_FILE, "| `catch2` | BSL-1.0 | tests |\n| `sdl3` | zlib | window |\n")
        self.write("CMakeLists.txt", f"add_subdirectory({CORE})\n")
        self.write(f"{CORE}/CMakeLists.txt", "add_subdirectory(Tests)\n")
        self.write(f"{CORE}/Private/Version.cpp", "")
        self.write(f"{CORE}/Public/Lemuria/Core/Version.hpp", "")
        self.write(f"{CORE}/Tests/VersionTest.cpp", "")

    def test_a_valid_repository_has_no_findings(self) -> None:
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_missing_required_file_is_refused(self) -> None:
        (self.root / "docs/VISION.md").unlink()
        self.assert_refused(structure_check.run(self.root), "docs/VISION.md")

    def test_the_visual_guide_and_the_license_list_are_required(self) -> None:
        for required in ("docs/VISUAL.md", "docs/THIRD_PARTY.md"):
            with self.subTest(required=required):
                self.assertIn(required, structure_check.REQUIRED_FILES)

    def test_a_library_without_a_registered_license_is_refused(self) -> None:
        self.write(structure_check.THIRD_PARTY_FILE, "| `catch2` | BSL-1.0 | tests |\n")
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' has no license registered")

    def test_a_library_with_an_empty_license_is_refused(self) -> None:
        self.write(structure_check.THIRD_PARTY_FILE, "| `catch2` | BSL-1.0 | tests |\n| `sdl3` |  | window |\n")
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' has no license registered")

    def test_a_library_that_left_the_build_must_leave_the_list(self) -> None:
        self.write("vcpkg.json", '{"dependencies": ["catch2"]}')
        self.assert_refused(structure_check.run(self.root), "the library 'sdl3' is registered but is not in vcpkg.json")

    def test_an_invalid_library_manifest_is_refused(self) -> None:
        self.write("vcpkg.json", "{")
        self.assert_refused(structure_check.run(self.root), "not valid JSON")

    def test_a_module_must_be_named_in_pascal_case(self) -> None:
        self.write("Engine/Source/Runtime/lemuria_physics/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "modules must be named")

    def test_a_framework_follows_the_same_module_rules(self) -> None:
        self.write("Frameworks/Classic/CMakeLists.txt", "")
        findings = structure_check.run(self.root)
        self.assert_refused(findings, "needs a Private folder")
        self.assert_refused(findings, "Public/Lemuria/Classic")
        self.assert_refused(findings, "needs tests")

    def test_the_platform_module_keeps_its_headers_in_the_platform_folder(self) -> None:
        platform = "Engine/Source/Runtime/PlatformSDL"
        self.write("CMakeLists.txt", f"add_subdirectory({CORE})\nadd_subdirectory({platform})\n")
        self.write(f"{platform}/CMakeLists.txt", "add_subdirectory(Tests)\n")
        self.write(f"{platform}/Private/PlatformInfo.cpp", "")
        self.write(f"{platform}/Public/Lemuria/Platform/PlatformInfo.hpp", "")
        self.write(f"{platform}/Tests/PlatformInfoTest.cpp", "")
        self.assertEqual(structure_check.run(self.root), [])

    def test_a_module_must_be_added_to_the_build(self) -> None:
        self.write("CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the top-level")

    def test_a_module_needs_a_private_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Private")
        self.assert_refused(structure_check.run(self.root), "needs a Private folder")

    def test_a_module_needs_public_headers_in_the_right_folder(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Public")
        self.assert_refused(structure_check.run(self.root), "Public/Lemuria/Core")

    def test_public_folders_written_with_the_wrong_case_are_refused(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Public")
        self.write(f"{CORE}/Public/lemuria/core/Version.hpp", "")
        self.assert_refused(structure_check.run(self.root), "Public/Lemuria/Core")

    def test_a_module_needs_tests(self) -> None:
        shutil.rmtree(self.root / f"{CORE}/Tests")
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_an_empty_tests_folder_is_not_enough(self) -> None:
        (self.root / f"{CORE}/Tests/VersionTest.cpp").unlink()
        self.assert_refused(structure_check.run(self.root), "needs tests")

    def test_the_tests_of_a_module_must_be_added_to_its_build(self) -> None:
        self.write(f"{CORE}/CMakeLists.txt", "")
        self.assert_refused(structure_check.run(self.root), "not added to the build of the module")

    def test_the_tests_of_a_sample_must_be_added_to_its_build(self) -> None:
        self.write("Samples/DemoGame/Source/Protocol/CMakeLists.txt", "")
        self.write("Samples/DemoGame/Source/Protocol/Tests/ProtocolTest.cpp", "")
        self.assert_refused(structure_check.run(self.root), "not added to the build of the module")

    def test_the_retired_folders_cannot_come_back(self) -> None:
        for folder in ("src", "samples", "include"):
            with self.subTest(folder=folder):
                self.write(f"{folder}/A.cpp", "")
                self.assert_refused(structure_check.run(self.root), "this folder was retired")
                shutil.rmtree(self.root / folder)

    def test_cpp_files_must_be_pascal_case(self) -> None:
        for name in ("frame_clock.cpp", "frameClock.hpp", "Frame_Clock.cpp"):
            with self.subTest(name=name):
                self.write(f"{CORE}/Private/{name}", "")
                self.assert_refused(structure_check.run(self.root), "PascalCase")
                (self.root / f"{CORE}/Private/{name}").unlink()

    def test_the_rule_also_applies_to_the_tests_of_the_repository(self) -> None:
        self.write("tests/architecture/use_after_free.cpp", "")
        self.assert_refused(structure_check.run(self.root), "PascalCase")

    def test_test_files_must_end_with_test(self) -> None:
        self.write(f"{CORE}/Tests/Version.cpp", "")
        self.assert_refused(structure_check.run(self.root), "Test.cpp")


if __name__ == "__main__":
    unittest.main()
