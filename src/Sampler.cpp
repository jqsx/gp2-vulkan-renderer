//
// Created by frane on 4/21/2026.
//

#include "Sampler.h"

#include "Engine.h"
#include "Logger.h"

Sampler::Sampler() : m_CreateInfo() {
    m_CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    Linear();

    Repeat();

    EnableAnisotropy(true);

    m_CreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    NormalizedCoordinates(true);

    m_CreateInfo.compareEnable = VK_FALSE;
    m_CreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    m_CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    m_CreateInfo.mipLodBias = 0.0f;
    m_CreateInfo.minLod = 0.0f;
    m_CreateInfo.maxLod = 0.0f;
    m_CreateInfo.flags = 0;
    m_CreateInfo.pNext = nullptr;

    m_CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    m_CreateInfo.minLod = 0.0f;
    m_CreateInfo.maxLod = VK_LOD_CLAMP_NONE;
    m_CreateInfo.mipLodBias = 0.0f;
}

Sampler::~Sampler() {
    Destroy();
}

Sampler & Sampler::Repeat() {
    m_CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    return *this;
}

Sampler & Sampler::ClampEdge() {
    m_CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    m_CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    m_CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    return *this;
}

Sampler & Sampler::ClampBorder() {
    m_CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    m_CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    m_CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    return *this;
}

Sampler & Sampler::Mirror() {
    m_CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    m_CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    m_CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    return *this;
}

Sampler & Sampler::Linear() {
    m_CreateInfo.magFilter = VK_FILTER_LINEAR;
    m_CreateInfo.minFilter = VK_FILTER_LINEAR;
    return *this;
}

Sampler & Sampler::Nearest() {
    m_CreateInfo.magFilter = VK_FILTER_NEAREST;
    m_CreateInfo.minFilter = VK_FILTER_NEAREST;
    return *this;
}

Sampler & Sampler::NormalizedCoordinates(bool state) {
    m_CreateInfo.unnormalizedCoordinates = state ? VK_FALSE : VK_TRUE;
    return *this;
}

Sampler & Sampler::EnableAnisotropy(bool state) {
    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(Engine::GetInstance()->GetPhysicalDevice(), &supportedFeatures);

    if (!supportedFeatures.samplerAnisotropy) {
        m_CreateInfo.maxAnisotropy = 1.0f;
        m_CreateInfo.anisotropyEnable = VK_FALSE;
        return *this;
    }

    m_CreateInfo.anisotropyEnable = state ? VK_TRUE : VK_FALSE;
    if (state) {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(Engine::GetInstance()->GetPhysicalDevice(), &deviceProps);
        m_CreateInfo.maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;
    }
    else {
        m_CreateInfo.maxAnisotropy = 1.0f;
    }
    return *this;
}

Sampler & Sampler::AnisotropyMax(float value) {
    m_CreateInfo.maxAnisotropy = value;
    return *this;
}

Sampler & Sampler::MipMapParams(VkSamplerMipmapMode mode, float minLod, float maxLod, float lodBias) {
    m_CreateInfo.mipmapMode = mode;
    m_CreateInfo.minLod = minLod;
    m_CreateInfo.maxLod = maxLod;
    m_CreateInfo.mipLodBias = lodBias;
    return *this;
}

InitResult Sampler::Create() {
    if (vkCreateSampler(Engine::GetInstance()->GetDevice(), &m_CreateInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create sampler.");
        return INIT_FAIL;
    }
    return INIT_SUCCESS;
}

void Sampler::Destroy() {
    if (m_Sampler == VK_NULL_HANDLE)
        return;
    vkDestroySampler(Engine::GetInstance()->GetDevice(), m_Sampler, nullptr);
    m_Sampler = VK_NULL_HANDLE;
}
