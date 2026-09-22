//
// Created by frane on 5/13/2026.
//

#ifndef GPVKFR_SHADER_H
#define GPVKFR_SHADER_H
#include <array>

#include "ShaderProgram.h"


class Shader {
    std::array<VkShaderModule, 2> m_ShaderModules{};

    static std::vector<char> readFile(const std::string& path);
    std::array<VkShaderModule, 2> createShaderModules(const std::string& frag, const std::string& vert);
    std::array<VkPipelineShaderStageCreateInfo, 2> createShaderStages() const;
public:
    void Create(const std::string& fragPath, const std::string& vertPath);
    void Destroy();
    VkShaderModule GetFragShader() const { return m_ShaderModules[0]; }
    VkShaderModule GetVertShader() const { return m_ShaderModules[1]; }
    std::array<VkPipelineShaderStageCreateInfo, 2> GetInfos() const;
};


#endif //GPVKFR_SHADER_H