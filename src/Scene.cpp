//
// Created by frane on 4/19/2026.
//

#include "Scene.h"
#include "GameObject.h"
#include "ShaderProgram.h"

#include <algorithm>
#include <array>
#include <limits>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

AABB Scene::CalculateWorldAABB(const GameObject& object) const {
    AABB result{};
    if (object.mesh == nullptr || !object.mesh->GetAABB().valid)
        return result;

    m4 modelMatrix = glm::identity<m4>();
    modelMatrix = glm::translate(modelMatrix, object.position);
    modelMatrix *= glm::mat4_cast(object.rotation);
    modelMatrix = glm::scale(modelMatrix, object.scale);

    for (const v3& corner : AABB::GetCorners(object.mesh->GetAABB())) {
        result.Expand(v3{modelMatrix * glm::vec4(corner, 1.0f)});
    }

    return result;
}

void Scene::CalculateDirectionalLightMatrices() {
    if (!m_AABB.valid)
        return;

    const v3 sceneCenter = m_AABB.Center();
    const v3 lightDirection = glm::normalize(m_LightDirection);
    const std::array<v3, 8> corners = AABB::GetCorners(m_AABB);

    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    for (const v3& corner : corners) {
        const float projection = glm::dot(corner, lightDirection);
        minProj = std::min(minProj, projection);
        maxProj = std::max(maxProj, projection);
    }

    const float distance = maxProj - glm::dot(sceneCenter, lightDirection);
    const v3 lightPosition = sceneCenter - lightDirection * distance;
    const v3 up = glm::abs(glm::dot(lightDirection, v3{0.0f, 1.0f, 0.0f})) > 0.99f
        ? v3{0.0f, 0.0f, 1.0f}
        : v3{0.0f, -1.0f, 0.0f};

    m_LightingData.lightView = glm::lookAt(lightPosition, sceneCenter, up);

    v3 minLightSpace{std::numeric_limits<float>::max()};
    v3 maxLightSpace{std::numeric_limits<float>::lowest()};
    for (const v3& corner : corners) {
        const v3 transformedCorner = v3{m_LightingData.lightView * glm::vec4(corner, 1.0f)};
        minLightSpace = glm::min(minLightSpace, transformedCorner);
        maxLightSpace = glm::max(maxLightSpace, transformedCorner);
    }

    const float nearZ = std::max(0.0f, -maxLightSpace.z);
    const float farZ = std::max(nearZ + 0.1f, -minLightSpace.z);
    m_LightingData.lightProjection = glm::orthoRH_ZO(
        minLightSpace.x,
        maxLightSpace.x,
        minLightSpace.y,
        maxLightSpace.y,
        nearZ,
        farZ
    );
    m_LightingData.lightViewProjection = m_LightingData.lightProjection * m_LightingData.lightView;
    m_LightingData.lightDirection = glm::vec4(lightDirection, 0.0f);
}

void Scene::ForfeitGameObjectToScene(ShaderProgram *pipeline, std::unique_ptr<GameObject> pGameObject) {
    if (!m_WorldObjects.contains(pipeline)) {
        m_WorldObjects.emplace(pipeline, std::vector<std::unique_ptr<GameObject>>());
    }

    m_AABB.Expand(CalculateWorldAABB(*pGameObject));
    CalculateDirectionalLightMatrices();

    std::vector<std::unique_ptr<GameObject>>& worldObjects = m_WorldObjects[pipeline];
    worldObjects.emplace_back(std::move(pGameObject));
}

void Scene::AddShaderProgram(ShaderProgram *pipeline) {
    m_ShaderPrograms.push_back(pipeline);
    m_WorldObjects.emplace(pipeline, std::vector<std::unique_ptr<GameObject>>());
}

const std::vector<std::unique_ptr<GameObject>> & Scene::GetGameObjects(ShaderProgram *pipeline) {
    if (m_WorldObjects.contains(pipeline)) {
        return m_WorldObjects[pipeline];
    }

    m_WorldObjects.emplace(pipeline, std::vector<std::unique_ptr<GameObject>>());
    return m_WorldObjects[pipeline];
}

void Scene::SetLightDirection(const v3& direction) {
    if (glm::length(direction) <= 0.0001f)
        return;

    m_LightDirection = glm::normalize(direction);
    CalculateDirectionalLightMatrices();
}

void Scene::AddPointLight(const PointLight& light) {
    m_PointLights.push_back(light);
}

void Scene::ClearPointLights() {
    m_PointLights.clear();
}
