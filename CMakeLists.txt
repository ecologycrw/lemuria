cmake_minimum_required(VERSION 3.30)

project(Lemuria VERSION 0.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LemuriaSanitizers)
include(LemuriaModule)

find_package(SDL3 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

enable_testing()

add_subdirectory(src/core/lemuria_core)
add_subdirectory(src/core/lemuria_platform_sdl)
add_subdirectory(src/framework/lemuria_classic)
add_subdirectory(samples/demo_protocol)
add_subdirectory(samples/demo_client)
add_subdirectory(tools/test_server)
add_subdirectory(tests)
