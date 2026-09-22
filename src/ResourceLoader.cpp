//
// Created by frane on 4/20/2026.
//

#include "ResourceLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <utility>
#include <vector>

#include "Engine.h"
#include "Logger.h"
#include <stb_image.h>
#include "Texture2D.h"
#include "TextureCube.h"
#include <tiny_gltf_v3.h>
#include <tinyexr.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/common.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/geometric.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

glm::vec3 ResourceLoader::CubemapDirection(uint32_t face, float u, float v) {
    switch (face) {
        case 0: return glm::normalize(glm::vec3{ 1.0f, -v, -u}); // +X
        case 1: return glm::normalize(glm::vec3{-1.0f, -v,  u}); // -X
        case 2: return glm::normalize(glm::vec3{ u,  1.0f,  v}); // +Y
        case 3: return glm::normalize(glm::vec3{ u, -1.0f, -v}); // -Y
        case 4: return glm::normalize(glm::vec3{ u, -v,  1.0f}); // +Z
        default: return glm::normalize(glm::vec3{-u, -v, -1.0f}); // -Z
    }
}

unsigned char * ResourceLoader::GetPixels4(const std::string &path, int& width, int& height) {
    int comp;
    return (unsigned char*)stbi_load(path.c_str(), &width, &height, &comp, STBI_rgb_alpha);
}

void ResourceLoader::CheckErrors() {
    for (int index = 0; index < m_ErrorStack.count(); ++index) {
        const tg3_error_entry* entry = m_ErrorStack.entry(index);

        Logger::GetInstance().err(std::string(entry->message));
    }
}

const tg3_str_int_pair* ResourceLoader::FindAttribute(const tg3_primitive &primitive, const std::string &name) {
    for (int index = 0; index < primitive.attributes_count; ++index) {
        const tg3_str_int_pair &attribute = primitive.attributes[index];
        if (attribute.key.data && name == attribute.key.data) {
            return &attribute;
        }
    }
    return nullptr;
}

std::string ResourceLoader::JoinTexturePath(const std::string &gltfPath, const char *uri) {
    if (!uri || uri[0] == '\0') {
        return {};
    }
    std::filesystem::path base = std::filesystem::path(gltfPath).parent_path();
    std::filesystem::path tex = std::filesystem::path(uri);
    return (base / tex).lexically_normal().string();
}

const uint8_t * ResourceLoader::GetAccessorData(const tg3_model* model, int accessorIndex) {
    const tg3_accessor& accessor = model->accessors[accessorIndex];
    const tg3_buffer_view& view  = model->buffer_views[accessor.buffer_view];
    const tg3_buffer& buffer = model->buffers[view.buffer];

    return buffer.data.data + view.byte_offset + accessor.byte_offset;
}

uint32_t ResourceLoader::GetAccessorStride(const tg3_model* model, int accessorIndex, size_t packedElementSize) {
    const tg3_accessor& accessor = model->accessors[accessorIndex];
    const tg3_buffer_view& view  = model->buffer_views[accessor.buffer_view];

    if (view.byte_stride > 0) {
        return view.byte_stride;
    }

    return packedElementSize;
}

uint32_t ResourceLoader::ReadIndex(const uint8_t *data, int componentType, size_t i) {
    switch (componentType) {
        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
            return reinterpret_cast<const uint8_t*>(data)[i];
        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
            return reinterpret_cast<const uint16_t*>(data)[i];
        case TG3_COMPONENT_TYPE_UNSIGNED_INT:
            return reinterpret_cast<const uint32_t*>(data)[i];
        default:
            throw std::runtime_error("Unsupported index component type");
    }
}

std::string ResourceLoader::GetTexturePath(const tinygltf3::Model &model, int textureIndex, const std::string &gltfPath) {
    if (textureIndex < 0 || textureIndex >= model->textures_count) {
        return "";
    }

    const tg3_texture& texture = model->textures[textureIndex];

    if (texture.source < 0 || texture.source >= model->images_count) {
        return "";
    }

    const tg3_image& image = model->images[texture.source];

    if (image.uri.data == nullptr || std::strlen(image.uri.data) == 0) {
        return "";
    }

    return std::string(image.uri.data, image.uri.len);
}

uint32_t ResourceLoader::FindTextureIndex(const std::string &texturePath, GLTFLoadData &load_data) {
    for (int index = 0; index < load_data.texturePaths.size(); ++index) {
        if (load_data.texturePaths[index] == texturePath) {
            return index;
        }
    }
    return -1;
}

