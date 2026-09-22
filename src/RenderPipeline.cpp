//
// Created by frane on 3/27/2026.
//

#include "RenderPipeline.h"

#include <filesystem>
#include <fstream>

#include "Engine.h"
#include "Logger.h"
#include "Sampler.h"

std::vector<char> RenderPipeline::readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file " + path);
    }

    uintmax_t size = std::filesystem::file_size(path);

    std::vector<char> buffer;

    buffer.resize(size);

    file.read(buffer.data(), size);

    file.close();

    return buffer;
}

VkShaderModule RenderPipeline::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(Engine::GetInstance()->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module.");
    }

    return shaderModule;
}

RenderPipeline::RenderPipeline() : m_PipelineLayout(), m_Pipeline(), m_DescriptorSetLayout(),
                                   m_DescriptorPool(),
                                   m_RenderPass(),
                                   m_VertexInfo() {
    PushConstantRange(0);
}

RenderPipeline & RenderPipeline::Paths(const std::string &fragPath, const std::string &vertPath) {
    m_FragPath = fragPath;
    m_VertPath = vertPath;
    return *this;
}

RenderPipeline & RenderPipeline::VertexInfo(const ::VertexInfo &vertexInfo) {
    m_VertexInfo = vertexInfo;
    return *this;
}

RenderPipeline & RenderPipeline::RenderPass(::RenderPass *renderPass) {
    m_RenderPass = renderPass;
    return *this;
}

RenderPipeline & RenderPipeline::PushConstantRange(uint32_t size) {
    m_PushConstantRange = size;
    return *this;
}

uint32_t RenderPipeline::AddUniformObjectBinding(uint32_t size) {
    UniformBinding& binding = m_UniformBindings.emplace_back();
    binding.size = size;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    return m_UniformBindings.size() - 1;
}

uint32_t RenderPipeline::AddSamplerArrayBinding(const std::vector<Texture2D*>& textures, Sampler* sampler) {
    UniformBinding& binding = m_UniformBindings.emplace_back();
    binding.size = -1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.sampler = sampler;
    binding.texture = textures;
    return m_UniformBindings.size() - 1;
}

