# A minimal server used only by the tests and the demo. It is not a product (docs/adr/0003-engine-so-de-cliente.md).
# The rules and the connections live in a library, so the tests can use them without starting the program.
lemuria_add_library(test_server_lib
    SOURCES
        Private/ServerGrid.cpp
        Private/ServerHost.cpp
        Private/ServerMotion.cpp
        Private/ServerWorld.cpp
        Private/WalkCheck.cpp
    PUBLIC_DEPS demo_protocol lemuria_core lemuria_platform_sdl)

lemuria_add_program(test_server
    SOURCES Private/Main.cpp
    DEPS test_server_lib)

add_test(NAME test_server_starts COMMAND test_server --version)
set_tests_properties(test_server_starts PROPERTIES PASS_REGULAR_EXPRESSION "Lemuria test server: protocol 1")

add_test(NAME test_server_refuses_a_wrong_port COMMAND test_server --port 70000)
set_tests_properties(test_server_refuses_a_wrong_port PROPERTIES WILL_FAIL TRUE)

# Every wrong command line is refused with the usage, instead of starting a server (INV-12). A server that
# starts anyway never ends, so the time limit fails the test.
set(wrong_command_lines "--port 0" "--port 65536" "--port 24a00" "--port -1" "--port" "--speed 2")
set(command_line_number 0)
foreach(command_line IN LISTS wrong_command_lines)
    math(EXPR command_line_number "${command_line_number} + 1")
    separate_arguments(arguments NATIVE_COMMAND "${command_line}")
    add_test(NAME test_server_refuses_command_line_${command_line_number} COMMAND test_server ${arguments})
    set_tests_properties(test_server_refuses_command_line_${command_line_number} PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage: test_server" TIMEOUT 10)
endforeach()

add_subdirectory(Tests)
