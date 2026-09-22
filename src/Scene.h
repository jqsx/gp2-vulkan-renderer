//
// Created by frane on 4/19/2026.
//

#ifndef GPVKFR_SCENE_H
#define GPVKFR_SCENE_H
#include <map>
#include <memory>
#include <vector>
#include <glm/geometric.hpp>
#include "GameObject.h"
#include "structs.h"

class GameObject;
class ShaderProgram;

class Scene {
    std::map<ShaderProgram*, std::vector<std::unique_ptr<GameObject>>> m_WorldObjects;
    std::vector<ShaderProgram*> m_ShaderPrograms;
    AABB m_AABB;
    DeferredLightingData m_LightingData{};
    v3 m_LightDirection{glm::normalize(v3{-0.4f, -1.0f, -0.3f})};
    std::vector<PointLight> m_PointLights;

    AABB CalculateWorldAABB(const GameObject& object) const;
    void CalculateDirectionalLightMatrices();

public:
    void ForfeitGameObjectToScene(ShaderProgram* pipeline, std::unique_ptr<GameObject> pGameObject);
    void AddShaderProgram(ShaderProgram* pipeline);

    const AABB& GetAABB() const { return m_AABB; }
    const DeferredLightingData& GetLightingData() const { return m_LightingData; }
    const v3& GetLightDirection() const { return m_LightDirection; }
    void SetLightDirection(const v3& direction);
    void AddPointLight(const PointLight& light);
    void ClearPointLights();
    const std::vector<PointLight>& GetPointLights() const { return m_PointLights; }

    const std::vector<ShaderProgram*>& GetShaders() { return m_ShaderPrograms; }
    const std::vector<std::unique_ptr<GameObject>>& GetGameObjects(ShaderProgram* pipeline);
};

#endif //GPVKFR_SCENE_H
