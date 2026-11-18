lemuria_add_library(demo_client_lib
    SOURCES Private/GroundScene.cpp Private/ProofScene.cpp
    PUBLIC_DEPS lemuria_classic lemuria_core demo_protocol)

lemuria_add_program(demo_client
    SOURCES Private/Main.cpp
    DEPS demo_client_lib lemuria_platform_sdl)

add_test(NAME demo_client_starts COMMAND demo_client --version)
set_tests_properties(demo_client_starts PROPERTIES PASS_REGULAR_EXPRESSION "Lemuria Demo: engine 0\\.0\\.0")

add_subdirectory(Tests)
