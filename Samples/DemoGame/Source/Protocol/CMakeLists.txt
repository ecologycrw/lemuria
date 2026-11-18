lemuria_add_library(demo_protocol
    SOURCES Private/DemoMap.cpp Private/Protocol.cpp
    PRIVATE_DEPS lemuria_core)

add_subdirectory(Tests)
