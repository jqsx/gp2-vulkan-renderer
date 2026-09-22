//
// Created by frane on 4/21/2026.
//

#ifndef GPVKFR_RENDERPASS_H
#define GPVKFR_RENDERPASS_H
#include <vulkan/vulkan.h>
#include "structs.h"

class RenderPass {
    VkRenderPassCreateInfo m_RenderPassCreateInfo{};
    VkRenderPass m_RenderPass{};
    VkAttachmentDescription m_ColorAttachment = {};
    VkAttachmentReference m_ColorAttachmentReference = {};
    VkSubpassDescription m_Subpass = {};
    VkSubpassDependency m_Dependency{};

    VkAttachmentDescription m_DepthAttachment{};
    VkAttachmentReference m_DepthAttachmentReference{};

    std::vector<VkFramebuffer> m_FrameBuffers;

public:
    RenderPass();
    ~RenderPass();

    // Helper functions

    VkRenderPassCreateInfo& GetCreateInfo() { return m_RenderPassCreateInfo; }

    void AllocateFrameBuffers(const std::vector<VkImageView>& swapChainImageViews, VkExtent2D extent);

    InitResult Create();
    void Destroy();

    VkFramebuffer GetFrameBuffer(uint32_t index) { return m_FrameBuffers[index]; }

    VkRenderPass GetRenderPass() const { return m_RenderPass; }
};


#endif //GPVKFR_RENDERPASS_H