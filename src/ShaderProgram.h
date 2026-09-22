//
// Created by frane on 5/4/2026.
//

#ifndef GPVKFR_DYNAMICRENDERPIPELINE_H
#define GPVKFR_DYNAMICRENDERPIPELINE_H
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "structs.h"

class Shader;
class IGBuffer;
class Sampler;

enum class UniformType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat4,
    Sampler2D
};

class ShaderProgram {
    VkPipeline m_Pipeline{nullptr};
    VkPipelineLayout m_PipelineLayout{nullptr};
    VkDescriptorSetLayout m_DescriptorSetLayout{nullptr};
    VkDescriptorPool m_DescriptorPool{nullptr};

    std::vector<UniformBinding> m_UniformBindings{};
    uint32_t m_PushConstantRange{0};
    VkShaderStageFlags m_PushConstantStages{VK_SHADER_STAGE_FRAGMENT_BIT};

    std::vector<VkFormat> m_Formats;
    const Shader* m_Shader{nullptr};
    VertexInfo m_VertexInfo{};
    VkCullModeFlags m_Culling{VK_CULL_MODE_BACK_BIT};
    VkFrontFace m_FrontFace{VK_FRONT_FACE_CLOCKWISE};
    uint32_t m_MaxMaterialCount{1};
    bool m_UseVertexInput{true};
    bool m_UseDepth{true};
    bool m_UseDepthBias{false};

    bool m_Initialized{false};

    template<uint32_t N>
    struct DynamicState {
        VkDynamicState dynamicStates[N];
        uint32_t dynamicStateCount;
        VkPipelineDynamicStateCreateInfo createInfo;
    };

    struct ViewportState {
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineViewportStateCreateInfo createInfo;
    };

    struct BlendAttachmentState {
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        VkPipelineColorBlendStateCreateInfo createInfo;
    };

    DynamicState<3> fillDynamicStateCreateInfo();

    VkPipelineVertexInputStateCreateInfo fillVertexInputState();
    VkPipelineInputAssemblyStateCreateInfo fillInputAssemblyState();
    ViewportState fillViewportState();
    VkPipelineRasterizationStateCreateInfo fillRasterizerStateCreateInfo();
    VkPipelineMultisampleStateCreateInfo fillMultisampleStateCreateInfo();
    BlendAttachmentState fillBlendAttachmentState();

    void AllocateAndMapBuffers();
    void AllocateDescriptorSetLayout();
    void AllocateDescriptorPool();
    void AllocatePipelineLayout();

    void InitializePipeline();
    void InitializeUniformBindings();

public:
    ShaderProgram() = default;

    // Allocate pipeline, pool, layouts
    void Create();
    void Destroy();
    // -1 if the uniform is not yet supported or failed
    UniformIdx CreateUniform(UniformType type);
    // -1 if the uniform is not yet supported or failed
    UniformIdx CreateUniform(uint32_t length);
    UniformIdx CreateStorageBuffer(uint32_t length);
    // -1 if the uniform is not yet supported or failed
    UniformIdx CreateSamplerUniform(Sampler* sampler, uint32_t count = 32);
    void SetPushConstantRange(uint32_t size, VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT);

    void SetGBufferFormats(const IGBuffer* buffer);
    void SetFormat(VkFormat format);

    void SetShader(const Shader* shader);
    void SetVertexInfo(VertexInfo vertexInfo);
    void SetCulling(VkCullModeFlags culling);
    void SetFrontFace(VkFrontFace frontFace);
    void SetMaxMaterialCount(uint32_t count);
    void SetVertexInputEnabled(bool enabled);
    void SetDepthEnabled(bool enabled);
    void SetDepthBiasEnabled(bool enabled);

    VkPipeline GetVkPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
    const std::vector<UniformBinding>& GetUniformBindings() const { return m_UniformBindings; }
    const UniformBinding& GetUniformBinding(UniformIdx idx) const;
};


#endif //GPVKFR_DYNAMICRENDERPIPELINE_H
