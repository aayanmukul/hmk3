#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

enum class CameraDir { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 worldUp { 0.f, 1.f, 0.f };

    float yaw    { -90.f };
    float pitch  {   0.f };
    float speed     { 5.f };
    float mouseSens { 0.08f };
    float fov       { 45.f };

    glm::vec3 front { 0.f, 0.f, -1.f };
    glm::vec3 right { 1.f, 0.f,  0.f };
    glm::vec3 up    { 0.f, 1.f,  0.f };

    Camera(glm::vec3 pos = { 0.f, 5.f, 15.f })
        : position(pos)
    {
        updateVectors();
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(CameraDir dir, float dt)
    {
        float dist = speed * dt;
        switch (dir) {
            case CameraDir::FORWARD:  position += front   * dist; break;
            case CameraDir::BACKWARD: position -= front   * dist; break;
            case CameraDir::LEFT:     position -= right   * dist; break;
            case CameraDir::RIGHT:    position += right   * dist; break;
            case CameraDir::UP:       position += worldUp * dist; break;
            case CameraDir::DOWN:     position -= worldUp * dist; break;
        }
    }

    void processMouseMovement(float dx, float dy, bool constrainPitch = true)
    {
        yaw   += dx * mouseSens;
        pitch += dy * mouseSens;

        if (constrainPitch)
            pitch = std::clamp(pitch, -89.f, 89.f);

        updateVectors();
    }

    void processScroll(float yOffset)
    {
        fov -= yOffset;
        fov = std::clamp(fov, 1.f, 90.f);
    }

    void reset()
    {
        position = { 0.f, 5.f, 15.f };
        yaw   = -90.f;
        pitch = 0.f;
        fov   = 45.f;
        updateVectors();
    }

private:
    void updateVectors()
    {
        float yawR   = glm::radians(yaw);
        float pitchR = glm::radians(pitch);

        front.x = cos(yawR) * cos(pitchR);
        front.y = sin(pitchR);
        front.z = sin(yawR) * cos(pitchR);
        front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};
