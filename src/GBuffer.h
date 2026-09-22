//
// Created by frane on 5/13/2026.
//

#ifndef GPVKFR_GBUFFER_H
#define GPVKFR_GBUFFER_H
#include <vector>

#include "AllocatedImage.h"

class ImageTransitionHelper;

class IGBuffer {
public:
    virtual const std::vector<VkRenderingAttachmentInfo>& GetColorAttachmentRenderingInfo() const = 0;
    virtual const std::vector<VkFormat>& GetFormats() const = 0;
    virtual ~IGBuffer() = 0;
};

class GBuffer : public IGBuffer {
    std::vector<AllocatedImage> m_Buffers;
    std::vector<VkRenderingAttachmentInfo> m_ColorAttachmentRenderingInfo;
    std::vector<VkFormat> m_Formats;
    VkImageLayout m_CurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};

public:
    GBuffer();
    GBuffer(GBuffer&& other) noexcept = default;
    GBuffer(const GBuffer& other) = delete;
    GBuffer& operator=(GBuffer&& other) noexcept = default;
    GBuffer& operator=(const GBuffer& other) = delete;
    ~GBuffer() override;
    const std::vector<VkRenderingAttachmentInfo>& GetColorAttachmentRenderingInfo() const override;
    const std::vector<VkFormat>& GetFormats() const override;

    void AppendImage(VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VmaMemoryUsage properties = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    VkImageView GetImageView(uint32_t index) const;
    VkImage GetImage(uint32_t index) const;
    void SetClearColor(uint32_t index, const VkClearColorValue& clearColor);
    void TransitionImages(ImageTransitionHelper& helper, VkImageLayout newLayout);
    virtual void Create();
    virtual void Destroy();
};

// Storage class for actual rendering and exchange of buffers via currentFrame
class DeferredRenderBuffer {
    std::vector<GBuffer> m_Buffers;

public:
    enum AttachmentType {
        RENDER_ATTACHMENT_COLOR = 0,
        RENDER_ATTACHMENT_NORMAL = 1,
        RENDER_ATTACHMENT_POSITION = 2
    };

    DeferredRenderBuffer();
    ~DeferredRenderBuffer();

    void Create();
    void Destroy();

    GBuffer& GetBuffer();
    GBuffer& GetBuffer(uint32_t index);
    const GBuffer& GetBuffer() const;
    const GBuffer& GetBuffer(uint32_t index) const;
    const VkImageView GetImageView(AttachmentType type) const;
};


#endif //GPVKFR_GBUFFER_H
