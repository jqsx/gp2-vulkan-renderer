//
// Created by frane on 5/4/2026.
//

#include "ShaderProgram.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>

#include "Engine.h"
#include "Sampler.h"
#include "Shader.h"

ShaderProgram::DynamicState<3> ShaderProgram::fillDynamicStateCreateInfo() {
    DynamicState<3> dyn{};
    dyn.dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dyn.dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
    dyn.dynamicStates[2] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    dyn.dynamicStateCount = 3;
    dyn.createInfo = {};

    dyn.createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.createInfo.dynamicStateCount = (uint32_t)dyn.dynamicStateCount;
    dyn.createInfo.pDynamicStates = dyn.dynamicStates;

    return dyn;
}

VkPipelineVertexInputStateCreateInfo ShaderProgram::fillVertexInputState() {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (m_UseVertexInput) {
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = m_VertexInfo.attributes.size();
        vertexInputInfo.pVertexAttributeDescriptions = m_VertexInfo.attributes.data();
        vertexInputInfo.pVertexBindingDescriptions = &m_VertexInfo.binding;
    }
    return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo ShaderProgram::fillInputAssemblyState() {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // I dont think we are going to be rendering out anything other than triangle lists
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return inputAssembly;
}

ShaderProgram::ViewportState ShaderProgram::fillViewportState() {
    VkExtent2D swapchainExtent = Engine::GetInstance()->GetSwapChainExtent();
    ViewportState state = {};

    state.viewport.x = 0.0f;
    state.viewport.y = 0.0f;
    state.viewport.width = (float)swapchainExtent.width;
    state.viewport.height = (float)swapchainExtent.height;
    state.viewport.minDepth = 0.0f;
    state.viewport.maxDepth = 1.0f;

    state.scissor.offset = {0, 0};
    state.scissor.extent = swapchainExtent;

    state.createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    state.createInfo.viewportCount = 1;
    state.createInfo.pViewports = &state.viewport;
    state.createInfo.scissorCount = 1;
    state.createInfo.pScissors = &state.scissor;

    return state;
}

VkPipelineRasterizationStateCreateInfo ShaderProgram::fillRasterizerStateCreateInfo() {
    VkPipelineRasterizationStateCreateInfo rasterizer = {};

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = m_Culling;
    rasterizer.frontFace = m_FrontFace;

    rasterizer.depthBiasEnable = m_UseDepthBias ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    return rasterizer;
}

VkPipelineMultisampleStateCreateInfo ShaderProgram::fillMultisampleStateCreateInfo() {
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return multisampling;
}

ShaderProgram::BlendAttachmentState ShaderProgram::fillBlendAttachmentState() {
    BlendAttachmentState state{};

    state.colorBlendAttachments.resize(m_Formats.size());
    for (VkPipelineColorBlendAttachmentState& colorBlendAttachment : state.colorBlendAttachments) {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // TODO: change if got to blending

        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    }

    state.createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    state.createInfo.logicOpEnable = VK_FALSE;
    state.createInfo.logicOp = VK_LOGIC_OP_COPY;
    state.createInfo.attachmentCount = static_cast<uint32_t>(state.colorBlendAttachments.size());
    state.createInfo.pAttachments = state.colorBlendAttachments.data();

    return state;
}

void ShaderProgram::AllocateAndMapBuffers() {

}

void ShaderProgram::AllocateDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    bindings.resize(m_UniformBindings.size());

    for (int index = 0; index < bindings.size(); ++index) {
        UniformBinding& binding = m_UniformBindings[index];
        VkDescriptorSetLayoutBinding& layoutBinding = bindings[index];
        layoutBinding.binding = index;
        layoutBinding.descriptorCount = binding.count;
        layoutBinding.descriptorType = binding.descriptorType;
        layoutBinding.pImmutableSamplers = nullptr;

        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = bindings.size();
    setLayoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(Engine::GetInstance()->GetDevice(), &setLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create descriptor set layout.");
        throw std::runtime_error("Failed to create descriptor set layout.");
    }
}

void ShaderProgram::AllocateDescriptorPool() {
    if (m_UniformBindings.empty()) {
        m_DescriptorPool = VK_NULL_HANDLE;
        return;
    }

    uint32_t maxFramesInFlight = Engine::GetInstance()->GetMaxFramesInFlight();
    std::vector<VkDescriptorPoolSize> poolSizes;

    for (const UniformBinding& binding : m_UniformBindings) {
        const uint32_t descriptorCount = maxFramesInFlight * m_MaxMaterialCount * binding.count;
        auto poolSizeIt = std::find_if(poolSizes.begin(), poolSizes.end(), [&](const VkDescriptorPoolSize& poolSize) {
            return poolSize.type == binding.descriptorType;
        });

        if (poolSizeIt != poolSizes.end()) {
            poolSizeIt->descriptorCount += descriptorCount;
        }
        else {
            poolSizes.push_back({binding.descriptorType, descriptorCount});
        }
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxFramesInFlight * m_MaxMaterialCount;

    if (vkCreateDescriptorPool(Engine::GetInstance()->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create descriptor pool.");
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void ShaderProgram::AllocatePipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = m_PushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = m_PushConstantStages;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = m_PushConstantRange > 0 ? 1 : 0;
    pipelineLayoutInfo.pPushConstantRanges = m_PushConstantRange > 0 ? &pushConstantRange : nullptr;

    if (vkCreatePipelineLayout(Engine::GetInstance()->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to initialize vk pipeline layout.");
        throw std::runtime_error("Failed to initialize pipeline layout");
    }
}

void ShaderProgram::InitializePipeline() {
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    renderingInfo.colorAttachmentCount = m_Formats.size();
    renderingInfo.pColorAttachmentFormats = m_Formats.data();
    renderingInfo.depthAttachmentFormat = m_UseDepth ? Engine::GetInstance()->GetDepthBuffer().GetFormat() : VK_FORMAT_UNDEFINED;
    renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    graphicsPipelineCreateInfo.pNext = &renderingInfo;
    graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.subpass = 0;

    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderInfos = m_Shader->GetInfos();

    graphicsPipelineCreateInfo.pStages = shaderInfos.data();
    graphicsPipelineCreateInfo.stageCount = shaderInfos.size();

    DynamicState<3> dynamicState = fillDynamicStateCreateInfo();
    graphicsPipelineCreateInfo.pDynamicState = &dynamicState.createInfo;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = fillVertexInputState();
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = fillInputAssemblyState();
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;

    ViewportState viewportState = fillViewportState();
    graphicsPipelineCreateInfo.pViewportState = &viewportState.createInfo;

    VkPipelineMultisampleStateCreateInfo multisampling = fillMultisampleStateCreateInfo();
    graphicsPipelineCreateInfo.pMultisampleState = &multisampling;

    VkPipelineRasterizationStateCreateInfo rasterizer = fillRasterizerStateCreateInfo();
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;

    BlendAttachmentState blendAttachmentState = fillBlendAttachmentState();
    graphicsPipelineCreateInfo.pColorBlendState = &blendAttachmentState.createInfo;

    graphicsPipelineCreateInfo.pDepthStencilState = m_UseDepth ? &Engine::GetInstance()->GetDepthBuffer().GetDepthStencil() : nullptr;
    graphicsPipelineCreateInfo.layout = m_PipelineLayout;

    if (vkCreateGraphicsPipelines(Engine::GetInstance()->GetDevice(), nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pipeline!");
}

void ShaderProgram::InitializeUniformBindings() {
    AllocateAndMapBuffers();
    AllocateDescriptorSetLayout();
    AllocatePipelineLayout();
    AllocateDescriptorPool();
}

void ShaderProgram::Create() {
    if (m_Initialized)
        return;
    InitializeUniformBindings();
    InitializePipeline();

    m_Initialized = true;
}

void ShaderProgram::Destroy() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    if (m_Pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    if (m_PipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    if (m_DescriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

    m_Pipeline = VK_NULL_HANDLE;
    m_PipelineLayout = VK_NULL_HANDLE;
    m_DescriptorPool = VK_NULL_HANDLE;
    m_DescriptorSetLayout = VK_NULL_HANDLE;
    m_UniformBindings.clear();
    m_Initialized = false;
}

UniformIdx ShaderProgram::CreateUniform(UniformType type) {
    if (m_Initialized)
        return -1;
    switch (type) {
        case UniformType::Float:
            return CreateUniform(sizeof(float));
        case UniformType::Vec2:
            return CreateUniform(sizeof(float) * 2.0f);
        case UniformType::Vec3:
            return CreateUniform(sizeof(float) * 3.0f);
        case UniformType::Vec4:
            return CreateUniform(sizeof(float) * 4.0f);
        case UniformType::Mat4:
            return CreateUniform(sizeof(float) * 16.0f);
        case UniformType::Sampler2D:
            return -1;
    }
    return -1;
}

UniformIdx ShaderProgram::CreateUniform(uint32_t length) {
    if (m_Initialized)
        return -1;
    UniformBinding binding = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        length,
        nullptr
    };
    m_UniformBindings.emplace_back(std::move(binding));
    return (UniformIdx)m_UniformBindings.size() - 1;
}

UniformIdx ShaderProgram::CreateStorageBuffer(uint32_t length) {
    if (m_Initialized)
        return -1;
    UniformBinding binding = {
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        length,
        nullptr
    };
    m_UniformBindings.emplace_back(std::move(binding));
    return (UniformIdx)m_UniformBindings.size() - 1;
}

UniformIdx ShaderProgram::CreateSamplerUniform(Sampler* sampler, uint32_t count) {
    if (m_Initialized)
        return -1;

    UniformBinding binding = {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        0,
        sampler,
        count
    };
    m_UniformBindings.emplace_back(std::move(binding));
    return (UniformIdx)m_UniformBindings.size() - 1;
}

void ShaderProgram::SetPushConstantRange(uint32_t size, VkShaderStageFlags stages) {
    if (m_Initialized)
        return;
    m_PushConstantRange = size;
    m_PushConstantStages = stages;
}

void ShaderProgram::SetGBufferFormats(const IGBuffer *buffer) {
    if (m_Initialized)
        return;
    m_Formats = buffer->GetFormats();
}

void ShaderProgram::SetFormat(VkFormat format) {
    if (m_Initialized)
        return;
    if (m_Formats.empty())
        m_Formats.emplace_back(format);
    else
        m_Formats.back() = format;
}

void ShaderProgram::SetShader(const Shader *shader) {
    if (m_Initialized)
        return;
    m_Shader = shader;
}

void ShaderProgram::SetVertexInfo(VertexInfo vertexInfo) {
    if (m_Initialized)
        return;
    m_VertexInfo = std::move(vertexInfo);
}

void ShaderProgram::SetCulling(VkCullModeFlags culling) {
    if (m_Initialized)
        return;
    m_Culling = culling;
}

void ShaderProgram::SetFrontFace(VkFrontFace frontFace) {
    if (m_Initialized)
        return;
    m_FrontFace = frontFace;
}

void ShaderProgram::SetMaxMaterialCount(uint32_t count) {
    if (m_Initialized)
        return;
    m_MaxMaterialCount = std::max<uint32_t>(1, count);
}

void ShaderProgram::SetVertexInputEnabled(bool enabled) {
    if (m_Initialized)
        return;
    m_UseVertexInput = enabled;
}

void ShaderProgram::SetDepthEnabled(bool enabled) {
    if (m_Initialized)
        return;
    m_UseDepth = enabled;
}

void ShaderProgram::SetDepthBiasEnabled(bool enabled) {
    if (m_Initialized)
        return;
    m_UseDepthBias = enabled;
}

const UniformBinding & ShaderProgram::GetUniformBinding(UniformIdx idx) const {
    return m_UniformBindings[idx];
}
