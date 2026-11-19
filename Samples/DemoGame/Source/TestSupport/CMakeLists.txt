# Pieces shared by the tests of the demonstration that use real connections. Only the tests link it.
lemuria_add_library(demo_test_support
    SOURCES Private/RealNetwork.cpp
    PRIVATE_DEPS lemuria_platform_sdl)
