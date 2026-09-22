//
// Created by frane on 6/11/2026.
//

#include "Material.h"

#include "ShaderProgram.h"

Material::Material(ShaderProgram* shader) {
    Create(shader);
}

Material::~Material() {
    Destroy();
}

void Material::Create(ShaderProgram* shader) {
    Destroy();
    m_Shader = shader;
    m_DescriptorSet.Create(shader);
}

void Material::Destroy() {
    if (m_Shader == nullptr)
        return;

    m_DescriptorSet.Destroy();
    m_Shader = nullptr;
}

uint32_t Material::GetUniformCount() const {
    return static_cast<uint32_t>(m_Shader->GetUniformBindings().size());
}

void Material::WriteToUniform(UniformIdx idx, void* data, bool allFrames) {
    m_DescriptorSet.WriteToUniform(idx, data, allFrames);
}

void Material::WriteToUniform(UniformIdx idx, const std::vector<Texture2D*>& textures) {
    m_DescriptorSet.WriteToUniform(idx, textures);
    m_DescriptorSet.DeferredUpdateOnlyTextureViews();
}

void Material::SetTextures(Texture2D* baseColor, Texture2D* normal, Texture2D* roughness) {
    m_Textures[MATERIAL_TEXTURE_BASE_COLOR] = baseColor;
    m_Textures[MATERIAL_TEXTURE_NORMAL] = normal;
    m_Textures[MATERIAL_TEXTURE_ROUGHNESS] = roughness;

    std::vector<Texture2D*> textures;
    textures.reserve(m_Textures.size());
    for (Texture2D* texture : m_Textures) {
        textures.push_back(texture);
    }
    WriteToUniform(1, textures);
}
