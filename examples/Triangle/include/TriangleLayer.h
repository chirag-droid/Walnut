#pragma once

#include "Walnut/Application.h"
#include "Walnut/Image.h"
#include "Walnut/Layer.h"

#include "vulkan/vulkan.h"

#include <memory>

class TriangleLayer : public Walnut::Layer {
public:
    TriangleLayer() : m_Device(VK_NULL_HANDLE) {}
    void Render();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffer();

    virtual void OnAttach() override;
    virtual void OnUIRender() override;
    virtual void OnDetach() override;
private:
    VkDevice m_Device;
    float m_ClearColor[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
    std::shared_ptr<Walnut::Image> m_Image = nullptr;
    Walnut::ImageFormat m_ImageFormat = Walnut::ImageFormat::RGBA;
    uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

    float m_LastRenderTime = 0.0f;
};
