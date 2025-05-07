#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>


class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch{ 0.f };
    // horizontal rotation
    float yaw{ 0.f };

    glm::mat4 getViewMatrix()
    {
        glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
        glm::mat4 cameraRotation = getRotationMatrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }
    glm::mat4 getRotationMatrix()
    {

        glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
        glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });

        return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    }


    void update(float deltaTime = 0.5)
    {
        glm::mat4 cameraRotation = getRotationMatrix();
        position += glm::vec3(cameraRotation * glm::vec4(velocity * deltaTime, 0.f));
    }
};