# Dear ImGui
add_library(ImGui INTERFACE)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
set(IMGUI_SOURCES
    # general stuff
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui.h

    # integration with glfw and vulkan
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_vulkan.cpp
)
list(TRANSFORM IMGUI_SOURCES PREPEND ${IMGUI_DIR}/)

target_sources(ImGui INTERFACE ${IMGUI_SOURCES})
target_include_directories(ImGui INTERFACE ${IMGUI_DIR})

set_target_properties(ImGui PROPERTIES FOLDER "Vendor")

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target" FORCE)
add_subdirectory(glfw)

set_property(TARGET glfw PROPERTY FOLDER "Vendor")

# glm
add_subdirectory(glm)

# fmt
add_subdirectory(fmt)

# spdlog
add_subdirectory(spdlog)
option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON)
option(SPDLOG_FMT_EXTERNAL_HO "Use external fmt header-only library instead of bundled" ON)
set_property(TARGET spdlog PROPERTY FOLDER "Vendor")

if (WALNUT_BUILD_EXAMPLES)
    # glslang
    option(ENABLE_SPVREMAPPER OFF)
    option(ENABLE_GLSLANG_BINARIES OFF)
    set(ENABLE_HLSL ON)
    option(ENABLE_OPT OFF)
    option(BUILD_TESTING OFF)
    option(BUILD_EXTERNAL OFF)

    add_subdirectory(glslang)

    set_target_properties(glslang PROPERTIES FOLDER "Vendor")
    set_target_properties(OGLCompiler PROPERTIES FOLDER "Vendor")
    set_target_properties(OSDependent PROPERTIES FOLDER "Vendor")
    set_target_properties(SPIRV PROPERTIES FOLDER "Vendor")
    set_target_properties(HLSL PROPERTIES FOLDER "Vendor")

    # Spirv-Tools
    add_subdirectory(spirv-headers)

    set(SKIP_SPIRV_TOOLS_INSTALL ON)
    set(SPIRV_SKIP_EXECUTABLES ON)
    set(SPIRV_SKIP_TESTS ON)

    add_subdirectory(spirv-tools)

    set_target_properties(SPIRV-Tools-opt PROPERTIES FOLDER "Vendor")

    # ShaderC
    set(SHADERC_SKIP_EXAMPLES ON)
    set(SHADERC_SKIP_TESTS ON)
    set(SHADERC_SKIP_COPYRIGHT_CHECK ON)
    set(SHADERC_SKIP_INSTALL ON)
    set(SHADERC_ENABLE_SHARED_CRT ON)
    add_subdirectory(shaderc)

    set_target_properties(shaderc PROPERTIES FOLDER "Vendor")
    set_target_properties(shaderc_util PROPERTIES FOLDER "Vendor")
    set_target_properties(shaderc_shared PROPERTIES FOLDER "Vendor")
    set_target_properties(glslc PROPERTIES FOLDER "Vendor")
    set_target_properties(glslc_exe PROPERTIES FOLDER "Vendor")
endif()
