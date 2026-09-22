//
// Created by frane on 4/20/2026.
//

#ifndef GPVKFR_RESOURCEMANAGER_H
#define GPVKFR_RESOURCEMANAGER_H
#include <memory>
#include <stb_image.h>
#include <vector>

#include "structs.h"
#include "Texture2D.h"
#include "Mesh.h"

class Material;
class Sampler;
class ShaderProgram;
class Texture2D;
class TextureCube;
// Refactor engine class and instead store all allocated objects here
// use the resource loader only to load things and then generate everything here
class ResourceManager {
    std::vector<std::unique_ptr<Texture2D>> m_TexturePool;
    std::vector<std::unique_ptr<TextureCube>> m_TextureCubePool;
    std::vector<std::unique_ptr<Mesh>> m_MeshPool;
    std::vector<std::unique_ptr<Material>> m_MaterialPool;
    std::vector<std::unique_ptr<Sampler>> m_SamplerPool;

    // std::vector<VkImageView>& m_SwapChainImageViews;

public:
    ResourceManager();
    ~ResourceManager();
    Texture2D* CreateTexture2D(stbi_uc* pixels, int width, int height, bool enableMip = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    Texture2D* CreateTexture2D(float* pixels, int width, int height, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT, bool enableMip = false);
    TextureCube* CreateTextureCube(float* pixels, uint32_t size, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT, bool enableMip = false);
    void DestroyTexture2D(Texture2D* texture);
    TextureCube* CreateRenderableTextureCube(uint32_t size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);

    Mesh* CreateMesh(const std::vector<uint32_t> &indices, const std::vector<Vertex> &vertices);
    Material* CreateMaterial(ShaderProgram* shader);
    void DestroyMaterials();
    Sampler* CreateSampler();
};


#endif //GPVKFR_RESOURCEMANAGER_H
