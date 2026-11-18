# A minimal server used only by the tests and the demo. It is not a product (docs/adr/0003-engine-so-de-cliente.md).
lemuria_add_program(test_server
    SOURCES src/main.cpp
    DEPS demo_protocol)

add_test(NAME test_server_starts COMMAND test_server)
set_tests_properties(test_server_starts PROPERTIES PASS_REGULAR_EXPRESSION "Lemuria test server: protocol 1")
