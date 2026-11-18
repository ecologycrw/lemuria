# Proves that a framework can be built with the Core alone, without Classic (INV-02, docs/adr/0002-tres-camadas.md).
# tools/checks refuses any dependency on Classic in this folder.
lemuria_add_library(custom_framework
    SOURCES Private/custom_framework.cpp
    PUBLIC_DEPS lemuria_core)

lemuria_add_tests(custom_framework_tests
    SOURCES custom_framework_test.cpp
    DEPS custom_framework)
