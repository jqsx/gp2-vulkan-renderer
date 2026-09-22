//
// Created by frane on 6/11/2026.
//

#ifndef GPVKFR_UPLOADMANAGER_H
#define GPVKFR_UPLOADMANAGER_H
#include <deque>
#include <functional>
#include <vector>

#include "AllocatedBuffer.h"


class UploadPool {
public:
    struct PendingUpload {
        AllocatedBuffer buffer;
        VkFence fence{VK_NULL_HANDLE};
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    };

    struct PendingBatchCommand {
        std::function<void(VkCommandBuffer commandBuffer, PendingUpload& upload)> command;
        PendingUpload pending;
    };

    struct BatchUpload {
        std::vector<PendingUpload> uploads;
        VkFence fence{VK_NULL_HANDLE};
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    };

    PendingUpload& GetUpload();
    void BufferBatchUpload(PendingUpload&& pendingUpload, std::function<void(VkCommandBuffer commandBuffer, PendingUpload& upload)> cmd);

    void Update();
    void Dispose();

    void BeginBatch();
    void SubmitBatch();
    VkCommandBuffer GetBatchBuffer();

private:
    bool IsFenceDone(PendingUpload& pendingUpload);
    bool IsFenceDone(VkFence fence);
    VkFence CreateFence();
    VkFence PopOrCreateFence();

    std::deque<PendingUpload> m_PendingDeallocations{};
    std::deque<PendingBatchCommand> m_PendingBatchDeallocations{};
    std::vector<VkFence> m_FreeFences{};
    std::vector<BatchUpload> m_BatchUploads{};

    VkCommandBuffer m_BatchBuffer{nullptr};
};


#endif //GPVKFR_UPLOADMANAGER_H