void ResourceLoader::LoadMaterials(const tinygltf3::Model &model, const std::string &path, GLTFLoadData &loadData) {
    loadData.materials.reserve(model->materials_count);

    for (int materialIndex = 0; materialIndex < model->materials_count; ++materialIndex) {
        const tg3_material& material = model->materials[materialIndex];
        LoadedMaterialData materialData{};
        materialData.baseColorTexturePath = GetTexturePath(model, material.pbr_metallic_roughness.base_color_texture.index, path);
        materialData.normalTexturePath = GetTexturePath(model, material.normal_texture.index, path);
        materialData.roughnessTexturePath = GetTexturePath(model, material.pbr_metallic_roughness.metallic_roughness_texture.index, path);
        materialData.roughnessFactor = static_cast<float>(material.pbr_metallic_roughness.roughness_factor);

        std::string* paths[] = {
            &materialData.baseColorTexturePath,
            &materialData.normalTexturePath,
            &materialData.roughnessTexturePath
        };

        for (std::string* texturePath : paths) {
            if (!texturePath->empty() && std::ranges::find(loadData.texturePaths, *texturePath) == loadData.texturePaths.end()) {
                loadData.texturePaths.push_back(*texturePath);
            }
        }

        materialData.baseColorTextureIndex = materialData.baseColorTexturePath.empty() ? static_cast<uint32_t>(-1) : FindTextureIndex(materialData.baseColorTexturePath, loadData);
        materialData.normalTextureIndex = materialData.normalTexturePath.empty() ? static_cast<uint32_t>(-1) : FindTextureIndex(materialData.normalTexturePath, loadData);
        materialData.roughnessTextureIndex = materialData.roughnessTexturePath.empty() ? static_cast<uint32_t>(-1) : FindTextureIndex(materialData.roughnessTexturePath, loadData);
        loadData.materials.emplace_back(std::move(materialData));
    }
}


void ResourceLoader::ReadNodeTransform(const tg3_node& node, v3& position, glm::quat& rotation, v3& scale) {
    position = { 0.0f, 0.0f, 0.0f };
    rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    scale = { 1.0f, 1.0f, 1.0f };

    position = {
        float(node.translation[0]),
        float(node.translation[1]),
        float(node.translation[2])
    };

    rotation = glm::quat(
        float(node.rotation[3]), // why is w first for quat stupid glm
        float(node.rotation[0]),
        float(node.rotation[1]),
        float(node.rotation[2])
    );

    scale = {
        float(node.scale[0]),
        float(node.scale[1]),
        float(node.scale[2])
    };
}

ResourceLoader::ResourceLoader(VkDevice device) : m_Device(device) {

}

ResourceLoader::~ResourceLoader() {
    m_TexturePool.clear();
}

Texture2D* ResourceLoader::LoadImage2D(const std::string& path, VkFormat format) {
    int width, height;
    unsigned char* pixels = GetPixels4(path, width, height);

    if (pixels == nullptr) {
        Logger::GetInstance().err("Failed to load image at " + path);
        return nullptr;
    }

    Texture2D* texture = Engine::GetInstance()->GetResourceManager().CreateTexture2D(pixels, width, height, true, format);

    stbi_image_free(pixels);

    return texture;
}

Texture2D* ResourceLoader::LoadEXRImage2D(const std::string& path) {
    int width = 0;
    int height = 0;
    float* pixels = nullptr;
    const char* error = nullptr;

    const int result = LoadEXR(&pixels, &width, &height, path.c_str(), &error);
    if (result != TINYEXR_SUCCESS) {
        std::string message = "Failed to load EXR at " + path;
        if (error != nullptr) {
            message += ": ";
            message += error;
            FreeEXRErrorMessage(error);
        }
        Logger::GetInstance().err(message);
        return nullptr;
    }

    Texture2D* texture = Engine::GetInstance()->GetResourceManager().CreateTexture2D(
        pixels,
        width,
        height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        false
    );

    std::free(pixels);
    return texture;
}

