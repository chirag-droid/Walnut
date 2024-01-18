# Enables or disable Vulkan WSI extensions based on build system
add_library(platform_wsi INTERFACE)
if(WIN32)
    target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_METAL_EXT)
    if (IOS)
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_IOS_MVK)
    endif()
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_MACOS_MVK)
    endif()
elseif(UNIX)
    find_package(PkgConfig REQUIRED QUIET) # Use PkgConfig to find Linux system libraries

    if(VK_WSI_SELECTION STREQUAL XCB)
        pkg_check_modules(XCB REQUIRED QUIET IMPORTED_TARGET xcb)
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_XCB_KHR)
        target_link_libraries(platform_wsi INTERFACE PkgConfig::XCB)
    endif()
    if(VK_WSI_SELECTION STREQUAL X11)
        pkg_check_modules(X11 REQUIRED QUIET IMPORTED_TARGET x11)
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_XLIB_XRANDR_EXT)
        target_link_libraries(platform_wsi INTERFACE PkgConfig::X11)
    endif()
    if(VK_WSI_SELECTION STREQUAL WAYLAND)
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_WAYLAND_KHR)
    endif()
    if(VK_WSI_SELECTION STREQUAL DirectFB)
        pkg_check_modules(DirectFB QUIET REQUIRED IMPORTED_TARGET directfb)
        target_compile_definitions(platform_wsi INTERFACE VK_USE_PLATFORM_DIRECTFB_EXT)
        target_link_libraries(platform_wsi INTERFACE PkgConfig::DirectFB)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

# Vulkan Header Files
set(VULKAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
add_library(vulkan INTERFACE ${VULKAN_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan INTERFACE ${VULKAN_DIR})

target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)
target_link_libraries(vulkan INTERFACE platform_wsi)

set_target_properties(vulkan PROPERTIES FOLDER "Vendor")

# Volk - Vulkan Loader
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
set(VOLK_FILES
    "${VOLK_DIR}/volk.c"
    "${VOLK_DIR}/volk.h")

add_library(volk STATIC ${VOLK_FILES})
target_include_directories(volk PUBLIC ${VOLK_DIR})
target_link_libraries(volk PUBLIC vulkan)

set_target_properties(volk PROPERTIES FOLDER "Vendor")

# Dear ImGui
add_library(ImGui STATIC)

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

target_sources(ImGui PRIVATE ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_DIR})
target_link_libraries(ImGui PRIVATE glfw volk)

target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

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
