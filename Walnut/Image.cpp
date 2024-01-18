#include "Walnut/Application.h"
#include "Walnut/Image.h"

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

namespace Walnut {
    namespace Utils {
        static uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
        {
            VkPhysicalDeviceMemoryProperties prop;
            vkGetPhysicalDeviceMemoryProperties(Application::GetPhysicalDevice(), &prop);
            for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
            {
                if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
                    return i;
            }

            return 0xffffffff;
        }

        static uint32_t BytesPerPixel(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::RGBA:    return 4;
                case ImageFormat::RGBA32F: return 16;
            }
            return 0;
        }
    }

    Image::Image(uint32_t width, uint32_t height, ImageFormat format)
        : m_Width(width), m_Height(height), m_Format(format)
    {
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
    }

    Image::~Image()
    {
        Release();
    }

    void Image::AllocateMemory(uint64_t size)
    {
        VkDevice device = Application::GetDevice();

        VkResult err;

        VkFormat vulkanFormat = Utils::WalnutFormatToVulkanFormat(m_Format);

        // Create the Image
        {
            VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = vulkanFormat;
            info.extent.width = m_Width;
            info.extent.height = m_Height;
            info.extent.depth = 1;
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            err = vkCreateImage(device, &info, nullptr, &m_Image);
            CheckVkResult(err);
            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(device, m_Image, &req);
            VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
            err = vkAllocateMemory(device, &alloc_info, nullptr, &m_Memory);
            CheckVkResult(err);
            err = vkBindImageMemory(device, m_Image, m_Memory, 0);
            CheckVkResult(err);
        }

        // Create the Image View:
        {
            VkImageViewCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            info.image = m_Image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = vulkanFormat;
            info.subresourceRange = {};
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;
            err = vkCreateImageView(device, &info, nullptr, &m_ImageView);
            CheckVkResult(err);
        }

        // Create sampler:
        {
            VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = info.addressModeU;
            info.addressModeW = info.addressModeU;
            info.mipLodBias = 0.0f;
            info.maxAnisotropy = 1.0f;
            info.minLod = 0.0f;
            info.maxLod = 1.0f;
            info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VkResult err = vkCreateSampler(device, &info, nullptr, &m_Sampler);
            CheckVkResult(err);
        }

        // Create the Descriptor Set:
        m_DescriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void Image::Release()
    {
        Application::SubmitResourceFree([sampler = m_Sampler, imageView = m_ImageView, image = m_Image,
            memory = m_Memory, stagingBuffer = m_StagingBuffer, stagingBufferMemory = m_StagingBufferMemory]()
        {
            VkDevice device = Application::GetDevice();

            vkDestroySampler(device, sampler, nullptr);
            vkDestroyImageView(device, imageView, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, memory, nullptr);
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        });

        m_Sampler = VK_NULL_HANDLE;
        m_ImageView = VK_NULL_HANDLE;
        m_Image = VK_NULL_HANDLE;
        m_Memory = VK_NULL_HANDLE;
        m_StagingBuffer = VK_NULL_HANDLE;
        m_StagingBufferMemory = VK_NULL_HANDLE;
    }

    void Image::Resize(uint32_t width, uint32_t height)
    {
        if (m_Image && m_Width == width && m_Height == height)
            return;

        // TODO: max size?

        m_Width = width;
        m_Height = height;

        Release();
        AllocateMemory(m_Width * m_Height * Utils::BytesPerPixel(m_Format));
    }

    VkFormat Image::GetImageFormat() {
        return Walnut::Utils::WalnutFormatToVulkanFormat(m_Format);
    };
}
