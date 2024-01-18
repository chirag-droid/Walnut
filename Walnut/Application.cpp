// Adapted from Dear ImGui Vulkan example

#include "Walnut/Application.h"
#include "logging.h"

#include "vulkan/vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <glm/glm.hpp>

#include <map>
#include <optional>
#include <set>
#include <vector>

// Emedded font
#include "RobotoMedium.h"

extern bool g_ApplicationRunning;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE
#if defined(_DEBUG) && !defined(WALNUT_DEBUG_REPORT)
#define WALNUT_DEBUG_REPORT
#endif

struct QueueFamilyIndices{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> g_ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT g_DebugMessenger = VK_NULL_HANDLE;
static QueueFamilyIndices       g_Indices;
static VkQueue                  g_GraphicsQueue = VK_NULL_HANDLE;
static VkQueue                  g_PresentQueue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
static VkSurfaceKHR             g_Surface = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int                      g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

// Per-frame-in-flight
static std::vector<std::vector<VkCommandBuffer>> s_AllocatedCommandBuffers;
static std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

// Unlike g_MainWindowData.FrameIndex, this is not the the swapchain image index
// and is always guaranteed to increase (eg. 0, 1, 2, 0, 1, 2)
static uint32_t s_CurrentFrameIndex = 0;

static Walnut::Application* s_Instance = nullptr;

void CheckVkResult(VkResult err)
{
    if (err == 0)
        return;

    spdlog::get("Vulkan")->error("VkResult = {}", err);
    if (err < 0)
        abort();
}

#ifdef WALNUT_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::map<VkDebugUtilsMessageSeverityFlagBitsEXT, spdlog::level::level_enum> level;
    level[VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT] = spdlog::level::debug;
    level[VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT] = spdlog::level::info;
    level[VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT] = spdlog::level::warn;
    level[VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT] = spdlog::level::err;

    spdlog::get("Vulkan")->log(level[messageSeverity], "{}", pCallbackData->pMessage);

    return VK_FALSE;
}
#endif

static bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void CreateVulkanInstance(std::vector<const char*> instanceExtensions)
{
    VkResult err;

    // Application Info
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Walnut";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Instance Create Info
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;

    // Enumerate available extensions
    uint32_t propertiesCount;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, nullptr);
    properties.resize(propertiesCount);
    err = vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, properties.data());
    CheckVkResult(err);

    // Enable required extensions
    if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    // Enable validation layers
#ifdef WALNUT_DEBUG_REPORT
    createInfo.enabledLayerCount = (uint32_t)g_ValidationLayers.size();
    createInfo.ppEnabledLayerNames = g_ValidationLayers.data();
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // Setup debug messenger callback
#ifdef WALNUT_DEBUG_REPORT
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = &debugCreateInfo;
#endif

    // Create Vulkan Instance
    createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    err = vkCreateInstance(&createInfo, g_Allocator, &g_Instance);
    CheckVkResult(err);
};

#ifdef WALNUT_DEBUG_REPORT
static void SetupDebugCallback()
{
    // Setup the debug report callback
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugUtilsMessengerEXT");
    IM_ASSERT(vkCreateDebugUtilsMessengerEXT != nullptr);
    VkDebugUtilsMessengerCreateInfoEXT utilsMessengerCi = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    utilsMessengerCi.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    utilsMessengerCi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    utilsMessengerCi.pfnUserCallback = debugCallback;
    utilsMessengerCi.pUserData = nullptr;
    CheckVkResult(vkCreateDebugUtilsMessengerEXT(g_Instance, &utilsMessengerCi, g_Allocator, &g_DebugMessenger));
}
#endif

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    std::vector<VkQueueFamilyProperties> queueFamilies;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    VkBool32 presentSupport = false;

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_Surface, &presentSupport);

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
        if (presentSupport) indices.presentFamily = i;

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(g_DeviceExtensions.begin(), g_DeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_Surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_Surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_Surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_Surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

static bool IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

static void PickPhysicalDevice()
{
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, NULL);
    if (deviceCount <= 0)
        spdlog::get("Vulkan")->error("failed to find GPUs with Vulkan support!");
    IM_ASSERT(deviceCount > 0);

    std::vector<VkPhysicalDevice> devices;
    devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, devices.data());

    for (const auto& device: devices)
    {
        if (IsDeviceSuitable(device)) {
            g_PhysicalDevice = device;
            g_Indices = FindQueueFamilies(g_PhysicalDevice);
            break;
        }
    }

    if (g_PhysicalDevice == VK_NULL_HANDLE)
        spdlog::get("")->error("failed to find suitable GPU!");
}

