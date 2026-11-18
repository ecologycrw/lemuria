import unittest

from support import TempRepositoryTest

from lemuria_checks import architecture_check

CORE = "Engine/Source/Runtime/Core"
PLATFORM = "Engine/Source/Runtime/PlatformSDL"
CLASSIC = "Frameworks/Classic"
CLIENT = "Samples/DemoGame/Source/Client"
PROTOCOL = "Samples/DemoGame/Source/Protocol"
TEST_SERVER = "Samples/DemoGame/Source/TestServer"


class ArchitectureCheckTests(TempRepositoryTest):
    def refused(self, path: str, content: str, expected: str) -> None:
        self.write(path, content)
        self.assert_refused(architecture_check.run(self.root), expected)

    def test_core_cannot_include_classic(self) -> None:
        self.refused(f"{CORE}/Private/A.cpp", '#include "Lemuria/Classic/FrameworkInfo.hpp"\n', "INV-02")

    def test_core_cannot_include_the_platform_layer(self) -> None:
        self.refused(f"{CORE}/Private/A.cpp", '#include "Lemuria/Platform/PlatformInfo.hpp"\n', "INV-02")

    def test_core_cannot_include_game_code(self) -> None:
        self.refused(f"{CORE}/Private/A.cpp", '#include "DemoProtocol/Protocol.hpp"\n', "INV-02")

    def test_an_include_with_the_wrong_case_does_not_escape_the_rules(self) -> None:
        for header in ("lemuria/classic/x.hpp", "LEMURIA/CLASSIC/X.hpp", "lemuria/platform/x.hpp",
                       "demoprotocol/protocol.hpp", "demo_protocol/protocol.hpp"):
            with self.subTest(header=header):
                self.refused(f"{CORE}/Private/A.cpp", f'#include "{header}"\n', "INV-02")

    def test_the_public_headers_of_core_follow_the_same_rules(self) -> None:
        self.refused(f"{CORE}/Public/Lemuria/Core/A.hpp", '#include "Lemuria/Classic/X.hpp"\n', "INV-02")

    def test_the_tests_of_core_follow_the_same_rules(self) -> None:
        self.refused(f"{CORE}/Tests/ATest.cpp", '#include "Lemuria/Classic/X.hpp"\n', "INV-02")

    def test_a_new_engine_module_cannot_depend_on_classic_or_on_a_game(self) -> None:
        for header in ("Lemuria/Classic/X.hpp", "DemoProtocol/Protocol.hpp"):
            with self.subTest(header=header):
                self.refused("Engine/Source/Runtime/Renderer/Private/A.cpp", f'#include "{header}"\n', "INV-02")

    def test_a_new_engine_module_cannot_use_sdl(self) -> None:
        self.refused("Engine/Source/Runtime/Renderer/Private/A.cpp", "#include <SDL3/SDL.h>\n", "INV-09")

    def test_classic_cannot_include_game_code(self) -> None:
        self.refused(f"{CLASSIC}/Private/A.cpp", '#include "DemoProtocol/Protocol.hpp"\n', "INV-02")

    def test_classic_cannot_include_the_platform_layer(self) -> None:
        self.refused(f"{CLASSIC}/Private/A.cpp", '#include "Lemuria/Platform/X.hpp"\n', "INV-02")

    def test_custom_framework_cannot_use_classic(self) -> None:
        self.refused("tests/architecture/custom_framework/Private/A.cpp", '#include "Lemuria/Classic/X.hpp"\n', "INV-02")

    def test_the_game_protocol_and_the_test_server_cannot_use_the_client_layers(self) -> None:
        for folder in (PROTOCOL, TEST_SERVER):
            with self.subTest(folder=folder):
                self.refused(f"{folder}/Private/A.cpp", '#include "Lemuria/Classic/X.hpp"\n', "INV-02")

    def test_the_test_server_can_use_the_platform_layer(self) -> None:
        self.write(f"{TEST_SERVER}/Private/Main.cpp", '#include "Lemuria/Platform/TcpListener.hpp"\n')
        self.write(f"{TEST_SERVER}/CMakeLists.txt", "lemuria_add_program(test_server DEPS lemuria_platform_sdl)\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_the_test_server_still_cannot_use_classic_sdl_or_the_operating_system(self) -> None:
        cases = (
            ('#include "Lemuria/Classic/X.hpp"\n', "INV-02"),
            ("#include <SDL3/SDL.h>\n", "INV-09"),
            ("#include <SDL3_net/SDL_net.h>\n", "INV-09"),
            ("#include <winsock2.h>\n", "INV-09"),
            ("#include <sys/socket.h>\n", "INV-09"),
        )
        for content, expected in cases:
            with self.subTest(content=content):
                self.refused(f"{TEST_SERVER}/Private/Main.cpp", content, expected)

    def test_the_test_server_still_cannot_link_classic_or_sdl(self) -> None:
        cases = (("lemuria_classic", "INV-02"), ("SDL3::SDL3", "INV-09"), ("SDL3_net::SDL3_net", "INV-09"))
        for target, expected in cases:
            with self.subTest(target=target):
                self.refused(f"{TEST_SERVER}/CMakeLists.txt", f"lemuria_add_program(a DEPS {target})\n", expected)

    def test_the_game_protocol_still_cannot_use_the_platform_layer(self) -> None:
        self.refused(f"{PROTOCOL}/Private/A.cpp", '#include "Lemuria/Platform/X.hpp"\n', "INV-02")

    def test_sdl_net_is_refused_outside_the_platform_layer(self) -> None:
        for path in (f"{CORE}/Private/A.cpp", f"{CLASSIC}/Private/A.cpp", f"{CLIENT}/Private/Main.cpp",
                     f"{PLATFORM}/Public/Lemuria/Platform/A.hpp", f"{PLATFORM}/Tests/ATest.cpp"):
            with self.subTest(path=path):
                self.refused(path, "#include <SDL3_net/SDL_net.h>\n", "INV-09")

    def test_the_tests_of_the_platform_layer_cannot_link_sdl_net(self) -> None:
        self.refused(f"{PLATFORM}/Tests/CMakeLists.txt", "lemuria_add_tests(a DEPS SDL3_net::SDL3_net)\n", "INV-09")

    def test_the_platform_layer_can_use_sdl_net(self) -> None:
        self.write(f"{PLATFORM}/Private/A.cpp", "#include <SDL3_net/SDL_net.h>\n")
        self.write(f"{PLATFORM}/CMakeLists.txt", "lemuria_add_library(a PRIVATE_DEPS SDL3_net::SDL3_net)\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_sdl_is_refused_outside_the_platform_layer(self) -> None:
        for path in (f"{CORE}/Private/A.cpp", f"{CLASSIC}/Private/A.cpp", f"{CORE}/Tests/ATest.cpp",
                     f"{TEST_SERVER}/Private/Main.cpp", "tests/architecture/ATest.cpp"):
            with self.subTest(path=path):
                self.refused(path, "#include <SDL3/SDL.h>\n", "INV-09")

    def test_sdl_is_refused_in_the_main_file_of_a_program(self) -> None:
        self.refused(f"{CLIENT}/Private/Main.cpp", "#include <SDL3/SDL.h>\nint main() {}\n", "INV-09")

    def test_sdl_is_refused_in_the_public_headers_of_the_platform_layer(self) -> None:
        self.refused(f"{PLATFORM}/Public/Lemuria/Platform/A.hpp", "#include <SDL3/SDL.h>\n", "INV-09")

    def test_sdl_is_refused_in_the_tests_of_the_platform_layer(self) -> None:
        self.refused(f"{PLATFORM}/Tests/ATest.cpp", "#include <SDL3/SDL.h>\n", "INV-09")

    def test_the_tests_of_the_platform_layer_cannot_link_sdl(self) -> None:
        self.refused(f"{PLATFORM}/Tests/CMakeLists.txt", "lemuria_add_tests(a DEPS SDL3::SDL3)\n", "INV-09")

    def test_operating_system_headers_are_refused_outside_the_platform_layer(self) -> None:
        for header in ("windows.h", "Windows.h", "unistd.h", "sys/socket.h", "winsock2.h"):
            with self.subTest(header=header):
                self.refused(f"{CORE}/Private/A.cpp", f"#include <{header}>\n", "INV-09")

    def test_the_platform_layer_may_use_sdl_and_the_operating_system(self) -> None:
        self.write(f"{PLATFORM}/Private/A.cpp", "#include <SDL3/SDL.h>\n#include <windows.h>\n")
        self.write(f"{PLATFORM}/CMakeLists.txt", "lemuria_add_library(a PRIVATE_DEPS SDL3::SDL3)\n")
        self.assertEqual(architecture_check.run(self.root), [])

    def test_a_commented_include_is_ignored(self) -> None:
        self.write(f"{CORE}/Private/A.cpp", '// #include <SDL3/SDL.h>\n/* #include "Lemuria/Classic/X.hpp" */\n')
        self.assertEqual(architecture_check.run(self.root), [])

    def test_core_cannot_link_classic(self) -> None:
        self.refused(f"{CORE}/CMakeLists.txt", "lemuria_add_library(a PUBLIC_DEPS lemuria_classic)\n", "INV-02")

    def test_classic_cannot_link_sdl(self) -> None:
        self.refused(f"{CLASSIC}/CMakeLists.txt", "lemuria_add_library(a PRIVATE_DEPS SDL3::SDL3)\n", "INV-09")

    def test_custom_framework_cannot_link_classic(self) -> None:
        self.refused("tests/architecture/custom_framework/CMakeLists.txt", "lemuria_add_library(a PUBLIC_DEPS lemuria_classic)\n", "INV-02")

    def test_a_program_cannot_link_sdl_directly(self) -> None:
        self.refused(f"{CLIENT}/CMakeLists.txt", "lemuria_add_program(a DEPS SDL3::SDL3)\n", "INV-09")

    def test_a_game_may_use_every_layer_of_the_engine(self) -> None:
        self.write(f"{CLIENT}/Private/Main.cpp",
                   '#include "Lemuria/Classic/A.hpp"\n#include "Lemuria/Platform/B.hpp"\n#include "DemoProtocol/C.hpp"\n')
        self.assertEqual(architecture_check.run(self.root), [])


if __name__ == "__main__":
    unittest.main()
