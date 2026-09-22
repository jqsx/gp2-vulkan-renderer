//
// Created by frane on 5/4/2026.
//

#ifndef GPVKFR_RENDERER_H
#define GPVKFR_RENDERER_H
#include <array>
#include <vector>

#include "AllocatedBuffer.h"
#include "AllocatedImage.h"
#include "GBuffer.h"
#include "Sampler.h"
#include "Shader.h"
#include "ShaderProgram.h"

class SwapChain;
class GameObject;
class Texture2D;
class TextureCube;

class Renderer {
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
    static constexpr uint32_t POINT_SHADOW_MAP_SIZE = 2048;
    static constexpr uint32_t MAX_POINT_LIGHTS = 8;

    struct PointLightShadowResource {
        AllocatedImage image;
        std::array<VkImageView, 6> faceViews{};
        VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    };

    DeferredRenderBuffer m_RenderGBuffer;
    AllocatedImage m_ShadowMap;
    VkImageLayout m_ShadowMapLayout{VK_IMAGE_LAYOUT_UNDEFINED};
    Shader m_ShadowShader;
    ShaderProgram m_ShadowProgram;
    Shader m_PointShadowShader;
    ShaderProgram m_PointShadowProgram;
    bool m_ShadowResourcesCreated{false};
    std::vector<AllocatedImage> m_DeferredOutputImages;
    std::vector<VkImageLayout> m_DeferredOutputLayouts;
    Sampler m_DeferredSampler;
    Shader m_DeferredShader;
    ShaderProgram m_DeferredProgram;
    std::vector<VkDescriptorSet> m_DeferredDescriptorSets;
    TextureCube* m_SkyboxTexture{nullptr};
    TextureCube* m_DefaultSkyboxTexture{nullptr};
    Sampler m_TonemapSampler;
    Shader m_TonemapShader;
    ShaderProgram m_TonemapProgram;
    std::vector<VkDescriptorSet> m_TonemapDescriptorSets;
    std::vector<UniformBuffer> m_DeferredLightingBuffers;
    UniformBuffer m_PointLightShadowBuffer;
    AllocatedImage m_FallbackPointShadowMap;
    VkImageLayout m_FallbackPointShadowMapLayout{VK_IMAGE_LAYOUT_UNDEFINED};
    std::vector<PointLightShadowResource> m_PointLightShadowResources;
    uint32_t m_ActivePointLightCount{0};
    bool m_PointLightDescriptorsDirty{true};
    bool m_IsShadowBuffered{ false };

    void CreateFrameResources();
    void DestroyFrameResources();
    void EnsureShadowResources();
    void DestroyShadowResources();
    void CreateDeferredLightingBuffers();
    void DestroyDeferredLightingBuffers();
    void CreatePointLightShadowResources();
    void DestroyPointLightShadowResources();
    void SyncPointLightShadowResources();
    void UpdatePointLightShadowData();
    void UpdateDeferredLightingBuffer();
    void EnsureDefaultSkyboxTexture();
    void CreateDeferredResources();
    void DestroyDeferredResources();
    void UpdateDeferredDescriptors();
    void CreateTonemapResources();
    void DestroyTonemapResources();
    void UpdateTonemapDescriptors();

    void RenderShadowPrepass(VkCommandBuffer commandBuffer, VkImageView targetView, VkExtent2D extent, const m4& lightViewProjection);
    void RenderPointShadowPrepass(VkCommandBuffer commandBuffer, VkImageView targetView, VkExtent2D extent, const PointLight& light, const m4& lightViewProjection);
    void RenderPointLightShadowMaps(VkCommandBuffer commandBuffer);
    void RenderShadowMap(VkCommandBuffer commandBuffer);
    void RenderGBuffer(uint32_t imageIndex, VkCommandBuffer commandBuffer);
    void RenderDeferredShading(VkCommandBuffer commandBuffer);
    void RenderTonemap(uint32_t imageIndex, VkCommandBuffer commandBuffer);

    void Record(uint32_t imageIndex, VkCommandBuffer commandBuffer);
public:
    Renderer();
    ~Renderer();
    const GBuffer& GetGBuffer() const { return m_RenderGBuffer.GetBuffer(); }
    void SetSkyboxTexture(TextureCube* texture);
    void RecreateFrameResources();
    void RenderFrame(uint32_t imageIndex, VkCommandBuffer commandBuffer);
};


#endif //GPVKFR_RENDERER_H