static void CreateLogicalDevice()
{
    VkResult err;

    std::vector<const char*> deviceExtensions;

    for (auto const& deviceExtension: g_DeviceExtensions) {
        deviceExtensions.push_back(deviceExtension);
    }

    // Enumerate physical device extension
    uint32_t propertiesCount;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &propertiesCount, nullptr);
    properties.resize(propertiesCount);
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &propertiesCount, properties.data());
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {g_Indices.graphicsFamily.value(), g_Indices.presentFamily.value()};

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    err = vkCreateDevice(g_PhysicalDevice, &createInfo, g_Allocator, &g_Device);
    CheckVkResult(err);

    vkGetDeviceQueue(g_Device, g_Indices.graphicsFamily.value(), 0, &g_GraphicsQueue);
    vkGetDeviceQueue(g_Device, g_Indices.presentFamily.value(), 0, &g_PresentQueue);
}

static void CreateDescriptorPool()
{
    VkResult err;

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
    CheckVkResult(err);
}


// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, int width, int height)
{
    wd->Surface = g_Surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_Indices.graphicsFamily.value(), wd->Surface, &res);
    if (res != VK_TRUE)
    {
        spdlog::get("Vulkan")->error("no WSI support on physical device 0");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_Indices.graphicsFamily.value(), g_Allocator, width, height, g_MinImageCount);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef WALNUT_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(g_Instance, g_DebugMessenger, g_Allocator);
