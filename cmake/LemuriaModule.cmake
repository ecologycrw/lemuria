# The only way to declare targets in this repository, so every target gets the same rules.
# This file is a protected "trava" (see docs/WORKFLOW.md).

# C++20 without compiler extensions, with every warning treated as an error (INV-10).
function(lemuria_apply_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX /permissive- /utf-8 /Zc:__cplusplus /external:anglebrackets /external:W0)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror)
    endif()
endfunction()

# lemuria_add_library(name SOURCES ... [PUBLIC_DEPS ...] [PRIVATE_DEPS ...])
function(lemuria_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS")
    add_library(${name} STATIC ${ARG_SOURCES})
    target_include_directories(${name} PUBLIC include)
    target_link_libraries(${name} PUBLIC ${ARG_PUBLIC_DEPS} PRIVATE ${ARG_PRIVATE_DEPS})
    lemuria_apply_defaults(${name})
endfunction()

# lemuria_add_program(name SOURCES ... [DEPS ...])
function(lemuria_add_program name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPS")
    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    lemuria_apply_defaults(${name})
endfunction()

# lemuria_add_tests(name SOURCES ... [DEPS ...] [LABELS ...])
function(lemuria_add_tests name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPS;LABELS")
    lemuria_add_program(${name} SOURCES ${ARG_SOURCES} DEPS ${ARG_DEPS} Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${name})
    if(ARG_LABELS)
        set_tests_properties(${name} PROPERTIES LABELS "${ARG_LABELS}")
    endif()
endfunction()
