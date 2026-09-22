//
// Created by frane on 6/11/2026.
//

#include "UploadPool.h"

#include "Engine.h"

#include <algorithm>
#include <utility>
#include <stdexcept>

UploadPool::PendingUpload & UploadPool::GetUpload() {
    PendingUpload& pendingUpload = m_PendingDeallocations.emplace_back();
    pendingUpload.fence = PopOrCreateFence();
    return pendingUpload;
}

void UploadPool::BufferBatchUpload(PendingUpload&& pendingUpload, std::function<void(VkCommandBuffer commandBuffer, PendingUpload &upload)> cmd) {
    if (m_BatchBuffer == nullptr)
        BeginBatch();
    pendingUpload.commandBuffer = GetBatchBuffer();
    PendingBatchCommand command { std::move(cmd), std::move(pendingUpload) };
    m_PendingBatchDeallocations.emplace_back(std::move(command));
}

void UploadPool::Update() {
    bool needsClearing = false;
    for (PendingUpload& pendingUpload : m_PendingDeallocations) {
        if (IsFenceDone(pendingUpload)) {
            needsClearing = true;

            m_FreeFences.push_back(pendingUpload.fence);

            vkResetFences(Engine::GetInstance()->GetDevice(), 1, &pendingUpload.fence);

            pendingUpload.buffer.DeallocateBuffer();
            Engine::GetInstance()->GetAllocCMD().Free(pendingUpload.commandBuffer);
            pendingUpload.commandBuffer = VK_NULL_HANDLE;
            pendingUpload.fence = nullptr;
        }
    }

    for (BatchUpload& batchUpload : m_BatchUploads) {
        if (IsFenceDone(batchUpload.fence)) {
            needsClearing = true;

            m_FreeFences.push_back(batchUpload.fence);

            vkResetFences(Engine::GetInstance()->GetDevice(), 1, &batchUpload.fence);

            for (PendingUpload& pendingUpload : batchUpload.uploads) {
                pendingUpload.buffer.DeallocateBuffer();
            }
            Engine::GetInstance()->GetAllocCMD().Free(batchUpload.commandBuffer);
            batchUpload.commandBuffer = VK_NULL_HANDLE;
            batchUpload.fence = nullptr;
        }
    }

    if (needsClearing) {
        std::erase_if(m_PendingDeallocations, [](const PendingUpload& pendingUpload) { return pendingUpload.fence == nullptr; });
        std::erase_if(m_BatchUploads, [](const BatchUpload& batch) { return batch.fence == nullptr;});
    }
}

void UploadPool::Dispose() {
    SubmitBatch();

    for (PendingUpload& pendingUpload : m_PendingDeallocations) {
        vkWaitForFences(Engine::GetInstance()->GetDevice(), 1, &pendingUpload.fence, VK_TRUE, UINT64_MAX);

        pendingUpload.buffer.DeallocateBuffer();
        Engine::GetInstance()->GetAllocCMD().Free(pendingUpload.commandBuffer);

        vkDestroyFence(Engine::GetInstance()->GetDevice(), pendingUpload.fence, nullptr);
    }
    for (BatchUpload& batchUpload : m_BatchUploads) {
        vkWaitForFences(Engine::GetInstance()->GetDevice(), 1, &batchUpload.fence, VK_TRUE, UINT64_MAX);

        for (PendingUpload& pendingUpload : batchUpload.uploads) {
            pendingUpload.buffer.DeallocateBuffer();
        }

        Engine::GetInstance()->GetAllocCMD().Free(batchUpload.commandBuffer);
        vkDestroyFence(Engine::GetInstance()->GetDevice(), batchUpload.fence, nullptr);
    }
    for (VkFence fence : m_FreeFences) {
        vkDestroyFence(Engine::GetInstance()->GetDevice(), fence, nullptr);
    }
}

void UploadPool::BeginBatch() {
    m_BatchBuffer = Engine::GetInstance()->GetAllocCMD().BeginCmd();
}

void UploadPool::SubmitBatch() {
    if (GetBatchBuffer() == VK_NULL_HANDLE || m_PendingBatchDeallocations.empty()) {
        if (GetBatchBuffer() != VK_NULL_HANDLE) {
            Engine::GetInstance()->GetAllocCMD().EndCmd(GetBatchBuffer(), VK_NULL_HANDLE, true);
            m_BatchBuffer = VK_NULL_HANDLE;
        }
        return;
    }
    BatchUpload upload{{}, PopOrCreateFence()};
    for (PendingBatchCommand& pendingUpload : m_PendingBatchDeallocations) {
        pendingUpload.command(GetBatchBuffer(), pendingUpload.pending);
        upload.uploads.emplace_back(std::move(pendingUpload.pending));
    }
    m_PendingBatchDeallocations.clear();
    Engine::GetInstance()->GetAllocCMD().EndCmd(GetBatchBuffer(), upload.fence);
    upload.commandBuffer = GetBatchBuffer();
    m_BatchBuffer = nullptr;
    m_BatchUploads.emplace_back(std::move(upload));
}

VkCommandBuffer UploadPool::GetBatchBuffer() {
    return m_BatchBuffer;
}

bool UploadPool::IsFenceDone(PendingUpload &pendingUpload) {
    return IsFenceDone(pendingUpload.fence);
}

bool UploadPool::IsFenceDone(VkFence fence) {
    return vkGetFenceStatus(Engine::GetInstance()->GetDevice(), fence) == VK_SUCCESS;
}

VkFence UploadPool::CreateFence() {
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = 0;

    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(Engine::GetInstance()->GetDevice(), &createInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence");
    }
    return fence;
}

VkFence UploadPool::PopOrCreateFence() {
    if (m_FreeFences.empty()) {
        return CreateFence();
    }
    VkFence fence = m_FreeFences.back();
    m_FreeFences.pop_back();
    return fence;
}
