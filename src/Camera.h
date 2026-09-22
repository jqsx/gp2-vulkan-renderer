//
// Created by frane on 4/20/2026.
//

#ifndef GPVKFR_CAMERA_H
#define GPVKFR_CAMERA_H

#include "Singleton.h"
#include "structs.h"

struct CameraInput {
    float cursorX{0};
    float cursorY{0};
    bool keyW{false};
    bool keyS{false};
    bool keyD{false};
    bool keyA{false};
    bool keyC{false};
    bool keySpace{false};

    float moveX{0.0f};
    float moveY{0.0f};
    float moveZ{0.0f};
    float mouseDeltaX{0.0f};
    float mouseDeltaY{0.0f};
    bool leftButton{false};
    bool rightButton{false};

    void Update() {
        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;

        moveX = (keyA ? -1.0f : 0.0f) + (keyD ? 1.0f : 0.0f);
        moveY = (keyC ? -1.0f : 0.0f) + (keySpace ? 1.0f : 0.0f);
        moveZ = (keyS ? -1.0f : 0.0f) + (keyW ? 1.0f : 0.0f);
    }
};

class Camera final : public Singleton<Camera> {
    m4 view;
    m4 proj;

    void pitchYawToVec(float pitch, float yaw, float &x, float &y, float &z);

public:
    float FOV{60.0f};
    float Near{0.1f};
    float Far{100.0f}; // i would have made these lowercase but there is some random macro for near far
    CameraInput input;

    v3 origin;
    v3 forward;
    v3 right;
    v3 up;

    float pitch, yaw;

    Camera();

    void Update(float deltaTime);
    void UpdateUniformBufferObject(UniformBufferObject& data);
    void RecalculateProjectionMatrix();
};


#endif //GPVKFR_CAMERA_H
