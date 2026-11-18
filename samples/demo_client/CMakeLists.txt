lemuria_add_program(demo_client
    SOURCES src/main.cpp
    DEPS lemuria_classic lemuria_platform_sdl demo_protocol)

add_test(NAME demo_client_starts COMMAND demo_client)
set_tests_properties(demo_client_starts PROPERTIES PASS_REGULAR_EXPRESSION "Lemuria Demo: engine 0\\.0\\.0")
