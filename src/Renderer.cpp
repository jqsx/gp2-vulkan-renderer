//
// Created by frane on 5/4/2026.
//

#include "Renderer.h"

#include "Camera.h"
#include "Engine.h"
#include "GameObject.h"
#include "ImageTransitionHelper.h"
#include "Material.h"
#include "Mesh.h"
#include "Scene.h"
#include "ShaderProgram.h"
#include "SwapChain.h"
#include "Texture2D.h"
#include "TextureCube.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include "ResourceLoader.h"

void Renderer::CreateFrameResources() {
    m_IsShadowBuffered = false;
    m_RenderGBuffer.Create();
    CreateDeferredLightingBuffers();
    CreatePointLightShadowResources();
    EnsureDefaultSkyboxTexture();

    VkFormat depthFormat = Engine::GetInstance()->GetDepthBuffer().GetFormat();
    m_ShadowMap.AllocateImage(
        SHADOW_MAP_SIZE,
        SHADOW_MAP_SIZE,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );
    m_ShadowMap.AllocateView(VK_IMAGE_ASPECT_DEPTH_BIT);

    Engine::GetInstance()->GetAllocCMD().CmdTemp([&](VkCommandBuffer cmd) {
        ImageTransitionHelper helper(cmd);
        helper.Transition(m_ShadowMap, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    });
    m_ShadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateDeferredResources();
    CreateTonemapResources();
}

void Renderer::DestroyFrameResources() {
    m_IsShadowBuffered = false;
    DestroyTonemapResources();
    DestroyDeferredResources();
    DestroyPointLightShadowResources();
    DestroyShadowResources();
    DestroyDeferredLightingBuffers();
    m_RenderGBuffer.Destroy();
    m_ShadowMap.DeallocateImage();
    m_ShadowMapLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void Renderer::EnsureShadowResources() {
    if (m_ShadowResourcesCreated)
        return;

    m_ShadowShader.Create("resources/shaders/shadowmap.frag.spv", "resources/shaders/shadowmap.vert.spv");
    m_ShadowProgram.SetShader(&m_ShadowShader);
    m_ShadowProgram.SetVertexInfo(Vertex().GetVertexInfo());
    m_ShadowProgram.SetPushConstantRange(sizeof(ShadowMatrices), VK_SHADER_STAGE_VERTEX_BIT);
    m_ShadowProgram.SetDepthBiasEnabled(true);
    m_ShadowProgram.SetCulling(VK_CULL_MODE_NONE);
    m_ShadowProgram.Create();

    m_PointShadowShader.Create("resources/shaders/pointshadow.frag.spv", "resources/shaders/pointshadow.vert.spv");
    m_PointShadowProgram.SetShader(&m_PointShadowShader);
    m_PointShadowProgram.SetVertexInfo(Vertex().GetVertexInfo());
    m_PointShadowProgram.SetPushConstantRange(sizeof(PointShadowMatrices), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    m_PointShadowProgram.SetDepthBiasEnabled(true);
    m_PointShadowProgram.SetCulling(VK_CULL_MODE_NONE);
    m_PointShadowProgram.Create();

    m_ShadowResourcesCreated = true;
}

void Renderer::DestroyShadowResources() {
    m_PointShadowProgram.Destroy();
    m_PointShadowShader.Destroy();
    m_ShadowProgram.Destroy();
    m_ShadowShader.Destroy();
    m_ShadowResourcesCreated = false;
}

void Renderer::CreateDeferredLightingBuffers() {
    Engine* engine = Engine::GetInstance();
    m_DeferredLightingBuffers.resize(engine->GetMaxFramesInFlight());

    for (UniformBuffer& buffer : m_DeferredLightingBuffers) {
        buffer.buffer.AllocateBuffer(
            sizeof(DeferredLightingData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        vmaMapMemory(engine->GetAllocator(), buffer.buffer.GetAllocation(), &buffer.ptr);
    }
}

void Renderer::DestroyDeferredLightingBuffers() {
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
    for (UniformBuffer& buffer : m_DeferredLightingBuffers) {
        if (buffer.ptr != nullptr) {
            vmaUnmapMemory(allocator, buffer.buffer.GetAllocation());
            buffer.ptr = nullptr;
        }
        buffer.buffer.DeallocateBuffer();
    }
    m_DeferredLightingBuffers.clear();
}

void Renderer::CreatePointLightShadowResources() {
    const uint32_t bufferSize = sizeof(PointLightShadowData) * MAX_POINT_LIGHTS;
    m_PointLightShadowBuffer.buffer.AllocateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    vmaMapMemory(Engine::GetInstance()->GetAllocator(), m_PointLightShadowBuffer.buffer.GetAllocation(), &m_PointLightShadowBuffer.ptr);
    std::vector<PointLightShadowData> emptyData(MAX_POINT_LIGHTS);
    memcpy(m_PointLightShadowBuffer.ptr, emptyData.data(), bufferSize);

    const VkFormat depthFormat = Engine::GetInstance()->GetDepthBuffer().GetFormat();
    m_FallbackPointShadowMap.AllocateImage(
        1,
        1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    );
    m_FallbackPointShadowMap.AllocateView(VK_IMAGE_ASPECT_DEPTH_BIT);

    Engine::GetInstance()->GetAllocCMD().CmdTemp([&](VkCommandBuffer cmd) {
        ImageTransitionHelper helper(cmd);
        helper.Transition(m_FallbackPointShadowMap, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    });
    m_FallbackPointShadowMapLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void Renderer::DestroyPointLightShadowResources() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    for (PointLightShadowResource& resource : m_PointLightShadowResources) {
        for (VkImageView& view : resource.faceViews) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        }
        resource.image.DeallocateImage();
    }
    m_PointLightShadowResources.clear();
    m_ActivePointLightCount = 0;

    m_FallbackPointShadowMap.DeallocateImage();
    m_FallbackPointShadowMapLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (m_PointLightShadowBuffer.ptr != nullptr) {
        vmaUnmapMemory(Engine::GetInstance()->GetAllocator(), m_PointLightShadowBuffer.buffer.GetAllocation());
        m_PointLightShadowBuffer.ptr = nullptr;
    }
    m_PointLightShadowBuffer.buffer.DeallocateBuffer();
}

void Renderer::SyncPointLightShadowResources() {
    const std::vector<PointLight>& pointLights = Engine::GetInstance()->GetScene()->GetPointLights();
    const uint32_t pointLightCount = std::min<uint32_t>(static_cast<uint32_t>(pointLights.size()), MAX_POINT_LIGHTS);
    if (pointLightCount == m_ActivePointLightCount)
        return;

    VkDevice device = Engine::GetInstance()->GetDevice();
    for (PointLightShadowResource& resource : m_PointLightShadowResources) {
        for (VkImageView& view : resource.faceViews) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        }
        resource.image.DeallocateImage();
    }
    m_PointLightShadowResources.clear();
    m_PointLightShadowResources.resize(pointLightCount);

    const VkFormat depthFormat = Engine::GetInstance()->GetDepthBuffer().GetFormat();
    for (PointLightShadowResource& resource : m_PointLightShadowResources) {
        resource.image.AllocateImage(
            POINT_SHADOW_MAP_SIZE,
            POINT_SHADOW_MAP_SIZE,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            6,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
        );
        resource.image.AllocateView(VK_IMAGE_ASPECT_DEPTH_BIT);
        for (uint32_t face = 0; face < 6; ++face) {
            resource.faceViews[face] = resource.image.AllocateLayerView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, face);
        }
        resource.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    m_ActivePointLightCount = pointLightCount;
    m_PointLightDescriptorsDirty = true;
    m_IsShadowBuffered = false;
    UpdatePointLightShadowData();
}

void Renderer::UpdatePointLightShadowData() {
    if (m_PointLightShadowBuffer.ptr == nullptr)
        return;

    std::vector<PointLightShadowData> gpuData(MAX_POINT_LIGHTS);
    const std::vector<PointLight>& pointLights = Engine::GetInstance()->GetScene()->GetPointLights();
    const uint32_t pointLightCount = std::min<uint32_t>(m_ActivePointLightCount, static_cast<uint32_t>(pointLights.size()));

    for (uint32_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex) {
        const PointLight& light = pointLights[lightIndex];
        const float radius = fmaxf(light.radius, 0.01f);
        gpuData[lightIndex].positionRadius = glm::vec4(light.position, radius);
        gpuData[lightIndex].colorIntensity = glm::vec4(light.color, light.intensity);
    }

    memcpy(m_PointLightShadowBuffer.ptr, gpuData.data(), sizeof(PointLightShadowData) * gpuData.size());
}

void Renderer::UpdateDeferredLightingBuffer() {
    if (m_DeferredLightingBuffers.empty())
        return;

    DeferredLightingData lightingData = Engine::GetInstance()->GetScene()->GetLightingData();
    UniformBufferObject cameraData{};
    Camera::GetInstance().UpdateUniformBufferObject(cameraData);
    lightingData.inverseView = glm::inverse(cameraData.view);
    lightingData.inverseProjection = glm::inverse(cameraData.proj);
    lightingData.cameraPosition = glm::vec4(Camera::GetInstance().origin, static_cast<float>(m_ActivePointLightCount));
    UniformBuffer& buffer = m_DeferredLightingBuffers[Engine::GetInstance()->GetCurrentFrameIndex()];
    memcpy(buffer.ptr, &lightingData, sizeof(DeferredLightingData));
}

void Renderer::EnsureDefaultSkyboxTexture() {
    if (m_DefaultSkyboxTexture != nullptr)
        return;

    m_DefaultSkyboxTexture = Engine::GetInstance()->GetResourceLoader().LoadEXRCubemap("resources/images/skybox.exr", 512);
    if (m_DefaultSkyboxTexture == nullptr) {
        float blackCube[6 * 4]{};
        for (int face = 0; face < 6; ++face) {
            blackCube[face * 4 + 3] = 1.0f;
        }
        m_DefaultSkyboxTexture = Engine::GetInstance()->GetResourceManager().CreateTextureCube(blackCube, 1, VK_FORMAT_R32G32B32A32_SFLOAT, false);
    }
    if (m_SkyboxTexture == nullptr) {
        m_SkyboxTexture = m_DefaultSkyboxTexture;
    }
}

void Renderer::CreateDeferredResources() {
    Engine* engine = Engine::GetInstance();
    const uint32_t frameCount = engine->GetMaxFramesInFlight();
    const VkExtent2D extent = engine->GetSwapChainExtent();
    constexpr VkFormat deferredFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    m_DeferredOutputImages.resize(frameCount);
    m_DeferredOutputLayouts.resize(frameCount, VK_IMAGE_LAYOUT_UNDEFINED);
    for (AllocatedImage& image : m_DeferredOutputImages) {
        image.AllocateImage(
            extent.width,
            extent.height,
            deferredFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );
        image.AllocateView(VK_IMAGE_ASPECT_COLOR_BIT);
    }

    m_DeferredSampler.ClampEdge().Linear().EnableAnisotropy(false);
    m_DeferredSampler.Create();

    m_DeferredShader.Create("resources/shaders/deferred.frag.spv", "resources/shaders/deferred.vert.spv");

    m_DeferredProgram.SetShader(&m_DeferredShader);
    m_DeferredProgram.SetFormat(deferredFormat);
    m_DeferredProgram.SetVertexInputEnabled(false);
    m_DeferredProgram.SetDepthEnabled(false);
    m_DeferredProgram.SetCulling(VK_CULL_MODE_NONE);
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, 1);
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, 1);
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, 1);
    m_DeferredProgram.CreateUniform(sizeof(DeferredLightingData));
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, 1);
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, 1);
    m_DeferredProgram.CreateStorageBuffer(sizeof(PointLightShadowData) * MAX_POINT_LIGHTS);
    m_DeferredProgram.CreateSamplerUniform(&m_DeferredSampler, MAX_POINT_LIGHTS);
    m_DeferredProgram.Create();

    std::vector<VkDescriptorSetLayout> layouts(frameCount, m_DeferredProgram.GetDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DeferredProgram.GetDescriptorPool();
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    m_DeferredDescriptorSets.resize(layouts.size());
    if (vkAllocateDescriptorSets(engine->GetDevice(), &allocInfo, m_DeferredDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate deferred descriptor sets");
    }

    UpdateDeferredDescriptors();
}

void Renderer::DestroyDeferredResources() {
    m_DeferredDescriptorSets.clear();
    m_DeferredProgram.Destroy();
    m_DeferredShader.Destroy();
    m_DeferredSampler.Destroy();

    for (AllocatedImage& image : m_DeferredOutputImages) {
        image.DeallocateImage();
    }
    m_DeferredOutputImages.clear();
    m_DeferredOutputLayouts.clear();
}

void Renderer::UpdateDeferredDescriptors() {
    Engine* engine = Engine::GetInstance();

    for (uint32_t frame = 0; frame < m_DeferredDescriptorSets.size(); ++frame) {
        std::array<VkDescriptorImageInfo, 5> imageInfos{};
        std::array<VkDescriptorImageInfo, MAX_POINT_LIGHTS> pointShadowInfos{};

        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[0].imageView = m_RenderGBuffer.GetBuffer(frame).GetImageView(DeferredRenderBuffer::RENDER_ATTACHMENT_COLOR);
        imageInfos[0].sampler = m_DeferredSampler.GetSampler();

        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].imageView = m_RenderGBuffer.GetBuffer(frame).GetImageView(DeferredRenderBuffer::RENDER_ATTACHMENT_NORMAL);
        imageInfos[1].sampler = m_DeferredSampler.GetSampler();

        imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[2].imageView = m_RenderGBuffer.GetBuffer(frame).GetImageView(DeferredRenderBuffer::RENDER_ATTACHMENT_POSITION);
        imageInfos[2].sampler = m_DeferredSampler.GetSampler();

        imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[3].imageView = m_ShadowMap.GetImageView();
        imageInfos[3].sampler = m_DeferredSampler.GetSampler();

        imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[4].imageView = m_SkyboxTexture->GetImageView();
        imageInfos[4].sampler = m_DeferredSampler.GetSampler();

        VkDescriptorBufferInfo lightingInfo{};
        lightingInfo.buffer = m_DeferredLightingBuffers[frame].buffer.GetBuffer();
        lightingInfo.offset = 0;
        lightingInfo.range = sizeof(DeferredLightingData);

        VkDescriptorBufferInfo pointLightInfo{};
        pointLightInfo.buffer = m_PointLightShadowBuffer.buffer.GetBuffer();
        pointLightInfo.offset = 0;
        pointLightInfo.range = sizeof(PointLightShadowData) * MAX_POINT_LIGHTS;

        for (uint32_t index = 0; index < MAX_POINT_LIGHTS; ++index) {
            pointShadowInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            pointShadowInfos[index].imageView = index < m_PointLightShadowResources.size()
                ? m_PointLightShadowResources[index].image.GetImageView()
                : m_FallbackPointShadowMap.GetImageView();
            pointShadowInfos[index].sampler = m_DeferredSampler.GetSampler();
        }

        std::array<VkWriteDescriptorSet, 8> writes{};
        for (VkWriteDescriptorSet& write : writes) {
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_DeferredDescriptorSets[frame];
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
        }

        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &imageInfos[0];

        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imageInfos[1];

        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].pImageInfo = &imageInfos[2];

        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[3].pBufferInfo = &lightingInfo;

        writes[4].dstBinding = 4;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[4].pImageInfo = &imageInfos[3];

        writes[5].dstBinding = 5;
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[5].pImageInfo = &imageInfos[4];

        writes[6].dstBinding = 6;
        writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[6].pBufferInfo = &pointLightInfo;

        writes[7].dstBinding = 7;
        writes[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[7].descriptorCount = MAX_POINT_LIGHTS;
        writes[7].pImageInfo = pointShadowInfos.data();

        vkUpdateDescriptorSets(engine->GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Renderer::CreateTonemapResources() {
    Engine* engine = Engine::GetInstance();

    m_TonemapSampler.ClampEdge().Linear().EnableAnisotropy(false);
    m_TonemapSampler.Create();

    m_TonemapShader.Create("resources/shaders/tonemap.frag.spv", "resources/shaders/tonemap.vert.spv");

    m_TonemapProgram.SetShader(&m_TonemapShader);
    m_TonemapProgram.SetFormat(engine->GetSwapChainFormat());
    m_TonemapProgram.SetVertexInputEnabled(false);
    m_TonemapProgram.SetDepthEnabled(false);
    m_TonemapProgram.SetCulling(VK_CULL_MODE_NONE);
    m_TonemapProgram.CreateSamplerUniform(&m_TonemapSampler, 1);
    m_TonemapProgram.Create();

    std::vector<VkDescriptorSetLayout> layouts(engine->GetMaxFramesInFlight(), m_TonemapProgram.GetDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_TonemapProgram.GetDescriptorPool();
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    m_TonemapDescriptorSets.resize(layouts.size());
    if (vkAllocateDescriptorSets(engine->GetDevice(), &allocInfo, m_TonemapDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate tonemap descriptor sets");
    }

    UpdateTonemapDescriptors();
}

void Renderer::DestroyTonemapResources() {
    m_TonemapDescriptorSets.clear();
    m_TonemapProgram.Destroy();
    m_TonemapShader.Destroy();
    m_TonemapSampler.Destroy();
}

void Renderer::UpdateTonemapDescriptors() {
    Engine* engine = Engine::GetInstance();

    for (uint32_t frame = 0; frame < m_TonemapDescriptorSets.size(); ++frame) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_DeferredOutputImages[frame].GetImageView();
        imageInfo.sampler = m_TonemapSampler.GetSampler();

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_TonemapDescriptorSets[frame];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(engine->GetDevice(), 1, &write, 0, nullptr);
    }
}

void Renderer::RenderShadowPrepass(VkCommandBuffer commandBuffer, VkImageView targetView, VkExtent2D extent, const m4& lightViewProjection) {
    VkClearValue depthClear{};
    depthClear.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = targetView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowProgram.GetVkPipeline());
    vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

    Scene* scene = Engine::GetInstance()->GetScene();
    ShadowMatrices shadowMatrices{};
    shadowMatrices.lightViewProjection = lightViewProjection;

    for (ShaderProgram* shaderProgram : scene->GetShaders()) {
        for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects(shaderProgram)) {
            if (object->mesh == nullptr)
                continue;

            m4 modelMatrix = glm::identity<glm::mat4>();
            modelMatrix = glm::translate(modelMatrix, object->position);
            modelMatrix *= glm::mat4_cast(object->rotation);
            modelMatrix = glm::scale(modelMatrix, object->scale);
            shadowMatrices.model = modelMatrix;

            vkCmdPushConstants(commandBuffer, m_ShadowProgram.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowMatrices), &shadowMatrices);

            VkBuffer vertexBuffers[] { object->mesh->GetVKVertexBuffer() };
            VkDeviceSize offsets[] {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, object->mesh->GetVKIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, object->mesh->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    vkCmdEndRendering(commandBuffer);
}

void Renderer::RenderPointShadowPrepass(VkCommandBuffer commandBuffer, VkImageView targetView, VkExtent2D extent, const PointLight& light, const m4& lightViewProjection) {
    VkClearValue depthClear{};
    depthClear.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = targetView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PointShadowProgram.GetVkPipeline());
    vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

    Scene* scene = Engine::GetInstance()->GetScene();
    PointShadowMatrices shadowMatrices{};
    shadowMatrices.lightViewProjection = lightViewProjection;
    shadowMatrices.lightPositionRadius = glm::vec4(light.position, fmaxf(light.radius, 0.01f));

    for (ShaderProgram* shaderProgram : scene->GetShaders()) {
        for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects(shaderProgram)) {
            if (object->mesh == nullptr)
                continue;

            m4 modelMatrix = glm::identity<glm::mat4>();
            modelMatrix = glm::translate(modelMatrix, object->position);
            modelMatrix *= glm::mat4_cast(object->rotation);
            modelMatrix = glm::scale(modelMatrix, object->scale);
            shadowMatrices.model = modelMatrix;

            vkCmdPushConstants(commandBuffer, m_PointShadowProgram.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointShadowMatrices), &shadowMatrices);

            VkBuffer vertexBuffers[] { object->mesh->GetVKVertexBuffer() };
            VkDeviceSize offsets[] {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, object->mesh->GetVKIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, object->mesh->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    vkCmdEndRendering(commandBuffer);
}

void Renderer::RenderShadowMap(VkCommandBuffer commandBuffer) {
    EnsureShadowResources();
    SyncPointLightShadowResources();
    UpdatePointLightShadowData();

    if (m_IsShadowBuffered) {
        if (m_PointLightDescriptorsDirty) {
            UpdateDeferredDescriptors();
            m_PointLightDescriptorsDirty = false;
        }
        return;
    }

    ImageTransitionHelper transitionHelper(commandBuffer);
    if (m_ShadowMapLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        transitionHelper.Transition(m_ShadowMap, m_ShadowMapLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_ShadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    RenderShadowPrepass(commandBuffer, m_ShadowMap.GetImageView(), {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}, Engine::GetInstance()->GetScene()->GetLightingData().lightViewProjection);

    transitionHelper.Transition(m_ShadowMap, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    m_ShadowMapLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    RenderPointLightShadowMaps(commandBuffer);
    m_IsShadowBuffered = true;
}

void Renderer::RenderPointLightShadowMaps(VkCommandBuffer commandBuffer) {
    const std::vector<PointLight>& pointLights = Engine::GetInstance()->GetScene()->GetPointLights();
    const uint32_t pointLightCount = std::min<uint32_t>(m_ActivePointLightCount, static_cast<uint32_t>(pointLights.size()));
    if (pointLightCount == 0) {
        if (m_PointLightDescriptorsDirty) {
            UpdateDeferredDescriptors();
            m_PointLightDescriptorsDirty = false;
        }
        return;
    }

    const std::array<v3, 6> directions = {
        v3{ 1.0f,  0.0f,  0.0f},
        v3{-1.0f,  0.0f,  0.0f},
        v3{ 0.0f,  1.0f,  0.0f},
        v3{ 0.0f, -1.0f,  0.0f},
        v3{ 0.0f,  0.0f,  1.0f},
        v3{ 0.0f,  0.0f, -1.0f}
    };
    const std::array<v3, 6> ups = {
        v3{0.0f, -1.0f,  0.0f},
        v3{0.0f, -1.0f,  0.0f},
        v3{0.0f,  0.0f,  1.0f},
        v3{0.0f,  0.0f, -1.0f},
        v3{0.0f, -1.0f,  0.0f},
        v3{0.0f, -1.0f,  0.0f}
    };

    ImageTransitionHelper transitionHelper(commandBuffer);
    for (uint32_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex) {
        PointLightShadowResource& resource = m_PointLightShadowResources[lightIndex];
        const PointLight& light = pointLights[lightIndex];

        if (resource.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            transitionHelper.Transition(resource.image, resource.layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
            resource.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        const float radius = fmaxf(light.radius, 0.01f);
        m4 projection = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, 0.01f, radius);
        projection[1][1] *= -1.0f;

        for (uint32_t face = 0; face < 6; ++face) {
            const m4 view = glm::lookAtLH(light.position, light.position + directions[face], ups[face]);
            RenderPointShadowPrepass(commandBuffer, resource.faceViews[face], {POINT_SHADOW_MAP_SIZE, POINT_SHADOW_MAP_SIZE}, light, projection * view);
        }

        transitionHelper.Transition(resource.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        resource.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (m_PointLightDescriptorsDirty) {
        UpdateDeferredDescriptors();
        m_PointLightDescriptorsDirty = false;
    }
}

void Renderer::RenderGBuffer(uint32_t imageIndex, VkCommandBuffer commandBuffer) {
    SwapChain* swapChain = Engine::GetInstance()->GetSwapChain();
    ImageTransitionHelper transitionHelper(commandBuffer);
    GBuffer& gBuffer = m_RenderGBuffer.GetBuffer();
    gBuffer.TransitionImages(transitionHelper, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue depthClear{};
    depthClear.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = Engine::GetInstance()->GetDepthBuffer().GetImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue = depthClear;

    const std::vector<VkRenderingAttachmentInfo>& colorAttachments = gBuffer.GetColorAttachmentRenderingInfo();

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = Engine::GetInstance()->GetSwapChainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();

    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetExtent().width);
    viewport.height = static_cast<float>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    Record(imageIndex, commandBuffer);

    vkCmdEndRendering(commandBuffer);

    gBuffer.TransitionImages(transitionHelper, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::RenderDeferredShading(VkCommandBuffer commandBuffer) {
    Engine* engine = Engine::GetInstance();
    const uint32_t frame = engine->GetCurrentFrameIndex();
    SwapChain* swapChain = engine->GetSwapChain();

    ImageTransitionHelper transitionHelper(commandBuffer);
    if (m_DeferredOutputLayouts[frame] != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        transitionHelper.Transition(
            m_DeferredOutputImages[frame],
            m_DeferredOutputLayouts[frame],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_DeferredOutputLayouts[frame] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkClearValue colorClear{};
    colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_DeferredOutputImages[frame].GetImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = colorClear;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = swapChain->GetExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetExtent().width);
    viewport.height = static_cast<float>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDescriptorSet descriptorSet = m_DeferredDescriptorSets[frame];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DeferredProgram.GetVkPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DeferredProgram.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    transitionHelper.Transition(
        m_DeferredOutputImages[frame],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    m_DeferredOutputLayouts[frame] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void Renderer::RenderTonemap(uint32_t imageIndex, VkCommandBuffer commandBuffer) {
    SwapChain* swapChain = Engine::GetInstance()->GetSwapChain();
    ImageTransitionHelper transitionHelper(commandBuffer);
    transitionHelper.Transition(swapChain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    VkClearValue colorClear{};
    colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapChain->GetImageView(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = colorClear;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = swapChain->GetExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetExtent().width);
    viewport.height = static_cast<float>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDescriptorSet descriptorSet = m_TonemapDescriptorSets[Engine::GetInstance()->GetCurrentFrameIndex()];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TonemapProgram.GetVkPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TonemapProgram.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    transitionHelper.Transition(swapChain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::Record(uint32_t imageIndex, VkCommandBuffer commandBuffer) {
    UniformBufferObject buffer{};
    Camera::GetInstance().UpdateUniformBufferObject(buffer);
    buffer.lightDirection = Engine::GetInstance()->GetScene()->GetLightingData().lightDirection;

    for (ShaderProgram* shaderProgram : Engine::GetInstance()->GetScene()->GetShaders()) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderProgram->GetVkPipeline());

        for (const std::unique_ptr<GameObject>& object : Engine::GetInstance()->GetScene()->GetGameObjects(shaderProgram)) {
            if (object->mesh != nullptr) {

                Material* material = object->material;
                if (material == nullptr)
                    continue;

                m4 modelMatrix = glm::identity<glm::mat4>();
                modelMatrix = glm::translate(modelMatrix, object->position);
                modelMatrix *= glm::mat4_cast(object->rotation);
                modelMatrix = glm::scale(modelMatrix, object->scale);
                buffer.model = modelMatrix;

                VkBuffer vertexBuffers[] { object->mesh->GetVKVertexBuffer() };
                VkDeviceSize offsets[] {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

                // I buffer in uint32 for larger meshes, assuming that these will get buffered at some point idk though ill just keep it with a lot to be safe
                vkCmdBindIndexBuffer(commandBuffer, object->mesh->GetVKIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                material->WriteToUniform(0, &buffer);

                vkCmdPushConstants(commandBuffer, shaderProgram->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &object->push);

                VkDescriptorSet descriptorSet = material->GetDescriptorSet();
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderProgram->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, object->mesh->GetIndexCount(), 1, 0, 0, 0);
            }
        }
    }
}

Renderer::Renderer() {
    CreateFrameResources();
}

Renderer::~Renderer() {
    DestroyFrameResources();
}

void Renderer::SetSkyboxTexture(TextureCube* texture) {
    m_SkyboxTexture = texture != nullptr ? texture : m_DefaultSkyboxTexture;

    if (!m_DeferredDescriptorSets.empty() && m_SkyboxTexture != nullptr) {
        UpdateDeferredDescriptors();
    }
}

void Renderer::RecreateFrameResources() {
    DestroyFrameResources();
    CreateFrameResources();
}

void Renderer::RenderFrame(uint32_t imageIndex, VkCommandBuffer commandBuffer) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    RenderShadowMap(commandBuffer);
    UpdateDeferredLightingBuffer();
    RenderGBuffer(imageIndex, commandBuffer);
    RenderDeferredShading(commandBuffer);
    RenderTonemap(imageIndex, commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
