//
// Created by frane on 5/13/2026.
//

#include "GBuffer.h"

#include <utility>

#include "Engine.h"
#include "ImageTransitionHelper.h"

IGBuffer::~IGBuffer() = default;

GBuffer::GBuffer() {
    m_Buffers.reserve(4);
    m_ColorAttachmentRenderingInfo.reserve(4);
    m_Formats.reserve(4);
}

GBuffer::~GBuffer() = default;

const std::vector<VkRenderingAttachmentInfo> & GBuffer::GetColorAttachmentRenderingInfo() const {
    return m_ColorAttachmentRenderingInfo;
}

const std::vector<VkFormat> & GBuffer::GetFormats() const {
    return m_Formats;
}

void GBuffer::AppendImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage properties) {
    AllocatedImage& image = m_Buffers.emplace_back();
    VkExtent2D extent = Engine::GetInstance()->GetSwapChainExtent();
    image.AllocateImage(extent.width, extent.height, format, tiling, usage, properties);
    image.AllocateView(VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView GBuffer::GetImageView(uint32_t index) const {
    return m_Buffers[index].GetImageView();
}

VkImage GBuffer::GetImage(uint32_t index) const {
    return m_Buffers[index].GetImage();
}

void GBuffer::SetClearColor(uint32_t index, const VkClearColorValue& clearColor) {
    m_ColorAttachmentRenderingInfo[index].clearValue.color = clearColor;
}

void GBuffer::TransitionImages(ImageTransitionHelper& helper, VkImageLayout newLayout) {
    if (m_CurrentLayout == newLayout)
        return;

    for (AllocatedImage& buffer : m_Buffers) {
        helper.Transition(buffer, m_CurrentLayout, newLayout, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    m_CurrentLayout = newLayout;
}

void GBuffer::Create() {
    for (AllocatedImage & buffer: m_Buffers) {
        m_Formats.emplace_back(buffer.GetFormat());
        VkRenderingAttachmentInfo colorAttachmentRenderingInfo = {};
        colorAttachmentRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentRenderingInfo.imageView = buffer.GetImageView();
        colorAttachmentRenderingInfo.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

        colorAttachmentRenderingInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRenderingInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentRenderingInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        m_ColorAttachmentRenderingInfo.emplace_back(std::move(colorAttachmentRenderingInfo));
    }
}

void GBuffer::Destroy() {
    for (AllocatedImage & buffer: m_Buffers) {
        buffer.DeallocateImage();
    }
    m_Buffers.clear();

    m_ColorAttachmentRenderingInfo.clear();
    m_Formats.clear();
    m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

DeferredRenderBuffer::DeferredRenderBuffer() {
}

DeferredRenderBuffer::~DeferredRenderBuffer() {
}

void DeferredRenderBuffer::Create() {
    Destroy();

    m_Buffers.reserve(Engine::GetInstance()->GetMaxFramesInFlight());

    for (int index = 0; index < Engine::GetInstance()->GetMaxFramesInFlight(); ++index) {
        GBuffer& buffer = m_Buffers.emplace_back();

        buffer.AppendImage(VK_FORMAT_R8G8B8A8_SRGB); // Color buffer;
        buffer.AppendImage(VK_FORMAT_R16G16B16A16_SFLOAT); // Normal Buffer using rgb for normal mapped normal value, a for the geometry observable area value
        buffer.AppendImage(VK_FORMAT_R16G16B16A16_SFLOAT); // Position Buffer, xyz position + spare channel

        buffer.Create();
        buffer.SetClearColor(RENDER_ATTACHMENT_POSITION, {{0.0f, 0.0f, 0.0f, -1.0f}});
    }
}

void DeferredRenderBuffer::Destroy() {
    for (GBuffer & buffer: m_Buffers) {
        buffer.Destroy();
    }
    m_Buffers.clear();
}

const GBuffer & DeferredRenderBuffer::GetBuffer() const {
    return m_Buffers[Engine::GetInstance()->GetCurrentFrameIndex()];
}

GBuffer & DeferredRenderBuffer::GetBuffer() {
    return m_Buffers[Engine::GetInstance()->GetCurrentFrameIndex()];
}

GBuffer & DeferredRenderBuffer::GetBuffer(uint32_t index) {
    return m_Buffers[index];
}

const GBuffer & DeferredRenderBuffer::GetBuffer(uint32_t index) const {
    return m_Buffers[index];
}

const VkImageView DeferredRenderBuffer::GetImageView(AttachmentType type) const {
    return GetBuffer().GetImageView(type);
}
