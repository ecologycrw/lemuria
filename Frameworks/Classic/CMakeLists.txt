lemuria_add_library(lemuria_classic
    SOURCES
        Private/FrameworkInfo.cpp
        Private/PathFinder.cpp
        Private/ThreeQuarterCamera.cpp
        Private/TileGrid.cpp
    PUBLIC_DEPS lemuria_core)

add_subdirectory(Tests)
