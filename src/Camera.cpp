//
// Created by frane on 4/20/2026.
//

#include "Camera.h"

#include "Engine.h"
#include "Window.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

Camera::Camera() {
    view = glm::identity<m4>();
    RecalculateProjectionMatrix();
}

void Camera::pitchYawToVec(float pitch, float yaw, float &x, float &y, float &z) {
	x = cosf(pitch) * sinf(yaw);
	y = sinf(pitch);
	z = cosf(pitch) * cosf(yaw);
}

void Camera::Update(float deltaTime) {

	const float mouseSensitivity = 0.1f;
	const float mouseMoveSensitivity = 0.01f;
	const float moveSpeed = 5.0f;

	Engine::GetInstance()->GetWindow()->SetMouseLock(input.leftButton || input.rightButton);

	const v3 worldUp{ 0.0f, 1.0f, 0.0f };

	pitchYawToVec(pitch, yaw, forward.x, forward.y, forward.z);
	forward = glm::normalize(forward);

	right = glm::normalize(glm::cross(forward, worldUp));
	up = glm::normalize(glm::cross(right, forward));

	if (input.leftButton && input.rightButton) {
		const float inputY = float(input.mouseDeltaY);

		origin += worldUp * inputY * mouseMoveSensitivity;
	}
	else if (input.leftButton) {
		const float inputY = float(input.mouseDeltaY);

		yaw += glm::radians(input.mouseDeltaX) * mouseSensitivity;

		pitchYawToVec(pitch, yaw, forward.x, forward.y, forward.z);
		forward = glm::normalize(forward);

		right = glm::normalize(glm::cross(forward, worldUp));
		up = glm::normalize(glm::cross(right, forward));

		origin += forward * (inputY) * deltaTime * moveSpeed;
	}
	else if (input.rightButton) {
		yaw += glm::radians(input.mouseDeltaX) * mouseSensitivity;
		pitch += glm::radians(input.mouseDeltaY) * mouseSensitivity;

		const float maxPitch = glm::radians(89.0f);
		pitch = glm::clamp(pitch, -maxPitch, maxPitch);

		origin += (-forward * input.moveZ + worldUp * input.moveY + right * input.moveX) * deltaTime * moveSpeed;
	}

	const float twoPi = glm::two_pi<float>();
	if (yaw > twoPi || yaw < -twoPi) {
		yaw = fmodf(yaw, twoPi);
	}

	view = glm::lookAt(origin, origin + forward, up);

	input.Update();
}

void Camera::UpdateUniformBufferObject(UniformBufferObject &data) {
    data.proj = proj;
    data.view = view;
}

void Camera::RecalculateProjectionMatrix() {
    Window* window = Engine::GetInstance()->GetWindow();
    float aspect = 1.0f;
    if (window->GetWidth() != 0 && window->GetHeight() != 0) {
        aspect = float(window->GetWidth()) / float(window->GetHeight());
    }
    proj = glm::perspectiveLH(glm::radians(FOV), aspect, Near, Far);
    proj[1][1] *= -1.0f;
}
