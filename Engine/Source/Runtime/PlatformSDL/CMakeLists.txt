# The only target allowed to use SDL (INV-09). SDL stays private: nothing else can see it.
lemuria_add_library(lemuria_platform_sdl
    SOURCES
        Private/Image.cpp
        Private/MonotonicClock.cpp
        Private/PlatformInfo.cpp
        Private/Renderer.cpp
        Private/TcpConnections.cpp
        Private/TcpLink.cpp
        Private/TcpListener.cpp
        Private/TcpMailbox.cpp
        Private/Window.cpp
    PUBLIC_DEPS lemuria_core
    PRIVATE_DEPS SDL3::SDL3 $<IF:$<TARGET_EXISTS:SDL3_net::SDL3_net>,SDL3_net::SDL3_net,SDL3_net::SDL3_net-static>)

# Shaders are compiled while configuring, so the static analysis finds them without a full build
# (docs/adr/0019-shaders-em-hlsl.md).
set(LEMURIA_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/Engine/Shaders")
set(LEMURIA_SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Generated/Shaders")
file(MAKE_DIRECTORY "${LEMURIA_SHADER_OUTPUT}")

# vcpkg installs build tools under the triplet of the machine, which can differ from the triplet of the game.
file(GLOB LEMURIA_SHADERC_TOOLS "${VCPKG_INSTALLED_DIR}/*/tools/shaderc")
find_program(LEMURIA_GLSLC glslc
    HINTS ${LEMURIA_SHADERC_TOOLS}
    NO_DEFAULT_PATH)
if(NOT LEMURIA_GLSLC)
    message(FATAL_ERROR "glslc was not found. It is installed by the shaderc library in vcpkg.json.")
endif()

if(WIN32)
    file(TO_CMAKE_PATH "$ENV{WindowsSdkVerBinPath}" LEMURIA_WINDOWS_SDK_TOOLS)
    find_program(LEMURIA_DXC dxc
        HINTS "${LEMURIA_WINDOWS_SDK_TOOLS}/x64"
        NO_DEFAULT_PATH)
    if(NOT LEMURIA_DXC)
        message(FATAL_ERROR "dxc was not found in the Windows SDK. Install the SDK with the Visual Studio C++ tools "
                            "(docs/setup/DEV.md).")
    endif()
endif()

# Turns a compiled shader into a C++ array. Each line holds 32 bytes, because compilers limit the line length.
# The file is written only when its content changes, so configuring again does not rebuild the renderer.
function(lemuria_embed_shader binary symbol output)
    file(READ "${binary}" bytes HEX)
    string(REGEX REPLACE "(................................................................)" "\\1\n" bytes "${bytes}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${bytes}")
    set(content "inline constexpr unsigned char ${symbol}[] = {\n${bytes}\n};\n")
    if(EXISTS "${output}")
        file(READ "${output}" previous)
        if(previous STREQUAL content)
            return()
        endif()
    endif()
    file(WRITE "${output}" "${content}")
endfunction()

function(lemuria_run_shader_compiler source binary)
    if(EXISTS "${binary}" AND NOT "${source}" IS_NEWER_THAN "${binary}")
        return()
    endif()
    execute_process(COMMAND ${ARGN}
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE compiler_output
        ERROR_VARIABLE compiler_output)
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR "Could not compile ${source}:\n${compiler_output}")
    endif()
endfunction()

# lemuria_add_shader(file stage dxil_profile symbol)
function(lemuria_add_shader file stage dxil_profile symbol)
    set(source "${LEMURIA_SHADER_SOURCES}/${file}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")

    set(spirv "${LEMURIA_SHADER_OUTPUT}/${file}.spv")
    lemuria_run_shader_compiler("${source}" "${spirv}"
        "${LEMURIA_GLSLC}" -x hlsl -fshader-stage=${stage} -fentry-point=main --target-env=vulkan1.0 -Werror
        -o "${spirv}" "${source}")
    lemuria_embed_shader("${spirv}" "k${symbol}Spirv" "${LEMURIA_SHADER_OUTPUT}/${symbol}Spirv.inc")

    if(WIN32)
        set(dxil "${LEMURIA_SHADER_OUTPUT}/${file}.dxil")
        lemuria_run_shader_compiler("${source}" "${dxil}"
            "${LEMURIA_DXC}" -T ${dxil_profile} -E main -WX -Wno-ignored-attributes -Fo "${dxil}" "${source}")
        lemuria_embed_shader("${dxil}" "k${symbol}Dxil" "${LEMURIA_SHADER_OUTPUT}/${symbol}Dxil.inc")
    endif()
endfunction()

lemuria_add_shader(Shape.vert.hlsl vertex vs_6_0 ShapeVertex)
lemuria_add_shader(Shape.frag.hlsl fragment ps_6_0 ShapeFragment)

target_include_directories(lemuria_platform_sdl PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Generated")

add_subdirectory(Tests)
