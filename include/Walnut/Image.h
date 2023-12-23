#pragma once

#include <string>

#include "vulkan/vulkan.h"

namespace Walnut {
    enum class ImageFormat
    {
        None = 0,
        RGBA,
        RGBA32F
    };

    namespace Utils {
        static VkFormat WalnutFormatToVulkanFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            return (VkFormat)0;
        }
    }

    class Image
    {
    public:
        Image(uint32_t width, uint32_t height, ImageFormat format);
        ~Image();

        void Resize(uint32_t width, uint32_t height);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        VkFormat GetImageFormat();
        VkImageView GetImageView() const { return m_ImageView; }
        VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
    private:
        void AllocateMemory(uint64_t size);
        void Release();
    private:
        uint32_t m_Width = 0, m_Height = 0;

        VkImage m_Image = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

        ImageFormat m_Format = ImageFormat::None;

        VkBuffer m_StagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_StagingBufferMemory = VK_NULL_HANDLE;

        size_t m_AlignedSize = 0;

        std::string m_Filepath;
    };
}
