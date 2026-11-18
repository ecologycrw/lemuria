cmake_minimum_required(VERSION 3.30)

project(Lemuria VERSION 0.0.0 LANGUAGES CXX)

# The project does not use C++20 modules (docs/adr/0001-cpp20-cmake-vcpkg.md).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LemuriaSanitizers)
include(LemuriaModule)

find_package(SDL3 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

enable_testing()

add_subdirectory(Engine/Source/Runtime/Core)
add_subdirectory(Engine/Source/Runtime/PlatformSDL)
add_subdirectory(Frameworks/Classic)
add_subdirectory(Samples/DemoGame)
add_subdirectory(tests)
