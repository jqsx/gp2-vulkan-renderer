//
// Created by frane on 6/11/2026.
//

#ifndef GPVKFR_MATERIAL_H
#define GPVKFR_MATERIAL_H

#include <array>
#include <vector>

#include "DescriptorSet.h"

class ShaderProgram;
class Texture2D;

enum MaterialTextureSlot {
    MATERIAL_TEXTURE_BASE_COLOR = 0,
    MATERIAL_TEXTURE_NORMAL = 1,
    MATERIAL_TEXTURE_ROUGHNESS = 2,
    MATERIAL_TEXTURE_COUNT = 3
};

class Material {
    DescriptorSet m_DescriptorSet;
    ShaderProgram* m_Shader{nullptr};
    std::array<Texture2D*, MATERIAL_TEXTURE_COUNT> m_Textures{};
    float m_Roughness{1.0f};

public:
    Material() = default;
    explicit Material(ShaderProgram* shader);
    ~Material();

    void Create(ShaderProgram* shader);
    void Destroy();

    uint32_t GetUniformCount() const;
    void WriteToUniform(UniformIdx idx, void* data, bool allFrames = false);
    void WriteToUniform(UniformIdx idx, const std::vector<Texture2D*>& textures);
    void SetTextures(Texture2D* baseColor, Texture2D* normal, Texture2D* roughness);
    void SetRoughness(float roughness) { m_Roughness = roughness; }

    ShaderProgram* GetShader() const { return m_Shader; }
    VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet.GetDescriptorSet(); }
    float GetRoughness() const { return m_Roughness; }
};

#endif //GPVKFR_MATERIAL_H
