//
// Created by frane on 4/20/2026.
//

#ifndef GPVKFR_ALLOCATEDBUFFER_H
#define GPVKFR_ALLOCATEDBUFFER_H

#include <vk_mem_alloc.h>


class AllocatedBuffer final {
    uint32_t m_Size{0};
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;

public:
    void DeallocateBuffer() const;
    void AllocateBuffer(uint32_t size, VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VmaMemoryUsage memusage = VMA_MEMORY_USAGE_CPU_TO_GPU, VmaAllocationCreateFlags allocFlags = 0);
    // For now there is no option to change the buffer create properties for this function, but it's ok for now
    // Also automatically maps data to buffer for this
    void AllocateGPUBuffer(uint32_t size, void* data, VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    // Has to match the size of the allocated buffer (GetSize())
    void Map(void* data);

    AllocatedBuffer() = default;
    AllocatedBuffer(AllocatedBuffer&& other) noexcept;
    AllocatedBuffer(const AllocatedBuffer&) = delete;
    AllocatedBuffer& operator=(AllocatedBuffer&& other) noexcept;
    AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;

    VkBuffer GetBuffer() const { return m_Buffer; }
    VmaAllocation GetAllocation() const { return m_Allocation; }
    uint32_t GetSize() const { return m_Size; }
};

#endif //GPVKFR_ALLOCATEDBUFFER_H
