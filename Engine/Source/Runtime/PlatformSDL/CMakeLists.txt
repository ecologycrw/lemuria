# The only target allowed to use SDL (INV-09). SDL stays private: nothing else can see it.
lemuria_add_library(lemuria_platform_sdl
    SOURCES
        Private/MonotonicClock.cpp
        Private/PlatformInfo.cpp
        Private/Window.cpp
    PUBLIC_DEPS lemuria_core
    PRIVATE_DEPS SDL3::SDL3)

add_subdirectory(Tests)
