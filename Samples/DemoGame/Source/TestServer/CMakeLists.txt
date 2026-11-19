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

add_subdirectory(Tests)
