set(HEADER_FILES Application.h Entrypoint.h Image.h Random.h Layer.h Timer.h include/logging.h assets/RobotoMedium.h)
set(CPP_FILES Application.cpp Image.cpp Random.cpp)

add_library(Walnut STATIC ${HEADER_FILES} ${CPP_FILES} ${IMGUI_SOURCES})

find_package(Vulkan)

# if Vulkan not found prompt the user to install it
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Please install Vulkan SDK. https://vulkan.lunarg.com/sdk/home")
endif()


target_include_directories(Walnut
    PUBLIC
        ${Walnut_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_link_libraries(Walnut
    PUBLIC
        glm::glm
        Vulkan::Vulkan
        ImGui
        glfw
        shaderc
    PRIVATE
        spdlog::spdlog_header_only
)
