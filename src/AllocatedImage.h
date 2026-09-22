//
// Created by frane on 4/20/2026.
//

#ifndef GPVKFR_ALLOCATEDIMAGE_H
#define GPVKFR_ALLOCATEDIMAGE_H

#include <vk_mem_alloc.h>

class AllocatedImage final {
    uint32_t m_Width{0}, m_Height{0};
    VkImage m_Image{VK_NULL_HANDLE};
    VmaAllocation m_Allocation{VK_NULL_HANDLE};
    VkImageView m_ImageView{VK_NULL_HANDLE};
    VkFormat m_Format{VK_FORMAT_R8G8B8A8_SRGB};
    uint32_t m_MipLevels{1};
    uint32_t m_LayerCount{1};
    VkImageViewType m_ViewType{VK_IMAGE_VIEW_TYPE_2D};

    uint32_t FindMipLevels(uint32_t width, uint32_t height) const;

    void GenerateMipMaps();

public:
    AllocatedImage() = default;
    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage(const AllocatedImage& other) = delete;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(const AllocatedImage& other) = delete;

    void AllocateImage(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VmaMemoryUsage properties = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, uint32_t layerCount = 1, VkImageCreateFlags flags = 0);
    void AllocateView(VkImageAspectFlags aspectFlags);
    VkImageView AllocateUntrackedView(VkFormat format, VkImageAspectFlags aspectFlags);
    VkImageView AllocateUntrackedView(VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t baseArrayLayer, uint32_t layerCount);
    VkImageView AllocateLayerView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layer) { return AllocateUntrackedView(format, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, layer, 1); }
    void DeallocateImage();
    void Map(void* data, bool generateMipMaps = false, int channels = 4);
    void MapBytes(void* data, uint32_t byteSize, bool generateMipMaps = false);
    void AllowMipMaps(uint32_t width, uint32_t height);

    VkImage GetImage() const { return m_Image; }
    VkImageView GetImageView() const { return m_ImageView; }
    VmaAllocation GetAllocation() const { return m_Allocation; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    VkFormat GetFormat() const { return m_Format; }
    uint32_t GetMipLevels() const { return m_MipLevels; }
    uint32_t GetLayerCount() const { return m_LayerCount; }
};

#endif //GPVKFR_ALLOCATEDIMAGE_H
