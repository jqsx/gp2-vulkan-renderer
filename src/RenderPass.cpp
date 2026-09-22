//
// Created by frane on 4/21/2026.
//

#include "RenderPass.h"

#include "Engine.h"
#include "Logger.h"

RenderPass::RenderPass() {

    // Provide a default configuration for the renderpass and
    // A separate function that gives access to these parameters
    // to change specific elements of the render pass instead of
    // reconfiguring with default values from scratch every time

    m_ColorAttachment.format = Engine::GetInstance()->GetSwapChainFormat();
    m_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    m_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    m_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    m_ColorAttachment.flags = 0;

    m_DepthAttachment.format = Engine::GetInstance()->GetDepthBuffer().GetFormat();
    m_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    m_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    m_DepthAttachmentReference.attachment = 1;
    m_DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    m_ColorAttachmentReference.attachment = 0;
    m_ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_Subpass.colorAttachmentCount = 1;
    m_Subpass.pColorAttachments = &m_ColorAttachmentReference;
    m_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    m_Subpass.pDepthStencilAttachment = &m_DepthAttachmentReference;

    m_RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    m_RenderPassCreateInfo.attachmentCount = 2;
    m_RenderPassCreateInfo.pAttachments = &m_ColorAttachment;
    m_RenderPassCreateInfo.subpassCount = 1;
    m_RenderPassCreateInfo.pSubpasses = &m_Subpass;

    m_Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    m_Dependency.dstSubpass = 0;

    m_Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    m_Dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    m_Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    m_Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    m_RenderPassCreateInfo.dependencyCount = 1;
    m_RenderPassCreateInfo.pDependencies = &m_Dependency;
}

RenderPass::~RenderPass() {
    Destroy();
}

void RenderPass::AllocateFrameBuffers(const std::vector<VkImageView> &swapChainImageViews, VkExtent2D extent) {
    if (m_FrameBuffers.size() > 0) {
        for (VkFramebuffer framebuffer : m_FrameBuffers)
            vkDestroyFramebuffer(Engine::GetInstance()->GetDevice(), framebuffer, nullptr);
    }

    m_FrameBuffers.resize(swapChainImageViews.size());
    for (int index = 0; index < swapChainImageViews.size(); ++index) {
        std::array<VkImageView, 2> attachments {
            swapChainImageViews[index],
            Engine::GetInstance()->GetDepthBuffer().GetImageView()
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(Engine::GetInstance()->GetDevice(), &framebufferInfo, nullptr, &m_FrameBuffers[index]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render framebuffer!");
        }
    }
}

InitResult RenderPass::Create() {
    m_RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; // just in case

    std::array<VkAttachmentDescription, 2> attachments = {m_ColorAttachment, m_DepthAttachment};
    m_RenderPassCreateInfo.attachmentCount = attachments.size();
    m_RenderPassCreateInfo.pAttachments = attachments.data();

    if (vkCreateRenderPass(Engine::GetInstance()->GetDevice(), &m_RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create render pass.");
        return INIT_FAIL;
    }

    Engine::GetInstance()->AskForFrameBuffers(this);

    return INIT_SUCCESS;
}

void RenderPass::Destroy() {
    for (VkFramebuffer framebuffer : m_FrameBuffers)
        vkDestroyFramebuffer(Engine::GetInstance()->GetDevice(), framebuffer, nullptr);
    m_FrameBuffers.resize(0);
    vkDestroyRenderPass(Engine::GetInstance()->GetDevice(), m_RenderPass, nullptr);
}
