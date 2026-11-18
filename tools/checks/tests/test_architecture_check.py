import unittest

from support import TempRepositoryTest

from lemuria_checks import architecture_check


class ArchitectureCheckTests(TempRepositoryTest):
    def refused(self, path: str, content: str, expected: str) -> None:
        self.write(path, content)
        self.assert_refused(architecture_check.run(self.root), expected)

    def test_core_cannot_include_classic(self) -> None:
        self.refused("src/core/lemuria_core/src/a.cpp", '#include "lemuria/classic/framework_info.hpp"\n', "INV-02")

    def test_core_cannot_include_the_platform_layer(self) -> None:
        self.refused("src/core/lemuria_core/src/a.cpp", '#include "lemuria/platform/platform_info.hpp"\n', "INV-02")

    def test_core_cannot_include_game_code(self) -> None:
        self.refused("src/core/lemuria_core/src/a.cpp", '#include "demo_protocol/protocol.hpp"\n', "INV-02")

    def test_classic_cannot_include_game_code(self) -> None:
        self.refused("src/framework/lemuria_classic/src/a.cpp", '#include "demo_protocol/protocol.hpp"\n', "INV-02")

    def test_classic_cannot_include_the_platform_layer(self) -> None:
        self.refused("src/framework/lemuria_classic/src/a.cpp", '#include "lemuria/platform/x.hpp"\n', "INV-02")

    def test_custom_framework_cannot_use_classic(self) -> None:
        self.refused("tests/architecture/custom_framework/src/a.cpp", '#include "lemuria/classic/x.hpp"\n', "INV-02")

    def test_sdl_is_refused_outside_the_platform_layer(self) -> None:
        for path in ("src/core/lemuria_core/src/a.cpp", "src/framework/lemuria_classic/src/a.cpp",
                     "tests/core/a_test.cpp", "tools/test_server/src/main.cpp"):
            with self.subTest(path=path):
                self.refused(path, "#include <SDL3/SDL.h>\n", "INV-09")

    def test_sdl_is_refused_in_the_main_file_of_a_program(self) -> None:
        self.refused("samples/demo_client/src/main.cpp", "#include <SDL3/SDL.h>\nint main() {}\n", "INV-09")

    def test_sdl_is_refused_in_the_public_headers_of_the_platform_layer(self) -> None:
        self.refused("src/core/lemuria_platform_sdl/include/lemuria/platform/a.hpp", "#include <SDL3/SDL.h>\n", "INV-09")

    def test_operating_system_headers_are_refused_outside_the_platform_layer(self) -> None:
        for header in ("windows.h", "Windows.h", "unistd.h", "sys/socket.h", "winsock2.h"):
            with self.subTest(header=header):
                self.refused("src/core/lemuria_core/src/a.cpp", f"#include <{header}>\n", "INV-09")

    def test_the_platform_layer_may_use_sdl_and_the_operating_system(self) -> None:
        self.write("src/core/lemuria_platform_sdl/src/a.cpp", "#include <SDL3/SDL.h>\n#include <windows.h>\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_a_commented_include_is_ignored(self) -> None:
        self.write("src/core/lemuria_core/src/a.cpp", '// #include <SDL3/SDL.h>\n/* #include "lemuria/classic/x.hpp" */\n')
        self.assertEqual(architecture_check.run(self.root), [])

    def test_core_cannot_link_classic(self) -> None:
        self.refused("src/core/lemuria_core/CMakeLists.txt", "lemuria_add_library(a PUBLIC_DEPS lemuria_classic)\n", "INV-02")

    def test_classic_cannot_link_sdl(self) -> None:
        self.refused("src/framework/lemuria_classic/CMakeLists.txt", "lemuria_add_library(a PRIVATE_DEPS SDL3::SDL3)\n", "INV-09")

    def test_custom_framework_cannot_link_classic(self) -> None:
        self.refused("tests/architecture/custom_framework/CMakeLists.txt", "lemuria_add_library(a PUBLIC_DEPS lemuria_classic)\n", "INV-02")

    def test_a_program_cannot_link_sdl_directly(self) -> None:
        self.refused("samples/demo_client/CMakeLists.txt", "lemuria_add_program(a DEPS SDL3::SDL3)\n", "INV-09")

    def test_a_game_may_use_every_layer_of_the_engine(self) -> None:
        self.write("samples/demo_client/src/main.cpp",
                   '#include "lemuria/classic/a.hpp"\n#include "lemuria/platform/b.hpp"\n#include "demo_protocol/c.hpp"\n')
        self.assertEqual(architecture_check.run(self.root), [])


if __name__ == "__main__":
    unittest.main()
