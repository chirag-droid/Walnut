add_library(Walnut STATIC
                   "${Walnut_SOURCE_DIR}/include/Walnut/Application.h"
                   "${Walnut_SOURCE_DIR}/include/Walnut/Entrypoint.h"
                   "${Walnut_SOURCE_DIR}/include/Walnut/Image.h"
                   "${Walnut_SOURCE_DIR}/include/Walnut/Layer.h"
                   "${Walnut_SOURCE_DIR}/include/Walnut/Timer.h"
                   Application.cpp Image.cpp Random.cpp include/logging.h
                   assets/RobotoMedium.h
                   ${IMGUI_SOURCES})

target_include_directories(Walnut
    PUBLIC
        ${Walnut_SOURCE_DIR}/include
        ${IMGUI_INCLUDE_DIRECTORY}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_link_libraries(Walnut PUBLIC glm::glm Vulkan::Vulkan PRIVATE glfw spdlog::spdlog_header_only)
