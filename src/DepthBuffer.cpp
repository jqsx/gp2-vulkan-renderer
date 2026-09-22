//
// Created by frane on 4/21/2026.
//

#include "DepthBuffer.h"

#include "Engine.h"
#include "ImageTransitionHelper.h"
#include <stdexcept>

class Engine;

DepthBuffer::DepthBuffer() {

    m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencil.depthTestEnable = VK_TRUE;
    m_DepthStencil.depthWriteEnable = VK_TRUE;
    m_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencil.minDepthBounds = 0.0f;
    m_DepthStencil.maxDepthBounds = 1.0f;

    m_DepthStencil.pNext = nullptr;
    m_DepthStencil.flags = 0;

    m_DepthStencil.front = {};
    m_DepthStencil.back = {};



    m_DepthStencil.stencilTestEnable = VK_FALSE;
}

DepthBuffer::~DepthBuffer() {
}

void DepthBuffer::InitDefaults() {
    m_Format = findDepthFormat();

    m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencil.depthTestEnable = VK_TRUE;
    m_DepthStencil.depthWriteEnable = VK_TRUE;
    m_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencil.minDepthBounds = 0.0f;
    m_DepthStencil.maxDepthBounds = 1.0f;

    m_DepthStencil.stencilTestEnable = VK_FALSE;
}

DepthBuffer & DepthBuffer::Format(VkFormat format) {
    m_Format = format;
    return *this;
}

DepthBuffer & DepthBuffer::StencilCompareOp(VkCompareOp op) {
    m_DepthStencil.depthCompareOp = op;
    return *this;
}

DepthBuffer & DepthBuffer::StencilDepthTestEnable(bool state) {
    m_DepthStencil.depthTestEnable = state;
    return *this;
}

DepthBuffer & DepthBuffer::StencilWriteEnable(bool state) {
    m_DepthStencil.depthWriteEnable = state;
    return *this;
}

DepthBuffer & DepthBuffer::StencilBoundsTestEnable(bool state) {
    m_DepthStencil.depthBoundsTestEnable = state;
    return *this;
}

void DepthBuffer::Create() {
    Logger::GetInstance().log("Begin create depth buffer");
    Engine* engine = Engine::GetInstance();
    VkExtent2D swapChainExtent = engine->GetSwapChainExtent();
    m_Image.AllocateImage(swapChainExtent.width, swapChainExtent.height, m_Format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    m_Image.AllocateView(VK_IMAGE_ASPECT_DEPTH_BIT);

    engine->GetAllocCMD().CmdTemp([&](VkCommandBuffer cmd) {
        ImageTransitionHelper helper(cmd);
        helper.Transition(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    });
    Logger::GetInstance().log("Finished create depth buffer");
}

void DepthBuffer::Destroy() {
    m_Image.DeallocateImage();
}

VkFormat DepthBuffer::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
VkFormatFeatureFlags features)  {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(Engine::GetInstance()->GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat DepthBuffer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
