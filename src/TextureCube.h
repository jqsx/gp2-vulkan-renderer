//
// Created by frane on 6/13/2026.
//

#ifndef GPVKFR_TEXTURECUBE_H
#define GPVKFR_TEXTURECUBE_H

#include <array>
#include <vulkan/vulkan.h>

#include "AllocatedImage.h"

class TextureCube final {
    AllocatedImage m_Image;
    std::array<VkImageView, 6> m_FaceViews{};

public:
    explicit TextureCube(AllocatedImage&& source, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ~TextureCube();

    VkImage GetImage() const { return m_Image.GetImage(); }
    VkImageView GetImageView() const { return m_Image.GetImageView(); }
    VkImageView GetFaceView(uint32_t face) const { return m_FaceViews[face]; }
    VkFormat GetFormat() const { return m_Image.GetFormat(); }
    uint32_t GetSize() const { return m_Image.GetWidth(); }
};

#endif //GPVKFR_TEXTURECUBE_H
