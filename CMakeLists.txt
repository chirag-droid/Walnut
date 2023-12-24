cmake_minimum_required(VERSION 3.16)

# -------------------- Base Project Setup --------------------
project(Walnut VERSION 0.1 DESCRIPTION "A fork of StudioCherno/Walnut application framework")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_STANDARD 17)
# ------------------------------------------------------------

# -------------------- Dependencies --------------------------
find_package(Vulkan COMPONENTS glslc)

# if Vulkan not found prompt the user to install it
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Please install Vulkan SDK. https://vulkan.lunarg.com/sdk/home")
endif()

# glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target" FORCE)
add_subdirectory(vendor/glfw EXCLUDE_FROM_ALL)

# glm
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)

# Dear ImGui
set(IMGUI_DIR ${Walnut_SOURCE_DIR}/vendor/ImGui)
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
set(IMGUI_INCLUDE_DIRECTORY ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# spdlog
add_subdirectory(vendor/spdlog)
# ------------------------------------------------------------

add_subdirectory(Walnut)

set(WALNUT_BUILD_EXAMPLES ON CACHE BOOL "Build the Walnut example programs" FORCE)
if (WALNUT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
