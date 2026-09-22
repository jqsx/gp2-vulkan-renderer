//
// Created by frane on 4/20/2026.
//

#include "AllocatedBuffer.h"

#include "Engine.h"

#include <cstring>
#include <stdexcept>
#include <utility>

#include "ImageTransitionHelper.h"

void AllocatedBuffer::DeallocateBuffer() const {
    // Deallocating by hand gives me placebo control over memory
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
    vmaDestroyBuffer(allocator, m_Buffer, m_Allocation);
}

void AllocatedBuffer::AllocateBuffer(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage memusage, VmaAllocationCreateFlags allocFlags) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaCreateInfo{};
    vmaCreateInfo.usage = memusage;
    vmaCreateInfo.flags = allocFlags;

    // Not having to pass physical device and device is luxurious
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();

    if (vmaCreateBuffer(allocator, &bufferInfo, &vmaCreateInfo, &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    m_Size = size;
}

void AllocatedBuffer::AllocateGPUBuffer(uint32_t size, void* data, VkBufferUsageFlags usage) {
    UploadPool::PendingUpload pendingUpload{};
    pendingUpload.buffer.AllocateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    );

    pendingUpload.buffer.Map(data);

    AllocateBuffer(
        size,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );

    const VkBuffer buffer = m_Buffer;
    const VkDeviceSize bufferSize = size;

    Engine::GetInstance()->GetUploadPool().BufferBatchUpload(std::move(pendingUpload), [buffer, bufferSize](VkCommandBuffer cmd, UploadPool::PendingUpload& upload) {
        ImageTransitionHelper helper(cmd);
        helper.CopyBuffer(upload.buffer.GetBuffer(), buffer, bufferSize);
    });
}

void AllocatedBuffer::Map(void *data) {
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();

    void* stored;
    if (vmaMapMemory(allocator, m_Allocation, &stored) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map buffer");
    }
    memcpy(stored, data, m_Size);
    vmaUnmapMemory(allocator, m_Allocation);
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer &&other) noexcept {
    m_Buffer = other.m_Buffer;
    m_Allocation = other.m_Allocation;
    m_Size = other.m_Size;
    other.m_Buffer = nullptr;
    other.m_Allocation = nullptr;
    other.m_Size = 0;
}

AllocatedBuffer & AllocatedBuffer::operator=(AllocatedBuffer &&other) noexcept {
    m_Buffer = other.m_Buffer;
    m_Allocation = other.m_Allocation;
    m_Size = other.m_Size;
    other.m_Buffer = nullptr;
    other.m_Allocation = nullptr;
    other.m_Size = 0;
    return *this;
}
