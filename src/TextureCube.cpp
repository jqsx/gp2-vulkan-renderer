//
// Created by frane on 6/13/2026.
//

#include "TextureCube.h"

#include <stdexcept>
#include <utility>

#include "Engine.h"

TextureCube::TextureCube(AllocatedImage&& source, VkImageAspectFlags aspectFlags) : m_Image(std::move(source)) {
    for (uint32_t face = 0; face < m_FaceViews.size(); ++face) {
        m_FaceViews[face] = m_Image.AllocateLayerView(m_Image.GetFormat(), aspectFlags, face);
    }
}

TextureCube::~TextureCube() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    for (VkImageView& view : m_FaceViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
            view = VK_NULL_HANDLE;
        }
    }

    m_Image.DeallocateImage();
}
