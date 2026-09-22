//
// Created by frane on 3/27/2026.
//

#ifndef RENDERPIPELINE_H
#define RENDERPIPELINE_H
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "AllocatedBuffer.h"
#include "RenderPass.h"
#include "structs.h"

class Sampler;
class Texture2D;

// Refactor to take in Vertex and UniformBufferObject template to make it possible for exchanging of attributes and uniform types easily
class RenderPipeline {
    static constexpr uint32_t SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH{32};

    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    RenderPass* m_RenderPass;
    VertexInfo m_VertexInfo;
    uint32_t m_PushConstantRange;

    std::vector<UniformBinding> m_UniformBindings;

    std::string m_FragPath;
    std::string m_VertPath;

    static std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);

public:
    RenderPipeline();

    ~RenderPipeline();

    void UpdateUniformBuffer(uint32_t currentFrame, uint32_t descriptionIndex, const void* data);

    VkDescriptorSet& GetDescriptorSet(uint32_t imageIndex);

    VkPipeline GetVkPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
    VkRenderPass GetVkRenderPass() const { return m_RenderPass->GetRenderPass(); }
    RenderPass* GetRenderPass() const { return m_RenderPass; }
    uint32_t GetPushConstantRange() const { return m_PushConstantRange; }

    RenderPipeline& Paths(const std::string& fragPath, const std::string& vertPath);
    RenderPipeline& VertexInfo(const VertexInfo& vertexInfo);
    RenderPipeline& RenderPass(RenderPass* renderPass);
    RenderPipeline& PushConstantRange(uint32_t size);
    // Returns binding index
    uint32_t AddUniformObjectBinding(uint32_t size);
    // Returns binding index
    uint32_t AddSamplerArrayBinding(const std::vector<Texture2D*>& textures, Sampler* sampler);

    uint32_t TempGetTextureCount() const { return m_UniformBindings[1].texture.size(); }

    InitResult Create();
    void Destroy();
};


#endif //RENDERPIPELINE_H
