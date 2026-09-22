//
// Created by frane on 4/20/2026.
//

#include "ResourceManager.h"

#include <utility>

#include "AllocatedImage.h"
#include "Engine.h"
#include "Material.h"
#include "Mesh.h"
#include "Sampler.h"
#include "structs.h"
#include "Texture2D.h"
#include "TextureCube.h"

ResourceManager::ResourceManager() {

}

ResourceManager::~ResourceManager() {
    DestroyMaterials();
    m_MeshPool.clear();
    m_TextureCubePool.clear();
    m_TexturePool.clear();
}

Texture2D * ResourceManager::CreateTexture2D(stbi_uc *pixels, int width, int height, bool enableMip, VkFormat format) {
    AllocatedImage image;
    if (enableMip)
        image.AllowMipMaps(width, height);
    image.AllocateImage(width, height, format);
    image.Map(pixels, enableMip, 4);
    image.AllocateView(VK_IMAGE_ASPECT_COLOR_BIT);

    std::unique_ptr<Texture2D>& texture = m_TexturePool.emplace_back(std::make_unique<Texture2D>(std::move(image)));
    return texture.get();
}

Texture2D* ResourceManager::CreateTexture2D(float* pixels, int width, int height, VkFormat format, bool enableMip) {
    AllocatedImage image;
    if (enableMip)
        image.AllowMipMaps(width, height);
    image.AllocateImage(width, height, format);

    const uint32_t imageSize = static_cast<uint32_t>(width * height * 4 * sizeof(float));
    image.MapBytes(pixels, imageSize, enableMip);
    image.AllocateView(VK_IMAGE_ASPECT_COLOR_BIT);

    std::unique_ptr<Texture2D>& texture = m_TexturePool.emplace_back(std::make_unique<Texture2D>(std::move(image)));
    return texture.get();
}

TextureCube* ResourceManager::CreateTextureCube(float* pixels, uint32_t size, VkFormat format, bool enableMip) {
    AllocatedImage image;
    if (enableMip)
        image.AllowMipMaps(size, size);
    image.AllocateImage(
        size,
        size,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    );

    const uint32_t imageSize = static_cast<uint32_t>(size * size * 6 * 4 * sizeof(float));
    image.MapBytes(pixels, imageSize, enableMip);
    image.AllocateView(VK_IMAGE_ASPECT_COLOR_BIT);

    std::unique_ptr<TextureCube>& texture = m_TextureCubePool.emplace_back(std::make_unique<TextureCube>(std::move(image)));
    return texture.get();
}

TextureCube* ResourceManager::CreateRenderableTextureCube(uint32_t size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
    AllocatedImage image;
    image.AllocateImage(
        size,
        size,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        usage | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    );
    image.AllocateView(aspectFlags);

    std::unique_ptr<TextureCube>& texture = m_TextureCubePool.emplace_back(std::make_unique<TextureCube>(std::move(image), aspectFlags));
    return texture.get();
}

void ResourceManager::DestroyTexture2D(Texture2D *texture) {
    // TODO
    // Only dynamic disposal of textures
    if (texture == nullptr)
        return;


}

Sampler* ResourceManager::CreateSampler() {
    std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>();
    Sampler* samplerPtr = sampler.get();
    m_SamplerPool.emplace_back(std::move(sampler));
    return samplerPtr;
}

Mesh* ResourceManager::CreateMesh(const std::vector<uint32_t> &indices, const std::vector<Vertex> &vertices) {
    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(indices, vertices);
    Mesh* result = mesh.get();
    m_MeshPool.emplace_back(std::move(mesh));
    return result; // Pointer doesn't escape the pointer gets moved
}

Material* ResourceManager::CreateMaterial(ShaderProgram* shader) {
    std::unique_ptr<Material> material = std::make_unique<Material>(shader);
    Material* result = material.get();
    m_MaterialPool.emplace_back(std::move(material));
    return result;
}

void ResourceManager::DestroyMaterials() {
    m_MaterialPool.clear();
}
