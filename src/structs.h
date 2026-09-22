//
// Created by frane on 2/13/2026.
//
#pragma once

#ifndef STRUCTS_H
#define STRUCTS_H

#include <array>
#include <float.h>
#include <optional>
#include <limits>
#include <vulkan/vulkan.h>
#include <vector>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include "AllocatedBuffer.h"
#include "Logger.h"

class ShaderProgram;
class Sampler;
class Texture2D;
using UniformIdx = int32_t;
typedef glm::vec3 v3;
typedef glm::vec2 v2;
typedef glm::mat4 m4;

enum InitResult {
    INIT_NINIT,
    INIT_SUCCESS,
    INIT_FAIL
};

struct UniformBuffer {
    AllocatedBuffer buffer;
    void* ptr{nullptr};
};

struct UniformBinding {
    VkDescriptorType descriptorType;
    uint32_t size;
    Sampler* sampler;
    uint32_t count{1}; // Array length for uniform binding
};

struct EngineConfig {
    int width;
    int height;
    const char* title;
    unsigned int swapChainAdditionalImageCount;
};

struct QueueFamilyIndices {
    std::optional<unsigned int> graphicsFamily;
    std::optional<unsigned int> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct VertexInfo {
    uint32_t size;
    VkVertexInputBindingDescription binding;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct UniformBufferObject {
    m4 model;
    m4 view;
    m4 proj;
    glm::vec4 lightDirection;
};

struct AABB {
    v3 min{FLT_MAX};
    v3 max{-FLT_MAX};
    bool valid{false};

    void Expand(const v3& point) {
        if (!valid) {
            min = point;
            max = point;
            valid = true;
            return;
        }

        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void Expand(const AABB& other) {
        if (!other.valid)
            return;
        Expand(other.min);
        Expand(other.max);
    }

    v3 Center() const {
        return (min + max) * 0.5f;
    }

    static std::array<v3, 8> GetCorners(const AABB& aabb) {
        return {
            v3{aabb.min.x, aabb.min.y, aabb.min.z},
            v3{aabb.max.x, aabb.min.y, aabb.min.z},
            v3{aabb.min.x, aabb.max.y, aabb.min.z},
            v3{aabb.max.x, aabb.max.y, aabb.min.z},
            v3{aabb.min.x, aabb.min.y, aabb.max.z},
            v3{aabb.max.x, aabb.min.y, aabb.max.z},
            v3{aabb.min.x, aabb.max.y, aabb.max.z},
            v3{aabb.max.x, aabb.max.y, aabb.max.z}
        };
    }
};

struct DeferredLightingData {
    m4 lightView;
    m4 lightProjection;
    m4 lightViewProjection;
    m4 inverseView;
    m4 inverseProjection;
    glm::vec4 lightDirection;
    glm::vec4 cameraPosition;
};

struct ShadowMatrices {
    m4 model;
    m4 lightViewProjection;
};

struct PointLight {
    v3 position{0.0f};
    v3 color{1.0f};
    float intensity{1.0f};
    float radius{1.0f};
};

struct PointLightShadowData {
    glm::vec4 positionRadius{0.0f};
    glm::vec4 colorIntensity{0.0f};
};

struct PointShadowMatrices {
    m4 model;
    m4 lightViewProjection;
    glm::vec4 lightPositionRadius;
};

struct PushData {
    uint32_t imageIndex;
    float roughness;
};

// Make some kind of interface struct that provides access to binding description for the render pipeline
struct Vertex {
    v3 position;
    v3 color;
    v3 normal;
    v2 texCoord;
    glm::vec4 tangent;

    Vertex() : position(), color(), normal(), texCoord(), tangent(1.0f, 0.0f, 0.0f, 1.0f) {}
    Vertex(v3 p, v3 c, v3 n, v2 tc, glm::vec4 t = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) : position(p), color(c), normal(n), texCoord(tc), tangent(t) {}

    static void AddBinding(std::vector<VkVertexInputAttributeDescription>& attrib, uint32_t offset, VkFormat format = VK_FORMAT_R32G32B32_SFLOAT, uint32_t binding = 0) {
        VkVertexInputAttributeDescription desc{};

        desc.location = uint32_t(attrib.size());
        desc.binding = binding;
        desc.format = format;
        desc.offset = offset;

        attrib.emplace_back(desc);
    }

    VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        // Good enough honestly

        AddBinding(attributeDescriptions, offsetof(Vertex, position));
        AddBinding(attributeDescriptions, offsetof(Vertex, color));
        AddBinding(attributeDescriptions, offsetof(Vertex, normal));
        AddBinding(attributeDescriptions, offsetof(Vertex, texCoord), VK_FORMAT_R32G32_SFLOAT);
        AddBinding(attributeDescriptions, offsetof(Vertex, tangent), VK_FORMAT_R32G32B32A32_SFLOAT);

        return attributeDescriptions;
    }

    VertexInfo GetVertexInfo() {
        return {sizeof(Vertex), GetBindingDescription(), GetVertexAttributeDescriptions()};
    }
};

#endif //STRUCTS_H
