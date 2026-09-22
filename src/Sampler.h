//
// Created by frane on 4/21/2026.
//

#ifndef GPVKFR_SAMPLER_H
#define GPVKFR_SAMPLER_H
#include <vulkan/vulkan_core.h>

#include "structs.h"


class Sampler {
    VkSampler m_Sampler{VK_NULL_HANDLE};
    VkSamplerCreateInfo m_CreateInfo;
    public:
    Sampler();
    ~Sampler();

    Sampler& Repeat();
    Sampler& ClampEdge();
    Sampler& ClampBorder();
    Sampler& Mirror();

    Sampler& Linear();
    Sampler& Nearest();

    Sampler& NormalizedCoordinates(bool state);

    Sampler& EnableAnisotropy(bool state);
    Sampler& AnisotropyMax(float value);

    Sampler& MipMapParams(VkSamplerMipmapMode mode, float minLod, float maxLod, float lodBias);

    VkSampler GetSampler() const { return m_Sampler; }

    InitResult Create();
    void Destroy();
};


#endif //GPVKFR_SAMPLER_H
