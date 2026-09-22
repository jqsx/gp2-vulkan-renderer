//
// Created by frane on 5/4/2026.
//

#ifndef GPVKFR_DESCRIPTORSET_H
#define GPVKFR_DESCRIPTORSET_H
#include <vector>
#include <vulkan/vulkan_core.h>

#include "structs.h"


class ShaderProgram;

struct WriteData {
    std::vector<UniformBuffer> uniformBuffers;
    std::vector<Texture2D*> texture;
};

class DescriptorSet {
    std::vector<WriteData> m_UniformBuffers{};
    std::vector<VkDescriptorSet> m_DescriptorSets{};
    std::vector<bool> m_DescriptorSetTextureUpdates{};
    ShaderProgram* m_ShaderProgram{nullptr};

    void DeferredTextureUpdateFrame(uint32_t frame);

public:
    void Create(ShaderProgram* program);
    // Unsafe, no point in updating buffers, instead just write to them
    // Safe when creating a material in the first place, unsafe when material was already used.
    void UpdateWriteDescriptors();
    // Safe when updating textures, deferring descriptor write update
    void DeferredUpdateOnlyTextureViews();
    void WriteToUniform(UniformIdx idx, void* data, bool allFrames = false);
    void WriteToUniform(UniformIdx idx, const std::vector<Texture2D*>& textures);
    void Destroy();

    VkDescriptorSet GetDescriptorSet();
};

#endif //GPVKFR_DESCRIPTORSET_H