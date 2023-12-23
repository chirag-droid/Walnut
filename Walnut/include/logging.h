#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

inline auto vulkan_logger = spdlog::stdout_color_mt("Vulkan");
inline auto glfw_logger = spdlog::stdout_color_mt("GLFW");
inline auto walnut_logger = spdlog::stdout_color_mt("Walnut");
