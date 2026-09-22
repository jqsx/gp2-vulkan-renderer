//
// Created by frane on 4/20/2026.
//

#include "AllocatedImage.h"

#include "AllocatedBuffer.h"
#include "Engine.h"
#include "ImageTransitionHelper.h"
#include "Logger.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

uint32_t AllocatedImage::FindMipLevels(uint32_t width, uint32_t height) const {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

AllocatedImage::AllocatedImage(AllocatedImage&& other) noexcept {
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Image = other.m_Image;
    m_Allocation = other.m_Allocation;
    m_ImageView = other.m_ImageView;
    m_Format = other.m_Format;
    m_MipLevels = other.m_MipLevels;
    m_LayerCount = other.m_LayerCount;
    m_ViewType = other.m_ViewType;

    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Image = VK_NULL_HANDLE;
    other.m_Allocation = VK_NULL_HANDLE;
    other.m_ImageView = VK_NULL_HANDLE;
    other.m_MipLevels = 1;
    other.m_LayerCount = 1;
    other.m_ViewType = VK_IMAGE_VIEW_TYPE_2D;
}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) noexcept {
    if (this == &other)
        return *this;

    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Image = other.m_Image;
    m_Allocation = other.m_Allocation;
    m_ImageView = other.m_ImageView;
    m_Format = other.m_Format;
    m_MipLevels = other.m_MipLevels;
    m_LayerCount = other.m_LayerCount;
    m_ViewType = other.m_ViewType;

    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Image = VK_NULL_HANDLE;
    other.m_Allocation = VK_NULL_HANDLE;
    other.m_ImageView = VK_NULL_HANDLE;
    other.m_MipLevels = 1;
    other.m_LayerCount = 1;
    other.m_ViewType = VK_IMAGE_VIEW_TYPE_2D;

    return *this;
}

void AllocatedImage::AllocateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage properties, uint32_t layerCount, VkImageCreateFlags flags) {
    VkExtent3D imageExtent;
    imageExtent.width = width;
    imageExtent.height = height;
    imageExtent.depth = 1;

    if (m_MipLevels > 1) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = imageExtent;
    imageInfo.mipLevels = m_MipLevels;
    imageInfo.arrayLayers = layerCount;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = flags;

    VmaAllocationCreateInfo vmaCreateInfo{};
    vmaCreateInfo.usage = properties;
    // vmaCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkResult result = vmaCreateImage(Engine::GetInstance()->GetAllocator(), &imageInfo, &vmaCreateInfo, &m_Image, &m_Allocation, nullptr);
    if (result != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create image", __LINE__, __FILE__);
        throw std::runtime_error("Failed to create image " + std::to_string(result));
    }

    m_Width = width;
    m_Height = height;
    m_Format = format;
    m_LayerCount = layerCount;
    m_ViewType = (layerCount == 6 && (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
}

void AllocatedImage::AllocateView(VkImageAspectFlags aspectFlags) {
    m_ImageView = AllocateUntrackedView(m_Format, aspectFlags);
}

VkImageView AllocatedImage::AllocateUntrackedView(VkFormat format, VkImageAspectFlags aspectFlags) {
    return AllocateUntrackedView(format, aspectFlags, m_ViewType, 0, m_LayerCount);
}

VkImageView AllocatedImage::AllocateUntrackedView(VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t baseArrayLayer, uint32_t layerCount) {
    if (m_Image == nullptr) {
        throw std::runtime_error("No image allocated for image view.");
    }
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = viewType;
    info.image = m_Image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = m_MipLevels;
    info.subresourceRange.baseArrayLayer = baseArrayLayer;
    info.subresourceRange.layerCount = layerCount;
    info.subresourceRange.aspectMask = aspectFlags;

    VkImageView view = VK_NULL_HANDLE;

    if (vkCreateImageView(Engine::GetInstance()->GetDevice(), &info, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view for allocated image");
    }

    return view;
}

void AllocatedImage::DeallocateImage() {
    if (m_Image == VK_NULL_HANDLE)
        return;

    vkDestroyImageView(Engine::GetInstance()->GetDevice(), m_ImageView, nullptr);
    vmaDestroyImage(Engine::GetInstance()->GetAllocator(), m_Image, m_Allocation);
    m_Image = VK_NULL_HANDLE;
    m_Allocation = VK_NULL_HANDLE;
    m_ImageView = VK_NULL_HANDLE;
}

/**
 *
 * @param data sized according to the amount of pixels in the image
 * @param generateMipMaps does the map also automatically generate mipmaps?
 * @param channels color channels
 */
void AllocatedImage::Map(void *data, bool generateMipMaps, int channels) {
    const uint32_t imageSize = m_Width * m_Height * channels * m_LayerCount;
    MapBytes(data, imageSize, generateMipMaps);
}

void AllocatedImage::MapBytes(void* data, uint32_t byteSize, bool generateMipMaps) {
    UploadPool::PendingUpload pendingUpload{};
    pendingUpload.buffer.AllocateBuffer(byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    pendingUpload.buffer.Map(data);

    const VkImage image = m_Image;
    const VkFormat format = m_Format;
    const uint32_t width = m_Width;
    const uint32_t height = m_Height;
    const uint32_t mipLevels = m_MipLevels;
    const uint32_t layerCount = m_LayerCount;

    Engine::GetInstance()->GetUploadPool().BufferBatchUpload(std::move(pendingUpload), [image, format, width, height, mipLevels, layerCount, generateMipMaps](VkCommandBuffer cmd, UploadPool::PendingUpload& upload) {
        ImageTransitionHelper helper(cmd);
        helper.Transition(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layerCount);
        helper.CopyBufferToImage(upload.buffer, image, width, height, layerCount);
        if (generateMipMaps)
            helper.GenerateMips(image, format, width, height, mipLevels, layerCount);
        else
            helper.Transition(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layerCount);
    });
}

void AllocatedImage::AllowMipMaps(uint32_t width, uint32_t height) {
    m_MipLevels = FindMipLevels(width, height);
}

void AllocatedImage::GenerateMipMaps() {
    Engine::GetInstance()->GetAllocCMD().CmdTemp([&](VkCommandBuffer cmd) {
        ImageTransitionHelper helper(cmd);
        helper.GenerateMips(*this);
    });
}
