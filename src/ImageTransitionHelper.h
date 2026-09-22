//
// Created by frane on 6/11/2026.
//

#ifndef GPVKFR_IMAGETRANSITIONHELPER_H
#define GPVKFR_IMAGETRANSITIONHELPER_H
#include "AllocatedImage.h"
#include "CommandPool.h"

class ImageTransitionHelper {
public:
    explicit ImageTransitionHelper(VkCommandBuffer buffer);

    void Transition(const AllocatedImage& target, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask, uint32_t baseMip = 0, uint32_t mipCount = 1);
    void Transition(VkImage target, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask, uint32_t baseMip = 0, uint32_t mipCount = 1, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);
    void CopyBufferToImage(const AllocatedBuffer& from, const AllocatedImage& to);
    void CopyBufferToImage(const AllocatedBuffer& from, VkImage to, uint32_t width, uint32_t height, uint32_t layerCount = 1);
    void CopyBuffer(const AllocatedBuffer& from, const AllocatedBuffer& to);
    void CopyBuffer(VkBuffer from, VkBuffer to, VkDeviceSize size);
    void GenerateMips(const AllocatedImage& target);
    void GenerateMips(VkImage target, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layerCount = 1);

private:
    VkCommandBuffer m_Buffer;
};

#endif //GPVKFR_IMAGETRANSITIONHELPER_H
