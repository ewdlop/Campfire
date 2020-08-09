#include "Scene/CameraController.h"
#include "Core/Input.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <imgui.h>

void CameraController::SetActiveCamera(const SharedPtr<Camera>& camera, const glm::vec3& pos, const glm::vec3& euler)
{
    activeCamera = camera;

    // Orient controller to match camera's rotation
    position = pos;
    pitch = euler.x;
    yaw -= euler.y;
}

void CameraController::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(CameraController::OnMouseScrolled));
    dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(CameraController::OnMouseMoved));
}

void CameraController::OnUpdate(float dt)
{
    movementSpeed = (Input::GetKey(KEY_LEFT_SHIFT)) ? maxSpeed : normalSpeed;

    if (Input::GetKey(KEY_W))
        ProcessKeyboard(FORWARD, dt);
    if (Input::GetKey(KEY_A))
        ProcessKeyboard(LEFT, dt);
    if (Input::GetKey(KEY_S))
        ProcessKeyboard(BACKWARD, dt);
    if (Input::GetKey(KEY_D))
        ProcessKeyboard(RIGHT, dt);

    UpdateCameraVectors();
    activeCamera->RecalculateViewMatrix(position, front, up);
}

bool CameraController::OnMouseScrolled(MouseScrolledEvent& e)
{
    if (fov >= 1.0f && fov <= 60.0f)
        fov -= e.GetYOffset();
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 60.0f)
        fov = 60.0f;

    activeCamera->vFov = fov;
    activeCamera->SetProjection();

    return false;
}

bool CameraController::OnWindowResized(WindowResizeEvent& e)
{
    activeCamera->width = e.GetWidth();
    activeCamera->height = e.GetHeight();
    activeCamera->SetProjection();

    return false;
}


bool CameraController::OnMouseMoved(MouseMovedEvent& e)
{
    static bool constrainPitch = true;
    static bool firstMouse = true;
    static float lastX, lastY;

    if (firstMouse)
    {
        lastX = e.GetX();
        lastY = e.GetY();
        firstMouse = false;
    }

    float xOffset = e.GetX() - lastX;
    float yOffset = e.GetY() - lastY;

    lastX = e.GetX();
    lastY = e.GetY();

    if (Input::GetMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        yaw += xOffset;
        pitch -= yOffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        UpdateCameraVectors();
        activeCamera->RecalculateViewMatrix(position, front, up);
    }
    return false;
}

void CameraController::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD)
        position += front * velocity;
    if (direction == BACKWARD)
        position -= front * velocity;
    if (direction == LEFT)
        position -= right * velocity;
    if (direction == RIGHT)
        position += right * velocity;

    // locks the camera at ground level
    //Position.y = 0.0f;
}

void CameraController::UpdateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 offset;
    offset.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    offset.y = sin(glm::radians(pitch));
    offset.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(offset);
    // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}