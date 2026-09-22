//
// Created by frane on 2/13/2026.
//

#define STB_IMAGE_IMPLEMENTATION

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS          // enable file I/O
#include "tiny_gltf_v3.h"

#define VMA_IMPLEMENTATION
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <vk_mem_alloc.h>

#include "structs.h"

#include "Engine.h"
#include "Material.h"
#include "ResourceLoader.h"
#include "Sampler.h"
#include "Shader.h"
#include "ShaderProgram.h"

static void Start() {
    Engine* engine = new Engine({ 300, 300, "GP2 VK Franciszek Rakowiecki", 1});

    engine->GetScene()->SetLightDirection({ -0.1f, -1.0f, -0.15f });

    Sampler* sampler = engine->GetResourceManager().CreateSampler();
    sampler->Linear().Create();

    {
        PointLight light;
        light.color = { 1.0f, 1.0f, 0.0f };
        light.position = { 10.0f, 2.0f, 0.0f };
        light.intensity = 1.0f;
        light.radius = 10.0f;
        engine->GetScene()->AddPointLight(light);
    }

    {
        PointLight light;
        light.color = { 0.0f, 1.0f, 1.0f };
        light.position = { -7.0f, 2.0f, 0.0f };
        light.intensity = 1.0f;
        light.radius = 10.0f;
        engine->GetScene()->AddPointLight(light);
    }

    GLTFLoadData loadData = engine->GetResourceLoader().LoadGLTF("resources/models/Sponza.gltf");

    std::unordered_set<uint32_t> linearTextureIndices;
    for (const LoadedMaterialData& materialData : loadData.materials) {
        if (materialData.normalTextureIndex != static_cast<uint32_t>(-1)) {
            linearTextureIndices.insert(materialData.normalTextureIndex);
        }
        if (materialData.roughnessTextureIndex != static_cast<uint32_t>(-1)) {
            linearTextureIndices.insert(materialData.roughnessTextureIndex);
        }
    }

    std::vector<Texture2D*> textures;
    textures.resize(loadData.texturePaths.size());
    for (int index = 0; index < textures.size(); ++index) {
        Logger::GetInstance().log("resources/images/" + loadData.texturePaths[index]);
        const VkFormat format = linearTextureIndices.contains(index) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB;
        textures[index] = engine->GetResourceLoader().LoadImage2D("resources/images/" + loadData.texturePaths[index], format);
    }
    auto getTexture = [&](uint32_t textureIndex) {
        if (textureIndex == static_cast<uint32_t>(-1) || textureIndex >= textures.size() || textures[textureIndex] == nullptr) {
            return engine->GetFallbackTexture();
        }
        return textures[textureIndex];
    };

    stbi_uc defaultNormalPixels[4] = {128, 128, 255, 255};
    stbi_uc defaultRoughnessPixels[4] = {0, 0, 0, 255};
    Texture2D* defaultNormalTexture = engine->GetResourceManager().CreateTexture2D(defaultNormalPixels, 1, 1, false, VK_FORMAT_R8G8B8A8_UNORM);
    Texture2D* defaultRoughnessTexture = engine->GetResourceManager().CreateTexture2D(defaultRoughnessPixels, 1, 1, false, VK_FORMAT_R8G8B8A8_UNORM);

    auto getNormalTexture = [&](uint32_t textureIndex) {
        if (textureIndex == static_cast<uint32_t>(-1) || textureIndex >= textures.size() || textures[textureIndex] == nullptr) {
            return defaultNormalTexture;
        }
        return textures[textureIndex];
    };

    auto getRoughnessTexture = [&](uint32_t textureIndex) {
        if (textureIndex == static_cast<uint32_t>(-1) || textureIndex >= textures.size() || textures[textureIndex] == nullptr) {
            return defaultRoughnessTexture;
        }
        return textures[textureIndex];
    };

    Shader shader;
    shader.Create("resources/shaders/flat.frag.spv", "resources/shaders/flat.vert.spv");

    ShaderProgram opaque;
    opaque.SetShader(&shader);
    opaque.SetVertexInfo(Vertex().GetVertexInfo());
    opaque.SetPushConstantRange(sizeof(PushData));
    opaque.SetGBufferFormats(&engine->GetRenderer().GetGBuffer());
    opaque.SetMaxMaterialCount(std::max<uint32_t>(1, static_cast<uint32_t>(loadData.materials.size()) + 1));
    opaque.CreateUniform(sizeof(UniformBufferObject));
    opaque.CreateSamplerUniform(sampler, MATERIAL_TEXTURE_COUNT);
    opaque.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    opaque.Create();

    std::vector<Material*> materials;
    materials.reserve(loadData.materials.size());
    for (const LoadedMaterialData& materialData : loadData.materials) {
        Material* material = engine->GetResourceManager().CreateMaterial(&opaque);
        material->SetTextures(
            getTexture(materialData.baseColorTextureIndex),
            getNormalTexture(materialData.normalTextureIndex),
            getRoughnessTexture(materialData.roughnessTextureIndex)
        );
        material->SetRoughness(materialData.roughnessFactor);
        materials.push_back(material);
    }

    Material* fallbackMaterial = engine->GetResourceManager().CreateMaterial(&opaque);
    fallbackMaterial->SetTextures(engine->GetFallbackTexture(), defaultNormalTexture, defaultRoughnessTexture);

    engine->GetScene()->AddShaderProgram(&opaque);

    for (int index = 0; index < loadData.meshes.size(); ++index) {
        MeshData& data = loadData.meshes[index];
        Mesh* mesh = engine->GetResourceManager().CreateMesh(data.indices, data.vertices);

        std::unique_ptr<GameObject> object = std::make_unique<GameObject>();
        object->mesh = mesh;
        object->material = data.materialIndex == static_cast<uint32_t>(-1) || data.materialIndex >= materials.size() ? fallbackMaterial : materials[data.materialIndex];
        object->push.imageIndex = MATERIAL_TEXTURE_BASE_COLOR;
        object->push.roughness = object->material->GetRoughness();
        object->position = data.position;
        object->rotation = data.rotation;
        object->scale = data.scale;

        engine->GetScene()->ForfeitGameObjectToScene(&opaque, std::move(object));
    }

    engine->Begin();

    opaque.Destroy();
    shader.Destroy();
    delete engine;
}

int main() {
    Start();
}
