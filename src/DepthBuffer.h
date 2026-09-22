//
// Created by frane on 4/21/2026.
//

#ifndef GPVKFR_DEPTHBUFFER_H
#define GPVKFR_DEPTHBUFFER_H
#include <vector>

#include <vulkan/vulkan.h>
#include "AllocatedImage.h"

class DepthBuffer {
    AllocatedImage m_Image;
    VkFormat m_Format;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencil;
public:
    DepthBuffer();
    ~DepthBuffer();

    void InitDefaults();

    DepthBuffer& Format(VkFormat format);
    DepthBuffer& StencilCompareOp(VkCompareOp op);
    DepthBuffer& StencilDepthTestEnable(bool state);
    DepthBuffer& StencilWriteEnable(bool state);
    DepthBuffer& StencilBoundsTestEnable(bool state);

    VkImage GetImage() const { return m_Image.GetImage(); }
    VkImageView GetImageView() const { return m_Image.GetImageView(); }
    VkFormat GetFormat() const { return m_Format; }
    const VkPipelineDepthStencilStateCreateInfo& GetDepthStencil() const { return m_DepthStencil; }

    void Create();
    void Destroy();

private:
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
};


#endif //GPVKFR_DEPTHBUFFER_H
