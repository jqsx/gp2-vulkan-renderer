//
// Created by frane on 4/21/2026.
//

#include "CommandPool.h"

#include "Engine.h"
#include "Logger.h"
#include <stdexcept>

VkCommandBuffer CommandPool::BeginCmd() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(Engine::GetInstance()->GetDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void CommandPool::EndCmd(VkCommandBuffer commandBuffer, VkFence fence, bool dispose) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_Queue, 1, &submitInfo, fence);

    if (fence == VK_NULL_HANDLE)
        vkQueueWaitIdle(m_Queue);

    if (dispose)
        vkFreeCommandBuffers(Engine::GetInstance()->GetDevice(), m_CommandPool, 1, &commandBuffer);
}

CommandPool::CommandPool() : m_CommandPool(nullptr), m_Queue(nullptr), m_QueueFamilyIndex(0)
{
}

CommandPool::~CommandPool() {
}

void CommandPool::SetQueue(VkQueue queue, uint32_t queueFamilyIndex) {
    if (m_CommandPool != nullptr)
        throw std::runtime_error("Command pool already exists");

    m_Queue = queue;
    m_QueueFamilyIndex = queueFamilyIndex;
}

void CommandPool::AllocateBuffers(uint32_t count) {
    if (m_CommandBuffers.size() > 0) {
        DeallocateBuffers();
    }

    m_CommandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(Engine::GetInstance()->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void CommandPool::DeallocateBuffers() {
    if (m_CommandBuffers.size() > 0)
        vkFreeCommandBuffers(Engine::GetInstance()->GetDevice(), m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());
    m_CommandBuffers.resize(0);
}

void CommandPool::Wait() {
    vkQueueWaitIdle(m_Queue);
}

VkCommandBuffer CommandPool::GetBuffer(uint32_t index) {
    return m_CommandBuffers[index];
}

InitResult CommandPool::Create() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    poolInfo.queueFamilyIndex = m_QueueFamilyIndex;

    if (vkCreateCommandPool(Engine::GetInstance()->GetDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create command pool.");
        return INIT_FAIL;
    }
    return INIT_SUCCESS;
}

void CommandPool::Destroy() {
    DeallocateBuffers();
    vkDestroyCommandPool(Engine::GetInstance()->GetDevice(), m_CommandPool, nullptr);
}

VkCommandBuffer CommandPool::CmdTemp(std::function<void(VkCommandBuffer cmd)> func, VkFence fence) {
    VkCommandBuffer buffer = BeginCmd();
    func(buffer);
    EndCmd(buffer, fence, fence == VK_NULL_HANDLE);
    return buffer;
}

void CommandPool::Cmd(uint32_t index, std::function<void(VkCommandBuffer cmd)> func, VkFence fence) {
    VkCommandBuffer buffer = GetBuffer(index);
    func(buffer);
    EndCmd(buffer, fence);
}

void CommandPool::Free(VkCommandBuffer commandBuffer) {
    if (commandBuffer == VK_NULL_HANDLE)
        return;
    vkFreeCommandBuffers(Engine::GetInstance()->GetDevice(), m_CommandPool, 1, &commandBuffer);
}

void CommandPool::ResetBuffer(uint32_t index) {
    vkResetCommandBuffer(GetBuffer(index), 0);
}
