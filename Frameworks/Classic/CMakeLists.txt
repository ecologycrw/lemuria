lemuria_add_library(lemuria_classic
    SOURCES Private/FrameworkInfo.cpp Private/ThreeQuarterCamera.cpp
    PUBLIC_DEPS lemuria_core)

add_subdirectory(Tests)
