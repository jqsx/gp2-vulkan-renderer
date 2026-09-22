//
// Created by frane on 5/13/2026.
//

#include "Shader.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "Engine.h"

std::vector<char> Shader::readFile(const std::string &path) {
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

std::array<VkShaderModule, 2> Shader::createShaderModules(const std::string& frag, const std::string& vert) {
    std::array<VkShaderModule, 2> shaderModules;

    std::vector<char> fragShaderCode = readFile(frag);
    std::vector<char> vertShaderCode = readFile(vert);

    { // Fragment Shader
        VkShaderModuleCreateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        createInfo.codeSize = fragShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

        if (vkCreateShaderModule(Engine::GetInstance()->GetDevice(), &createInfo, nullptr, &shaderModules[0]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create frag shader module.");
        }
    }

    { // Vertex Shader
        VkShaderModuleCreateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        createInfo.codeSize = vertShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

        if (vkCreateShaderModule(Engine::GetInstance()->GetDevice(), &createInfo, nullptr, &shaderModules[1]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vert shader module.");
        }
    }

    return shaderModules;
}

std::array<VkPipelineShaderStageCreateInfo, 2> Shader::createShaderStages() const {
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

    VkPipelineShaderStageCreateInfo fragShaderInfo = {};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderInfo.module = GetFragShader();
    fragShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo vertShaderInfo = {};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderInfo.module = GetVertShader();
    vertShaderInfo.pName = "main";

    shaderStages[0] = fragShaderInfo;
    shaderStages[1] = vertShaderInfo;

    return shaderStages;
}

void Shader::Create(const std::string &fragPath, const std::string &vertPath) {
    Destroy();
    m_ShaderModules = createShaderModules(fragPath, vertPath);
}

void Shader::Destroy() {
    for (VkShaderModule& shaderModule : m_ShaderModules) {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(Engine::GetInstance()->GetDevice(), shaderModule, nullptr);
            shaderModule = VK_NULL_HANDLE;
        }
    }
}

std::array<VkPipelineShaderStageCreateInfo, 2> Shader::GetInfos() const {
    return createShaderStages();
}
