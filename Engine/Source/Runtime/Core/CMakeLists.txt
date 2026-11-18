lemuria_add_library(lemuria_core
    SOURCES Private/Version.cpp
    PUBLIC_DEPS tl::expected)

target_compile_definitions(lemuria_core PRIVATE
    LEMURIA_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    LEMURIA_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    LEMURIA_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

add_subdirectory(Tests)
