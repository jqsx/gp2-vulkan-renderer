//
// Created by frane on 4/19/2026.
//

#ifndef GPVKFR_GAMEOBJECT_H
#define GPVKFR_GAMEOBJECT_H
#include "Material.h"
#include "Mesh.h"
#include "glm/gtc/quaternion.hpp"


class GameObject {
public:
    Mesh* mesh;
    Material* material;
    PushData push;
    v3 position;
    v3 scale;
    glm::quat rotation;
};


#endif //GPVKFR_GAMEOBJECT_H