#endif // WALNUT_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkResult err;

    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    CheckVkResult(err);

    s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % g_MainWindowData.ImageCount;

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        CheckVkResult(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        CheckVkResult(err);
    }

    {
        // Free resources in queue
        for (auto& func : s_ResourceFreeQueue[s_CurrentFrameIndex])
            func();
        s_ResourceFreeQueue[s_CurrentFrameIndex].clear();
    }
    {
        // Free command buffers allocated by Application::GetCommandBuffer
        // These use g_MainWindowData.FrameIndex and not s_CurrentFrameIndex because they're tied to the swapchain image index
        auto& allocatedCommandBuffers = s_AllocatedCommandBuffers[wd->FrameIndex];
        if (allocatedCommandBuffers.size() > 0)
        {
            vkFreeCommandBuffers(g_Device, fd->CommandPool, (uint32_t)allocatedCommandBuffers.size(), allocatedCommandBuffers.data());
            allocatedCommandBuffers.clear();
        }

        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        CheckVkResult(err);
        VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        CheckVkResult(err);
    }
    {
        VkRenderPassBeginInfo info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        CheckVkResult(err);
        err = vkQueueSubmit(g_GraphicsQueue, 1, &info, fd->Fence);
        CheckVkResult(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_PresentQueue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    CheckVkResult(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char* description)
{
    spdlog::get("GLFW")->error("{}: {}", error, description);
}

namespace Walnut {
    Application::Application(const ApplicationSpecification& specification)
        : m_Specification(specification)
    {
        s_Instance = this;

        Init();
    }

    Application::~Application()
    {
        Shutdown();

        s_Instance = nullptr;
    }

    Application& Application::Get()
    {
        return *s_Instance;
    }

    void Application::Init()
    {

#ifdef _DEBUG
        spdlog::set_level(spdlog::level::info);
#else
        spdlog::set_level(spdlog::level::warn);
#endif

        // Setup GLFW window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            spdlog::get("Walnut")->error("Could not initialize GLFW!\n");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_WindowHandle = glfwCreateWindow(m_Specification.Width, m_Specification.Height, m_Specification.Name.c_str(), NULL, NULL);

        // Setup Vulkan
        if (!glfwVulkanSupported())
        {
            spdlog::get("GLFW")->error("Vulkan not supported!");
            return;
        }

        std::vector<const char*> extensions;

        // glfw required extensions
        uint32_t extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        for (uint32_t i = 0; i < extensions_count; i++)
            extensions.push_back(glfw_extensions[i]);

        CreateVulkanInstance(extensions);

        ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
            return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
        }, &g_Instance);

#ifdef WALNUT_DEBUG_REPORT
        SetupDebugCallback();
#endif

        // Create Window Surface
        VkResult err = glfwCreateWindowSurface(g_Instance, m_WindowHandle, g_Allocator, &g_Surface);
        CheckVkResult(err);

        PickPhysicalDevice();
        CreateLogicalDevice();

        CreateDescriptorPool();

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(m_WindowHandle, &w, &h);
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        SetupVulkanWindow(wd, w, h);

        s_AllocatedCommandBuffers.resize(wd->ImageCount);
        s_ResourceFreeQueue.resize(wd->ImageCount);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_WindowHandle, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = g_Instance;
        init_info.PhysicalDevice = g_PhysicalDevice;
        init_info.Device = g_Device;
        init_info.QueueFamily = g_Indices.graphicsFamily.value();
        init_info.Queue = g_GraphicsQueue;
        init_info.PipelineCache = g_PipelineCache;
        init_info.DescriptorPool = g_DescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_MinImageCount;
        init_info.ImageCount = wd->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = g_Allocator;
        init_info.CheckVkResultFn = CheckVkResult;
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

        // Load default font
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        ImFont* robotoFont = io.Fonts->AddFontFromMemoryCompressedTTF((void*)RobotoMedium_compressed_data, RobotoMedium_compressed_size, 20.0f, &fontConfig);
        io.FontDefault = robotoFont;
    }

    void Application::Shutdown()
    {
        for (auto& layer : m_LayerStack)
            layer->OnDetach();

        m_LayerStack.clear();

        // Cleanup
        VkResult err = vkDeviceWaitIdle(g_Device);
        CheckVkResult(err);

        // Free resources in queue
        for (auto& queue : s_ResourceFreeQueue)
        {
            for (auto& func : queue)
                func();
        }
        s_ResourceFreeQueue.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        CleanupVulkanWindow();
        CleanupVulkan();

        glfwDestroyWindow(m_WindowHandle);
        glfwTerminate();

        g_ApplicationRunning = false;
    }

    void Application::Run()
    {
        m_Running = true;

        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        ImGuiIO& io = ImGui::GetIO();

        // Main loop
        while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            for (auto& layer : m_LayerStack)
                layer->OnUpdate(m_TimeStep);

            // Resize swap chain?
            if (g_SwapChainRebuild)
            {
                int width, height;
                glfwGetFramebufferSize(m_WindowHandle, &width, &height);
                if (width > 0 && height > 0)
                {
                    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_Indices.graphicsFamily.value(), g_Allocator, width, height, g_MinImageCount);
                    g_MainWindowData.FrameIndex = 0;

                    // Clear allocated command buffers from here since entire pool is destroyed
                    s_AllocatedCommandBuffers.clear();
                    s_AllocatedCommandBuffers.resize(g_MainWindowData.ImageCount);

                    g_SwapChainRebuild = false;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            {
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

                // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
                // because it would be confusing to have two docking targets within each others.
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
                if (m_MenubarCallback)
                    window_flags |= ImGuiWindowFlags_MenuBar;

                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
                // and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;

                // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
                // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
                // all active windows docked into it will lose their parent and become undocked.
                // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
                // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("DockSpace Demo", nullptr, window_flags);
                ImGui::PopStyleVar();

                ImGui::PopStyleVar(2);

                // Submit the DockSpace
                ImGuiIO& io = ImGui::GetIO();
                if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
                {
                    ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
                    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                }

                if (m_MenubarCallback)
                {
                    if (ImGui::BeginMenuBar())
                    {
                        m_MenubarCallback();
                        ImGui::EndMenuBar();
                    }
                }

                for (auto& layer : m_LayerStack)
                    layer->OnUIRender();

                ImGui::End();
            }

            // Rendering
            ImGui::Render();
            ImDrawData* main_draw_data = ImGui::GetDrawData();
            const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            if (!main_is_minimized)
                FrameRender(wd, main_draw_data);

            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present Main Platform Window
            if (!main_is_minimized)
                FramePresent(wd);

            float time = GetTime();
            m_FrameTime = time - m_LastFrameTime;
            m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
            m_LastFrameTime = time;
        }

    }

    void Application::Close()
    {
        m_Running = false;
    }

    float Application::GetTime()
    {
        return (float)glfwGetTime();
    }

    VkInstance Application::GetInstance()
    {
        return g_Instance;
    }

    VkPhysicalDevice Application::GetPhysicalDevice()
    {
        return g_PhysicalDevice;
    }

    VkDevice Application::GetDevice()
    {
        return g_Device;
    }

    VkCommandBuffer Application::GetCommandBuffer(bool begin)
    {
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

        // Use any command queue
        VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdBufAllocateInfo.commandPool = command_pool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer& command_buffer = s_AllocatedCommandBuffers[wd->FrameIndex].emplace_back();
        auto err = vkAllocateCommandBuffers(g_Device, &cmdBufAllocateInfo, &command_buffer);

        VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        err = vkBeginCommandBuffer(command_buffer, &begin_info);
        CheckVkResult(err);

        return command_buffer;
    }

    void Application::FlushCommandBuffer(VkCommandBuffer commandBuffer)
    {
        const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

        VkSubmitInfo end_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &commandBuffer;
        auto err = vkEndCommandBuffer(commandBuffer);
        CheckVkResult(err);

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = 0;
        VkFence fence;
        err = vkCreateFence(g_Device, &fenceCreateInfo, nullptr, &fence);
        CheckVkResult(err);

        err = vkQueueSubmit(g_GraphicsQueue, 1, &end_info, fence);
        CheckVkResult(err);

        err = vkWaitForFences(g_Device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        CheckVkResult(err);

        vkDestroyFence(g_Device, fence, nullptr);
    }


    void Application::SubmitResourceFree(std::function<void()>&& func)
    {
        s_ResourceFreeQueue[s_CurrentFrameIndex].emplace_back(func);
    }

}