InitResult RenderPipeline::Create() {
    std::vector<char> vertShaderCode = readFile(m_VertPath);
    std::vector<char> fragShaderCode = readFile(m_FragPath);

    uint32_t maxFramesInFlight = Engine::GetInstance()->GetMaxFramesInFlight();
    VkExtent2D swapchainExtent = Engine::GetInstance()->GetSwapChainExtent();

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};

    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};

    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
    dynamicState.pDynamicStates = dynamicStateEnables.data();

    auto bindingDescription = m_VertexInfo.binding;
    auto attributeDescriptions = m_VertexInfo.attributes;

    // This setup accepts that the mesh class is the only renderable form of mesh that can be drawn by the scene.
    // It is required to change this information about the input binding if the attribute layout per vertex is different for the mesh.
    // That is bit mid but its ok easy == just dont change the vertex layout and just pack it with everything easy peasy

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1; // Instancing
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // Attrib descriptor (Vertices and basically how to divide the appended buffer)
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;/* VK_FRONT_FACE_COUNTER_CLOCKWISE;*/

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasClamp = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // TODO: change if got to blending

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Uniform Description Set Creation

    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    bindings.resize(m_UniformBindings.size());

    for (int index = 0; index < bindings.size(); ++index) {
        UniformBinding& binding = m_UniformBindings[index];
        VkDescriptorSetLayoutBinding& layoutBinding = bindings[index];
        layoutBinding.binding = index;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = binding.descriptorType;
        layoutBinding.pImmutableSamplers = nullptr;

        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBinding.descriptorCount = SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH;
        }
    }

    // uboLayoutBinding.pImmutableSamplers = nullptr; // For images next chapter

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = bindings.size();
    setLayoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(Engine::GetInstance()->GetDevice(), &setLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create descriptor set layout.");
        return INIT_FAIL;
    }

    // Temporary solution but for now everything should be possible to be kept within one push constant struct
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = m_PushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; // This is the uniform layout, basically glGetUniform() equivalent of but as descriptors I assume. // yep supplied descriptor set layout

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(Engine::GetInstance()->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to initialize vk pipeline layout.");
        return INIT_FAIL;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &Engine::GetInstance()->GetDepthBuffer().GetDepthStencil();
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass->GetRenderPass();
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(Engine::GetInstance()->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create graphics pipeline.");
        return INIT_FAIL;
    }

    vkDestroyShaderModule(Engine::GetInstance()->GetDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(Engine::GetInstance()->GetDevice(), fragShaderModule, nullptr);

    // uniform buffers

    for (UniformBinding& binding : m_UniformBindings) {
        if (binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            continue;
        binding.uniformBuffers.resize(maxFramesInFlight);
        VkDeviceSize bufferSize = binding.size;
        VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
        for (int index = 0; index < maxFramesInFlight; ++index) {
            binding.uniformBuffers[index].buffer.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            vmaMapMemory(allocator, binding.uniformBuffers[index].buffer.GetAllocation(), &binding.uniformBuffers[index].ptr);
        }
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(m_UniformBindings.size());
    // poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // poolSizes[0].descriptorCount = maxFramesInFlight;
    // poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // poolSizes[1].descriptorCount = maxFramesInFlight;
    for (int index = 0; index < poolSizes.size(); ++index) {
        poolSizes[index].type = m_UniformBindings[index].descriptorType;
        poolSizes[index].descriptorCount = maxFramesInFlight * (m_UniformBindings[index].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? 1 : SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxFramesInFlight;

    if (vkCreateDescriptorPool(Engine::GetInstance()->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create descriptor pool.");
        return INIT_FAIL;
    }

    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.descriptorSetCount = maxFramesInFlight;

    m_DescriptorSets.resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(Engine::GetInstance()->GetDevice(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to allocate descriptor sets.");
        return INIT_FAIL;
    }

    for (int index = 0; index < maxFramesInFlight; ++index) {
        std::vector<VkWriteDescriptorSet> writeDescriptors{};
        writeDescriptors.resize(m_UniformBindings.size());
        int bindingIndex = 0;
        std::vector<std::vector<VkDescriptorImageInfo>> imageInfosList;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        for (UniformBinding& binding : m_UniformBindings) {
            VkWriteDescriptorSet& writeDescriptor = writeDescriptors[bindingIndex];
            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back();
                bufferInfo.buffer = binding.uniformBuffers[index].buffer.GetBuffer();
                bufferInfo.offset = 0;
                bufferInfo.range = binding.size;

                writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptor.dstSet = m_DescriptorSets[index];
                writeDescriptor.dstBinding = bindingIndex;
                writeDescriptor.descriptorCount = 1;
                writeDescriptor.pBufferInfo = &bufferInfo;
                writeDescriptor.pImageInfo = nullptr; // textures
                writeDescriptor.dstArrayElement = 0;
                writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                std::vector<VkDescriptorImageInfo>& imageInfos = imageInfosList.emplace_back();
                imageInfos.resize(SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH);
                for (int index = 0; index < SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH; ++index) {
                    imageInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    if (index < binding.texture.size()) {
                        imageInfos[index].imageView = binding.texture[index]->GetImageView();
                    }
                    else {
                        imageInfos[index].imageView = Engine::GetInstance()->GetFallbackTexture()->GetImageView();
                    }
                    imageInfos[index].sampler = binding.sampler->GetSampler();
                }

                writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptor.dstSet = m_DescriptorSets[index];
                writeDescriptor.dstBinding = bindingIndex;
                writeDescriptor.dstArrayElement = 0;
                writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptor.descriptorCount = SAMPLER_ARRAY_SHADER_DEFINITION_LENGTH;
                writeDescriptor.pImageInfo = imageInfos.data();
                writeDescriptor.pBufferInfo = nullptr;
            }
            bindingIndex++;
        }

        vkUpdateDescriptorSets(Engine::GetInstance()->GetDevice(), writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
    }

    return INIT_SUCCESS;
}

void RenderPipeline::Destroy() {
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
    // for (UniformBinding& binding : m_UniformBindings) {
    //     for (UniformBuffer& ubo : binding.uniformBuffers) { // None are allocated for images so it should be chill i hope
    //         vmaUnmapMemory(allocator, ubo.buffer.GetAllocation()); // looks like it doesnt really matter if it remains mapped but ill still unmap it to be sure
    //         ubo.buffer.DeallocateBuffer();
    //     }
    //     binding.uniformBuffers.clear();
    // }
    m_UniformBindings.clear();

    vkDestroyDescriptorPool(Engine::GetInstance()->GetDevice(), m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(Engine::GetInstance()->GetDevice(), m_DescriptorSetLayout, nullptr);

    vkDestroyPipeline(Engine::GetInstance()->GetDevice(), m_Pipeline, nullptr);
    vkDestroyPipelineLayout(Engine::GetInstance()->GetDevice(), m_PipelineLayout, nullptr);
}

RenderPipeline::~RenderPipeline() {
    Destroy();
}

void RenderPipeline::UpdateUniformBuffer(uint32_t currentFrame, uint32_t descriptionIndex, const void *data) {
    UniformBinding& binding = m_UniformBindings[descriptionIndex];
    memcpy(binding.uniformBuffers[currentFrame].ptr, data, binding.size);
}

VkDescriptorSet& RenderPipeline::GetDescriptorSet(uint32_t imageIndex) {
    return m_DescriptorSets[imageIndex];
}
