set(HEADER_FILES Application.h Entrypoint.h Image.h Random.h Layer.h Timer.h include/logging.h assets/RobotoMedium.h)
set(CPP_FILES Application.cpp Image.cpp Random.cpp)

add_library(Walnut STATIC ${HEADER_FILES} ${CPP_FILES} ${IMGUI_SOURCES})

target_include_directories(Walnut
    PUBLIC
        ${Walnut_SOURCE_DIR}
        ${IMGUI_INCLUDE_DIRECTORY}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_link_libraries(Walnut PUBLIC glm::glm Vulkan::Vulkan PRIVATE glfw spdlog::spdlog_header_only)