TextureCube* ResourceLoader::LoadEXRCubemap(const std::string& path, uint32_t faceSize) {
    int width = 0;
    int height = 0;
    float* pixels = nullptr;
    const char* error = nullptr;

    const int result = LoadEXR(&pixels, &width, &height, path.c_str(), &error);
    if (result != TINYEXR_SUCCESS) {
        std::string message = "Failed to load EXR cubemap source at " + path;
        if (error != nullptr) {
            message += ": ";
            message += error;
            FreeEXRErrorMessage(error);
        }
        Logger::GetInstance().err(message);
        return nullptr;
    }

    std::vector<float> cubemapPixels;
    cubemapPixels.resize(static_cast<size_t>(faceSize) * faceSize * 6 * 4);

    for (uint32_t face = 0; face < 6; ++face) {
        for (uint32_t y = 0; y < faceSize; ++y) {
            for (uint32_t x = 0; x < faceSize; ++x) {
                const float u = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;
                const float v = (2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;
                const glm::vec3 direction = CubemapDirection(face, u, v);
                const float horizontal = std::atan2(direction.z, direction.x) / (2.0f * glm::pi<float>()) + 0.5f;
                const float vertical = 0.5f - std::asin(std::clamp(direction.y, -1.0f, 1.0f)) / glm::pi<float>();
                const int sourceX = std::clamp(static_cast<int>(horizontal * static_cast<float>(width)), 0, width - 1);
                const int sourceY = std::clamp(static_cast<int>(vertical * static_cast<float>(height)), 0, height - 1);
                const int sourceOffset = (sourceY * width + sourceX) * 4;

                const size_t offset = (static_cast<size_t>(face) * faceSize * faceSize + y * faceSize + x) * 4;
                cubemapPixels[offset + 0] = pixels[sourceOffset + 0];
                cubemapPixels[offset + 1] = pixels[sourceOffset + 1];
                cubemapPixels[offset + 2] = pixels[sourceOffset + 2];
                cubemapPixels[offset + 3] = pixels[sourceOffset + 3];
            }
        }
    }

    TextureCube* texture = Engine::GetInstance()->GetResourceManager().CreateTextureCube(
        cubemapPixels.data(),
        faceSize,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        true
    );

    std::free(pixels);
    return texture;
}

void ResourceLoader::DestroyImage(Texture2D *texture) {
    std::ranges::remove_if(m_TexturePool.begin(), m_TexturePool.end(), [texture](const std::unique_ptr<Texture2D>& element) { return element.get() == texture; });
    // doesn't matter rn its just some function if im dynamically loading and unloading textures
    // they all get deallocated when the resource loader instance is deleted in engine
}

void ResourceLoader::LoadNodeMeshes(const tinygltf3::Model& model, int nodeIndex, const std::string& path, GLTFLoadData& loadData) {
    if (nodeIndex < 0 || nodeIndex >= model->nodes_count) {
        return;
    }

    const tg3_node& node = model->nodes[nodeIndex];

    v3 nodePosition;
    glm::quat nodeRotation;
    v3 nodeScale;
    ReadNodeTransform(node, nodePosition, nodeRotation, nodeScale);

    if (node.mesh >= 0 && node.mesh < model->meshes_count) {
        const tg3_mesh& mesh = model->meshes[node.mesh];

        for (int primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex) {
            const tg3_primitive& primitive = mesh.primitives[primitiveIndex];

            if (primitive.mode != TG3_MODE_TRIANGLES) {
                continue;
            }

            const tg3_str_int_pair* positionAttr = FindAttribute(primitive, "POSITION");
            const tg3_str_int_pair* normalAttr = FindAttribute(primitive, "NORMAL");
            const tg3_str_int_pair* texCoordAttr = FindAttribute(primitive, "TEXCOORD_0");
            const tg3_str_int_pair* tangentAttr = FindAttribute(primitive, "TANGENT");

            if (positionAttr == nullptr) {
                Logger::GetInstance().err("Primitive missing POSITION in " + path);
                continue;
            }

            const int positionAccessorIndex = positionAttr->value;
            const tg3_accessor& positionAccessor = model->accessors[positionAccessorIndex];

            if (positionAccessor.component_type != TG3_COMPONENT_TYPE_FLOAT || positionAccessor.type != TG3_TYPE_VEC3) {
                Logger::GetInstance().err("position attribute isnt made up of 3 floats thats bit weird " + path);
                continue;
            }

            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            const uint8_t* positionData = GetAccessorData(model.get(), positionAccessorIndex);
            const size_t positionStride = GetAccessorStride(model.get(), positionAccessorIndex, sizeof(float) * 3);

            const uint8_t* texCoordData = nullptr;
            const uint8_t* normalData = nullptr;
            const uint8_t* tangentData = nullptr;
            size_t texCoordStride = 0;
            size_t normalStride = 0;
            size_t tangentStride = 0;

            if (normalAttr != nullptr) {
                const int normalAccessorIndex = normalAttr->value;
                const tg3_accessor& normalAccessor = model->accessors[normalAccessorIndex];

                if (normalAccessor.component_type == TG3_COMPONENT_TYPE_FLOAT && normalAccessor.type == TG3_TYPE_VEC3) {
                    normalData = GetAccessorData(model.get(), normalAccessorIndex);
                    normalStride = GetAccessorStride(model.get(), normalAccessorIndex, sizeof(float) * 3);
                }
            }

            if (texCoordAttr != nullptr) {
                const int texCoordAccessorIndex = texCoordAttr->value;
                const tg3_accessor& texCoordAccessor = model->accessors[texCoordAccessorIndex];

                if (texCoordAccessor.component_type == TG3_COMPONENT_TYPE_FLOAT && texCoordAccessor.type == TG3_TYPE_VEC2) {
                    texCoordData = GetAccessorData(model.get(), texCoordAccessorIndex);
                    texCoordStride = GetAccessorStride(model.get(), texCoordAccessorIndex, sizeof(float) * 2);
                }
            }

            if (tangentAttr != nullptr) {
                const int tangentAccessorIndex = tangentAttr->value;
                const tg3_accessor& tangentAccessor = model->accessors[tangentAccessorIndex];

                if (tangentAccessor.component_type == TG3_COMPONENT_TYPE_FLOAT && tangentAccessor.type == TG3_TYPE_VEC4) {
                    tangentData = GetAccessorData(model.get(), tangentAccessorIndex);
                    tangentStride = GetAccessorStride(model.get(), tangentAccessorIndex, sizeof(float) * 4);
                }
            }

            vertices.reserve(positionAccessor.count);

            for (int vertexIndex = 0; vertexIndex < positionAccessor.count; ++vertexIndex) {
                Vertex vertex{};

                const float* pos = reinterpret_cast<const float*>(positionData + (positionStride * vertexIndex));
                vertex.position = { -pos[0], pos[1], pos[2] };
                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (normalData != nullptr) {
                    const float* normal = reinterpret_cast<const float*>(normalData + (normalStride * vertexIndex));
                    vertex.normal = glm::normalize(v3{ normal[0], normal[1], normal[2] });
                }
                else {
                    vertex.normal = { 0.0f, 1.0f, 0.0f };
                }

                if (texCoordData != nullptr) {
                    const float* uv = reinterpret_cast<const float*>(texCoordData + (texCoordStride * vertexIndex));
                    vertex.texCoord = { uv[0], uv[1] };
                }
                else {
                    vertex.texCoord = { 0.0f, 0.0f };
                }

                if (tangentData != nullptr) {
                    const float* tangent = reinterpret_cast<const float*>(tangentData + (tangentStride * vertexIndex));
                    vertex.tangent = glm::vec4(glm::normalize(v3{ tangent[0], tangent[1], tangent[2] }), tangent[3]);
                }
                else {
                    vertex.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                }

                vertices.push_back(vertex);
            }

            if (primitive.indices >= 0) {
                const tg3_accessor& indexAccessor = model->accessors[primitive.indices];

                if (indexAccessor.type != TG3_TYPE_SCALAR) {
                    Logger::GetInstance().err("Index accessor is not scalar in " + path);
                    continue;
                }

                const uint8_t* indexData = GetAccessorData(model.get(), primitive.indices);
                indices.reserve(indexAccessor.count);

                for (int index = 0; index < indexAccessor.count; ++index) {
                    indices.push_back(ReadIndex(indexData, indexAccessor.component_type, index));
                }
            }

            else {
                indices.reserve(vertices.size());

                for (uint32_t index = 0; index < vertices.size(); ++index) {
                    indices.push_back(index);
                }
            }

            if (normalData == nullptr) {
                for (uint32_t index = 0; index + 2 < indices.size(); index += 3) {
                    Vertex& a = vertices[indices[index]];
                    Vertex& b = vertices[indices[index + 1]];
                    Vertex& c = vertices[indices[index + 2]];
                    v3 faceNormal = glm::cross(b.position - a.position, c.position - a.position);
                    if (glm::length(faceNormal) <= 0.0001f) {
                        faceNormal = {0.0f, 1.0f, 0.0f};
                    }
                    else {
                        faceNormal = glm::normalize(faceNormal);
                    }
                    a.normal = faceNormal;
                    b.normal = faceNormal;
                    c.normal = faceNormal;
                }
            }

            if (tangentData == nullptr && texCoordData != nullptr) {
                for (Vertex& vertex : vertices) {
                    vertex.tangent = glm::vec4(0.0f);
                }

                for (uint32_t index = 0; index + 2 < indices.size(); index += 3) {
                    Vertex& a = vertices[indices[index]];
                    Vertex& b = vertices[indices[index + 1]];
                    Vertex& c = vertices[indices[index + 2]];

                    const v3 edge1 = b.position - a.position;
                    const v3 edge2 = c.position - a.position;
                    const v2 deltaUV1 = b.texCoord - a.texCoord;
                    const v2 deltaUV2 = c.texCoord - a.texCoord;
                    const float determinant = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;

                    if (glm::abs(determinant) <= 0.000001f)
                        continue;

                    const float invDeterminant = 1.0f / determinant;
                    const v3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * invDeterminant;

                    a.tangent += glm::vec4(tangent, 0.0f);
                    b.tangent += glm::vec4(tangent, 0.0f);
                    c.tangent += glm::vec4(tangent, 0.0f);
                }

                for (Vertex& vertex : vertices) {
                    v3 tangent = v3(vertex.tangent);
                    if (glm::length(tangent) <= 0.0001f) {
                        tangent = glm::abs(vertex.normal.y) < 0.999f
                            ? glm::normalize(glm::cross(v3{0.0f, 1.0f, 0.0f}, vertex.normal))
                            : glm::normalize(glm::cross(v3{1.0f, 0.0f, 0.0f}, vertex.normal));
                    }
                    else {
                        tangent = glm::normalize(tangent - vertex.normal * glm::dot(vertex.normal, tangent));
                    }

                    vertex.tangent = glm::vec4(tangent, 1.0f);
                }
            }

            uint32_t materialIndex = static_cast<uint32_t>(-1);
            uint32_t textureIndex = static_cast<uint32_t>(-1);
            if (primitive.material >= 0 && primitive.material < loadData.materials.size()) {
                materialIndex = primitive.material;
                textureIndex = loadData.materials[materialIndex].baseColorTextureIndex;
            }

            MeshData meshData{};
            meshData.vertices = vertices;
            meshData.indices = indices;
            meshData.textureIndex = textureIndex;
            meshData.materialIndex = materialIndex;
            meshData.position = nodePosition;
            meshData.rotation = nodeRotation;
            meshData.scale = nodeScale;

            loadData.meshes.emplace_back(meshData);

            Logger::GetInstance().log(
                "Loaded node mesh from " + path +
                " node=" + std::to_string(nodeIndex) +
                " vertices=" + std::to_string(vertices.size()) +
                " indices=" + std::to_string(indices.size())
            );
        }
    }

    for (int childIndex = 0; childIndex < node.children_count; ++childIndex) {
        LoadNodeMeshes(model, node.children[childIndex], path, loadData);
    }
}

GLTFLoadData ResourceLoader::LoadGLTF(const std::string &path) {
    GLTFLoadData loadData{};
    tinygltf3::Model model{};
    tg3_error_code code = tinygltf3::parse_file(model, m_ErrorStack, path.c_str());

    CheckErrors();

    if (code != TG3_OK) {
        Logger::GetInstance().err(std::string("(" + std::to_string(code) + ") Failed to parse model " + path));
        return {};
    }

    if (model->scenes_count == 0) {
        Logger::GetInstance().err("No scenes in " + path);
        return {};
    }

    int sceneIndex = model->default_scene;

    if (sceneIndex < 0 || sceneIndex >= model->scenes_count) {
        sceneIndex = 0;
    }

    const tg3_scene& scene = model->scenes[sceneIndex];

    LoadMaterials(model, path, loadData);

    for (int nodeIndex = 0; nodeIndex < scene.nodes_count; ++nodeIndex) {
        LoadNodeMeshes(model, scene.nodes[nodeIndex], path, loadData);
    }

    return loadData;
}
