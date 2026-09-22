//
// Created by frane on 4/20/2026.
//

#ifndef GPVKFR_RESOURCELOADER_H
#define GPVKFR_RESOURCELOADER_H
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "structs.h"
#include "tiny_gltf_v3.h"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"

class Mesh;
class Texture2D;
class TextureCube;

struct LoadedMaterialData {
    uint32_t baseColorTextureIndex{static_cast<uint32_t>(-1)};
    uint32_t normalTextureIndex{static_cast<uint32_t>(-1)};
    uint32_t roughnessTextureIndex{static_cast<uint32_t>(-1)};
    std::string baseColorTexturePath;
    std::string normalTexturePath;
    std::string roughnessTexturePath;
    float roughnessFactor{1.0f};
};

struct MeshData {
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    uint32_t textureIndex;
    uint32_t materialIndex;

    v3 position;
    glm::quat rotation;
    v3 scale;
};

struct GLTFLoadData {
    std::vector<MeshData> meshes;
    std::vector<std::string> texturePaths;
    std::vector<LoadedMaterialData> materials;
};

class ResourceLoader {
    VkDevice m_Device;

    std::vector<std::unique_ptr<Texture2D>> m_TexturePool;

    tinygltf3::ErrorStack m_ErrorStack;

    unsigned char* GetPixels4(const std::string& path, int& width, int& height);

    void CheckErrors();

    const tg3_str_int_pair* FindAttribute(const tg3_primitive& primitive, const std::string& name);

    static std::string JoinTexturePath(const std::string& gltfPath, const char* uri);
    static const uint8_t* GetAccessorData(const tg3_model* model, int accessorIndex);
    static uint32_t GetAccessorStride(const tg3_model* model, int accessorIndex, size_t packedElementSize);
    static uint32_t ReadIndex(const uint8_t* data, int componentType, size_t i);
    static std::string GetTexturePath(const tinygltf3::Model& model, int textureIndex, const std::string& gltfPath);
    static uint32_t FindTextureIndex(const std::string& texturePath, GLTFLoadData& load_data);
    static glm::vec3 CubemapDirection(uint32_t face, float u, float v);
    void LoadMaterials(const tinygltf3::Model& model, const std::string& path, GLTFLoadData& loadData);
    void ReadNodeTransform(const tg3_node& node, v3& position, glm::quat& rotation, v3& scale);
    void LoadNodeMeshes(const tinygltf3::Model& model, int nodeIndex, const std::string& path, GLTFLoadData& loadData);

public:
    ResourceLoader(VkDevice device);
    ~ResourceLoader();

    Texture2D* LoadImage2D(const std::string& path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    Texture2D* LoadEXRImage2D(const std::string& path);
    TextureCube* LoadEXRCubemap(const std::string& path, uint32_t faceSize = 512);
    void DestroyImage(Texture2D* texture);

    GLTFLoadData LoadGLTF(const std::string& path);
};


#endif //GPVKFR_RESOURCELOADER_H
