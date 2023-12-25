cmake_minimum_required(VERSION 3.16)

project(Walnut VERSION 0.1 DESCRIPTION "A fork of StudioCherno/Walnut application framework")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_STANDARD 17)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(WALNUT_BUILD_EXAMPLES ON CACHE BOOL "Build the Walnut example programs")
endif()

set(VK_WSI_SELECTION "XLIB" CACHE STRING "Select WSI target")
set_property(CACHE VK_WSI_SELECTION PROPERTY STRINGS XCB XLIB WAYLAND DirectFB)

add_subdirectory(vendor)

add_subdirectory(Walnut)
if (WALNUT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
