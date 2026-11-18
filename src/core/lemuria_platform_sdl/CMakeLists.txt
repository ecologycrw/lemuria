# The only target allowed to use SDL (INV-09). SDL stays private: nothing else can see it.
lemuria_add_library(lemuria_platform_sdl
    SOURCES src/platform_info.cpp
    PUBLIC_DEPS lemuria_core
    PRIVATE_DEPS SDL3::SDL3)
