lemuria_add_library(demo_protocol
    SOURCES Private/DemoMap.cpp Private/Framing.cpp Private/MessageCodec.cpp Private/Protocol.cpp
    PUBLIC_DEPS lemuria_core)

add_subdirectory(Tests)
