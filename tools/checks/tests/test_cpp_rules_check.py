import unittest

from support import TempRepositoryTest

from lemuria_checks import cpp_rules_check

ALLOCATION_COUNTER = "tests/architecture/frame_allocations"


class CppRulesTests(TempRepositoryTest):
    def findings_for(self, code: str, path: str = "Engine/Source/Runtime/Core/Private/a.cpp") -> list:
        self.write(path, code)
        return cpp_rules_check.run(self.root)

    def test_loose_new_is_refused(self) -> None:
        self.assert_refused(self.findings_for("int* a = new int(1);\n"), "'new' is forbidden")

    def test_loose_delete_is_refused(self) -> None:
        self.assert_refused(self.findings_for("void f(int* a) { delete a; }\n"), "'delete' is forbidden")

    def test_array_delete_is_refused(self) -> None:
        self.assert_refused(self.findings_for("void f(int* a) { delete[] a; }\n"), "'delete' is forbidden")

    def test_deleted_functions_are_allowed(self) -> None:
        self.assertEqual(self.findings_for("struct A { A(const A&) = delete; A& operator=(A&&) =\n delete; };\n"), [])

    def test_manual_memory_is_refused(self) -> None:
        for call in ("malloc(4)", "std::calloc(1, 4)", "realloc(p, 4)", "free(p)"):
            with self.subTest(call=call):
                self.assert_refused(self.findings_for(f"auto a = {call};\n"), "manual memory")

    def test_functions_of_other_libraries_with_similar_names_are_allowed(self) -> None:
        self.assertEqual(self.findings_for("void f() { SDL_free(p); release_free(p); }\n"), [])

    def test_unbounded_string_functions_are_refused(self) -> None:
        for call in ("strcpy(a, b)", "sprintf(a, b)", "std::strcat(a, b)", "scanf(a)"):
            with self.subTest(call=call):
                self.assert_refused(self.findings_for(f"void f() {{ {call}; }}\n"), "C string functions")

    def test_system_time_and_random_are_refused_outside_the_platform_layer(self) -> None:
        for code in ("std::chrono::steady_clock::now()", "std::chrono::system_clock::now()", "rand()", "std::random_device d"):
            with self.subTest(code=code):
                self.assert_refused(self.findings_for(f"void f() {{ {code}; }}\n"), "INV-05")

    def test_the_platform_layer_may_read_the_system_clock(self) -> None:
        code = "auto now() { return std::chrono::steady_clock::now(); }\n"
        self.assertEqual(self.findings_for(code, "Engine/Source/Runtime/PlatformSDL/Private/a.cpp"), [])

    def test_the_tests_of_the_platform_layer_cannot_read_the_system_clock(self) -> None:
        code = "auto now() { return std::chrono::steady_clock::now(); }\n"
        self.assert_refused(self.findings_for(code, "Engine/Source/Runtime/PlatformSDL/Tests/ATest.cpp"), "INV-05")

    def test_reinterpreting_bytes_is_refused(self) -> None:
        self.assert_refused(self.findings_for("auto p = reinterpret_cast<const H*>(bytes);\n"), "INV-12")

    def test_comments_and_strings_are_ignored(self) -> None:
        code = '// new delete malloc(1)\n/* strcpy(a, b) */\nauto a = "new delete free(p)";\nauto b = R"x(rand())x";\n'
        self.assertEqual(self.findings_for(code), [])

    def test_the_line_of_the_problem_is_reported(self) -> None:
        self.assert_refused(self.findings_for("// first\n\nint* a = new int(1);\n"), "line 3")

    def test_rules_also_apply_to_programs_and_tests(self) -> None:
        for path in ("Samples/DemoGame/Source/Client/Private/Main.cpp", "Engine/Source/Runtime/Core/Tests/ATest.cpp",
                     "Samples/DemoGame/Source/TestServer/Private/Main.cpp", "Frameworks/Classic/Private/a.cpp",
                     "tests/architecture/ATest.cpp"):
            with self.subTest(path=path):
                self.assert_refused(self.findings_for("int* a = new int(1);\n", path), "'new' is forbidden")

    def test_the_sanitizer_canaries_are_exempt(self) -> None:
        self.assertEqual(self.findings_for("int* a = new int(1);\n", "tests/architecture/sanitizer_canary/a.cpp"), [])

    def test_the_allocation_counter_may_replace_the_allocation_functions(self) -> None:
        code = (
            "void* operator new(std::size_t size) { return std::malloc(size); }\n"
            "void operator delete(void* pointer) noexcept { std::free(pointer); }\n"
        )
        self.assertEqual(self.findings_for(code, f"{ALLOCATION_COUNTER}/AllocationCounter.cpp"), [])

    def test_the_allocation_counter_follows_every_other_rule(self) -> None:
        cases = (
            ("void f() { strcpy(a, b); }\n", "C string functions"),
            ("auto a = rand();\n", "INV-05"),
            ("auto a = std::chrono::steady_clock::now();\n", "INV-05"),
            ("auto p = reinterpret_cast<const H*>(bytes);\n", "INV-12"),
        )
        for code, expected in cases:
            with self.subTest(code=code):
                self.assert_refused(self.findings_for(code, f"{ALLOCATION_COUNTER}/AllocationCounter.cpp"), expected)

    def test_the_memory_exemption_does_not_reach_other_folders(self) -> None:
        for path in ("tests/architecture/frame_allocations_other/a.cpp", "tests/architecture/a.cpp",
                     "tests/frame_allocations/a.cpp", "Frameworks/Classic/Tests/frame_allocations/a.cpp"):
            with self.subTest(path=path):
                self.assert_refused(self.findings_for("int* a = new int(1);\n", path), "'new' is forbidden")


if __name__ == "__main__":
    unittest.main()
