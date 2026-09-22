//
// Created by frane on 5/4/2026.
//

#include "DescriptorSet.h"

#include "Engine.h"
#include "Sampler.h"
#include "ShaderProgram.h"
#include <stdexcept>

void DescriptorSet::DeferredTextureUpdateFrame(uint32_t frame) {
    std::vector<VkWriteDescriptorSet> writeDescriptors{};
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfosList;
    writeDescriptors.reserve(m_UniformBuffers.size());
    imageInfosList.reserve(m_UniformBuffers.size());
    const std::vector<UniformBinding>& bindings = m_ShaderProgram->GetUniformBindings();
    for (int bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
        const UniformBinding& binding = bindings[bindingIndex];
        if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            continue;
        WriteData& data = m_UniformBuffers[bindingIndex];
        VkWriteDescriptorSet& writeDescriptor = writeDescriptors.emplace_back();
        writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.dstSet = m_DescriptorSets[frame];
        writeDescriptor.dstBinding = bindingIndex;
        writeDescriptor.dstArrayElement = 0;

        std::vector<VkDescriptorImageInfo>& imageInfos = imageInfosList.emplace_back();
        imageInfos.resize(binding.count);
        for (int imageIndex = 0; imageIndex < binding.count; ++imageIndex) {
            imageInfos[imageIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (imageIndex < data.texture.size()) {
                imageInfos[imageIndex].imageView = data.texture[imageIndex]->GetImageView();
            }
            else {
                imageInfos[imageIndex].imageView = Engine::GetInstance()->GetFallbackTexture()->GetImageView();
            }
            imageInfos[imageIndex].sampler = binding.sampler->GetSampler();
        }

        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptor.descriptorCount = binding.count;
        writeDescriptor.pImageInfo = imageInfos.data();
        writeDescriptor.pBufferInfo = nullptr;
    }

    vkUpdateDescriptorSets(Engine::GetInstance()->GetDevice(), writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
}

void DescriptorSet::Create(ShaderProgram *program) {
    m_ShaderProgram = program;
    uint32_t maxFramesInFlight = Engine::GetInstance()->GetMaxFramesInFlight();
    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, program->GetDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = program->GetDescriptorPool();
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.descriptorSetCount = maxFramesInFlight;

    m_DescriptorSets.resize(maxFramesInFlight);
    m_DescriptorSetTextureUpdates.resize(maxFramesInFlight, false);
    if (vkAllocateDescriptorSets(Engine::GetInstance()->GetDevice(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to allocate descriptor sets.");
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    // Allocate a buffer per binding inside the material
    // Allows for infrequent writes to the buffer via the material
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
    const std::vector<UniformBinding>& bindings = program->GetUniformBindings();
    m_UniformBuffers.resize(bindings.size());
    for (int index = 0; index < bindings.size(); ++index) {
        const UniformBinding& binding = bindings[index];
        if (binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            continue;
        WriteData& data = m_UniformBuffers[index];
        data.uniformBuffers.resize(maxFramesInFlight);
        VkDeviceSize bufferSize = binding.size;
        for (int index = 0; index < maxFramesInFlight; ++index) {
            data.uniformBuffers[index].buffer.AllocateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            // Cant use Map function because it unmaps the ptr to the buffer
            // and we need this one to remain open
            vmaMapMemory(allocator, data.uniformBuffers[index].buffer.GetAllocation(), &data.uniformBuffers[index].ptr);
        }
    }

    UpdateWriteDescriptors();
}

void DescriptorSet::UpdateWriteDescriptors() {
    uint32_t maxFramesInFlight = Engine::GetInstance()->GetMaxFramesInFlight();
    for (int frame = 0; frame < maxFramesInFlight; ++frame) {
        std::vector<VkWriteDescriptorSet> writeDescriptors{};
        std::vector<std::vector<VkDescriptorImageInfo>> imageInfosList;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        writeDescriptors.reserve(m_UniformBuffers.size());
        imageInfosList.reserve(m_UniformBuffers.size());
        bufferInfos.reserve(m_UniformBuffers.size());
        const std::vector<UniformBinding>& bindings = m_ShaderProgram->GetUniformBindings();
        for (int bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
            const UniformBinding& binding = bindings[bindingIndex];
            WriteData& data = m_UniformBuffers[bindingIndex];
            VkWriteDescriptorSet& writeDescriptor = writeDescriptors.emplace_back();
            writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptor.dstSet = m_DescriptorSets[frame];
            writeDescriptor.dstBinding = bindingIndex;
            writeDescriptor.dstArrayElement = 0;

            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back();
                bufferInfo.buffer = data.uniformBuffers[frame].buffer.GetBuffer();
                bufferInfo.offset = 0;
                bufferInfo.range = binding.size;

                writeDescriptor.descriptorCount = 1;
                writeDescriptor.pBufferInfo = &bufferInfo;
                writeDescriptor.pImageInfo = nullptr; // textures
                writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                std::vector<VkDescriptorImageInfo>& imageInfos = imageInfosList.emplace_back();
                imageInfos.resize(binding.count);
                for (int imageIndex = 0; imageIndex < binding.count; ++imageIndex) {
                    imageInfos[imageIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    if (imageIndex < data.texture.size()) {
                        imageInfos[imageIndex].imageView = data.texture[imageIndex]->GetImageView();
                    }
                    else {
                        imageInfos[imageIndex].imageView = Engine::GetInstance()->GetFallbackTexture()->GetImageView();
                    }
                    imageInfos[imageIndex].sampler = binding.sampler->GetSampler();
                }

                writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptor.descriptorCount = binding.count;
                writeDescriptor.pImageInfo = imageInfos.data();
                writeDescriptor.pBufferInfo = nullptr;
            }
        }

        vkUpdateDescriptorSets(Engine::GetInstance()->GetDevice(), writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
    }
}

void DescriptorSet::DeferredUpdateOnlyTextureViews() {
    uint32_t maxFramesInFlight = Engine::GetInstance()->GetMaxFramesInFlight();
    for (int index = 0; index < maxFramesInFlight; ++index) {
        m_DescriptorSetTextureUpdates[index] = true;
    }
}

void DescriptorSet::WriteToUniform(UniformIdx idx, void *data, bool allFrames) {
    WriteData& writeData = m_UniformBuffers[idx];
    const UniformBinding& binding = m_ShaderProgram->GetUniformBinding(idx);
    if (binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        return;
    if (allFrames) {
        for (UniformBuffer& buffer : writeData.uniformBuffers) {
            memcpy(buffer.ptr, data, binding.size);
        }
    }
    else {
        memcpy(writeData.uniformBuffers[Engine::GetInstance()->GetCurrentFrameIndex()].ptr, data, binding.size);
    }
}

void DescriptorSet::WriteToUniform(UniformIdx idx, const std::vector<Texture2D *> &textures) {
    WriteData& writeData = m_UniformBuffers[idx];
    const UniformBinding& binding = m_ShaderProgram->GetUniformBinding(idx);
    if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        return;

    // Automatically resizes the texture count
    writeData.texture = textures;
}

void DescriptorSet::Destroy() {
    VmaAllocator allocator = Engine::GetInstance()->GetAllocator();
    m_DescriptorSets.clear();
    for (WriteData& ubo : m_UniformBuffers) {
        for (UniformBuffer& buffer : ubo.uniformBuffers) {
            vmaUnmapMemory(allocator, buffer.buffer.GetAllocation());
            buffer.buffer.DeallocateBuffer();
        }
        ubo.uniformBuffers.clear();
        ubo.texture.clear(); // only stores the references to the textures so just clear
    }
    m_UniformBuffers.clear();
}

VkDescriptorSet DescriptorSet::GetDescriptorSet() {
    uint32_t currentFrame = Engine::GetInstance()->GetCurrentFrameIndex();
    if (m_DescriptorSetTextureUpdates[currentFrame]) {
        DeferredTextureUpdateFrame(currentFrame);
        m_DescriptorSetTextureUpdates[currentFrame] = false;
    }
    return m_DescriptorSets[currentFrame];
}
