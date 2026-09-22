//
// Created by frane on 4/21/2026.
//

#ifndef GPVKFR_COMMANDPOOL_H
#define GPVKFR_COMMANDPOOL_H
#include <vector>
#include <vulkan/vulkan.h>
#include "structs.h"

class CommandPool {
        VkCommandPool m_CommandPool;
        VkQueue m_Queue;
        uint32_t m_QueueFamilyIndex;

        std::vector<VkCommandBuffer> m_CommandBuffers;
public:
        CommandPool();
        ~CommandPool();

        void SetQueue(VkQueue queue, uint32_t queueFamilyIndex);

        VkCommandBuffer CmdTemp(std::function<void(VkCommandBuffer cmd)> func, VkFence fence = VK_NULL_HANDLE);
        void Cmd(uint32_t index, std::function<void(VkCommandBuffer cmd)> func, VkFence fence = nullptr);
        void Free(VkCommandBuffer commandBuffer);
        void ResetBuffer(uint32_t index);
        void AllocateBuffers(uint32_t count);
        void DeallocateBuffers();

        VkCommandBuffer BeginCmd();
        void EndCmd(VkCommandBuffer commandBuffer, VkFence fence, bool dispose = false);

        void Wait();

        VkCommandBuffer GetBuffer(uint32_t index);

        InitResult Create();
        void Destroy();
};

#endif //GPVKFR_COMMANDPOOL_H
