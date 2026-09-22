//
// Created by frane on 6/11/2026.
//

#include "Fence.h"

#include <span>

#include "Engine.h"
#include <stdexcept>

Fence::~Fence() {
    Destroy();
}

VkFence Fence::GetFence() const {
    return m_Fences[Engine::GetInstance()->GetCurrentFrameIndex()];
}

void Fence::Wait() {
    vkWaitForFences(Engine::GetInstance()->GetDevice(), 1, &m_Fences[Engine::GetInstance()->GetCurrentFrameIndex()], VK_TRUE, UINT64_MAX);
}

void Fence::Reset() {
    vkResetFences(Engine::GetInstance()->GetDevice(), 1, &m_Fences[Engine::GetInstance()->GetCurrentFrameIndex()]);
}

void Fence::Create(bool signaled) {
    const uint32_t maxFramesInFlight{Engine::GetInstance()->GetMaxFramesInFlight()};
    VkFenceCreateInfo sample;
    sample.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    sample.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    std::vector<VkFenceCreateInfo> fenceCreateInfos{maxFramesInFlight, sample};

    if (maxFramesInFlight > 1)
        for (int index = 0; index < maxFramesInFlight - 1; ++index) {
            VkFenceCreateInfo& createInfo = fenceCreateInfos[index];
            createInfo.pNext = &fenceCreateInfos[index + 1];
        }

    m_Fences.resize(maxFramesInFlight);

    if (vkCreateFence(Engine::GetInstance()->GetDevice(), &fenceCreateInfos[0], nullptr, m_Fences.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fences");
    }
}

void Fence::Destroy() {
    if (m_Fences.empty()) {
        return;
    }

    vkWaitForFences(Engine::GetInstance()->GetDevice(), m_Fences.size(), m_Fences.data(), VK_TRUE, UINT64_MAX);
    for (VkFence fence : m_Fences) {
        vkDestroyFence(Engine::GetInstance()->GetDevice(), fence, nullptr);
    }

    m_Fences.clear();
}
