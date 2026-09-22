//
// Created by frane on 6/11/2026.
//

#ifndef GPVKFR_FENCE_H
#define GPVKFR_FENCE_H
#include <vector>
#include <vulkan/vulkan_core.h>


class Fence {
public:
    Fence() = default;
    ~Fence();

    VkFence GetFence() const;
    void Wait();
    void Reset();

    void Create(bool signaled = true);
    void Destroy();

private:
    std::vector<VkFence> m_Fences{};
};


#endif //GPVKFR_FENCE_H