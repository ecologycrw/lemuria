lemuria_add_library(lemuria_classic
    SOURCES
        Private/AdapterCapabilities.cpp
        Private/FrameworkInfo.cpp
        Private/GameSession.cpp
        Private/PathFinder.cpp
        Private/ThreeQuarterCamera.cpp
        Private/TileGrid.cpp
        Private/World.cpp
    PUBLIC_DEPS lemuria_core)

add_subdirectory(Tests)
