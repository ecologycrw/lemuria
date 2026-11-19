# A minimal server used only by the tests and the demo. It is not a product (docs/adr/0003-engine-so-de-cliente.md).
# The rules live in a library, so the tests can use them without any connection.
lemuria_add_library(test_server_lib
    SOURCES Private/ServerGrid.cpp Private/ServerMotion.cpp Private/ServerWorld.cpp Private/WalkCheck.cpp
    PUBLIC_DEPS demo_protocol lemuria_core)

lemuria_add_program(test_server
    SOURCES Private/Main.cpp
    DEPS test_server_lib)

add_test(NAME test_server_starts COMMAND test_server)
set_tests_properties(test_server_starts PROPERTIES PASS_REGULAR_EXPRESSION "Lemuria test server: protocol 1")

add_subdirectory(Tests)
