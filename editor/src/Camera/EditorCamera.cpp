/******************************************************************************/
/*!
\file   EditorCamera.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Editor camera implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Camera/EditorCamera.hpp"
#include "Input/InputManager.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>

EditorCamera::EditorCamera()
{
    UpdateVectors();
    UpdateView();
    UpdateProjection();
}

void EditorCamera::Update(float deltaTime)
{
    // Don't process input if ImGui wants it
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard)
    {
        m_FirstMouse = true;
        return;
    }

    switch (m_Mode)
    {
        case Mode::Fly:
            UpdateFlyMode(deltaTime);
            break;
        case Mode::Orbit:
            UpdateOrbitMode(deltaTime);
            break;
        case Mode::Pan:
            UpdatePanMode(deltaTime);
            break;
    }

    UpdateVectors();
    UpdateView();
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void EditorCamera::UpdateFlyMode(float deltaTime)
{
    auto* input = InputManager::Get_Instance();

    // Get mouse state
    float mouseX, mouseY;
    input->Get_MousePosition(mouseX, mouseY);

    // Handle mouse look (right click + drag)
    if (input->Is_MousePressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        if (!m_IsRightMousePressed)
        {
            m_LastMousePosition = glm::vec2(mouseX, mouseY);
            m_IsRightMousePressed = true;
            m_FirstMouse = false;
        }

        if (!m_FirstMouse)
        {
            float deltaX = mouseX - m_LastMousePosition.x;
            float deltaY = m_LastMousePosition.y - mouseY; // Inverted

            // Apply rotation with smoothing
            m_Rotation.y += deltaX * m_RotationSpeed;
            m_Rotation.x += deltaY * m_RotationSpeed;

            // Clamp pitch
            m_Rotation.x = glm::clamp(m_Rotation.x, -89.0f, 89.0f);
        }

        m_LastMousePosition = glm::vec2(mouseX, mouseY);

        // Movement (WASD + QE for up/down)
        glm::vec3 movement(0.0f);

        if (input->Is_KeyPressed(GLFW_KEY_W))
            movement += m_Front;
        if (input->Is_KeyPressed(GLFW_KEY_S))
            movement -= m_Front;
        if (input->Is_KeyPressed(GLFW_KEY_A))
            movement -= m_Right;
        if (input->Is_KeyPressed(GLFW_KEY_D))
            movement += m_Right;
        if (input->Is_KeyPressed(GLFW_KEY_Q))
            movement.y -= 1.0f;
        if (input->Is_KeyPressed(GLFW_KEY_E))
            movement.y += 1.0f;

        // Speed boost with shift
        float currentSpeed = m_MoveSpeed;
        if (input->Is_KeyPressed(GLFW_KEY_LEFT_SHIFT))
            currentSpeed *= 3.0f;

        // Apply movement with smoothing
        if (glm::length(movement) > 0.0f)
        {
            movement = glm::normalize(movement);
            m_Position += movement * currentSpeed * deltaTime;
        }
    }
    else
    {
        m_IsRightMousePressed = false;
        m_FirstMouse = true;
    }
}

void EditorCamera::UpdateOrbitMode(float)
{
    auto* input = InputManager::Get_Instance();

    float mouseX, mouseY;
    input->Get_MousePosition(mouseX, mouseY);

    // Orbit with middle mouse button
    if (input->Is_MousePressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        if (!m_IsMiddleMousePressed)
        {
            m_LastMousePosition = glm::vec2(mouseX, mouseY);
            m_IsMiddleMousePressed = true;
            m_FirstMouse = false;
        }

        if (!m_FirstMouse)
        {
            float deltaX = mouseX - m_LastMousePosition.x;
            float deltaY = m_LastMousePosition.y - mouseY;

            // Update rotation
            m_Rotation.y += deltaX * m_RotationSpeed;
            m_Rotation.x += deltaY * m_RotationSpeed;
            m_Rotation.x = glm::clamp(m_Rotation.x, -89.0f, 89.0f);

            // Calculate new position orbiting around target
            float camX = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
            float camY = sin(glm::radians(m_Rotation.x));
            float camZ = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));

            m_Position = m_Target + glm::vec3(camX, camY, camZ) * m_OrbitDistance;
        }

        m_LastMousePosition = glm::vec2(mouseX, mouseY);
    }
    else
    {
        m_IsMiddleMousePressed = false;
        m_FirstMouse = true;
    }

    // Pan with shift + middle mouse
    if (input->Is_KeyPressed(GLFW_KEY_LEFT_SHIFT) && input->Is_MousePressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        float deltaX = mouseX - m_LastMousePosition.x;
        float deltaY = m_LastMousePosition.y - mouseY;

        m_Target += m_Right * (-deltaX * m_PanSpeed * m_OrbitDistance);
        m_Target += m_Up * (deltaY * m_PanSpeed * m_OrbitDistance);
        m_Position += m_Right * (-deltaX * m_PanSpeed * m_OrbitDistance);
        m_Position += m_Up * (deltaY * m_PanSpeed * m_OrbitDistance);
    }
}

void EditorCamera::UpdatePanMode(float)
{
    auto* input = InputManager::Get_Instance();

    float mouseX, mouseY;
    input->Get_MousePosition(mouseX, mouseY);

    // Pan with middle mouse
    if (input->Is_MousePressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        if (!m_IsMiddleMousePressed)
        {
            m_LastMousePosition = glm::vec2(mouseX, mouseY);
            m_IsMiddleMousePressed = true;
            m_FirstMouse = false;
        }

        if (!m_FirstMouse)
        {
            float deltaX = mouseX - m_LastMousePosition.x;
            float deltaY = m_LastMousePosition.y - mouseY;

            m_Position += m_Right * (-deltaX * m_PanSpeed * 10.0f);
            m_Position += m_Up * (deltaY * m_PanSpeed * 10.0f);
        }

        m_LastMousePosition = glm::vec2(mouseX, mouseY);
    }
    else
    {
        m_IsMiddleMousePressed = false;
        m_FirstMouse = true;
    }
}

void EditorCamera::OnMouseScroll(float delta)
{
    // Don't process if ImGui wants it
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (m_Mode == Mode::Fly)
    {
        // Adjust move speed in fly mode
        m_MoveSpeed = glm::clamp(m_MoveSpeed + delta * 0.5f, 0.1f, 100.0f);
    }
    else if (m_Mode == Mode::Orbit)
    {
        // Zoom in orbit mode
        m_OrbitDistance = glm::clamp(m_OrbitDistance - delta * m_ZoomSpeed, 1.0f, 100.0f);

        // Update position based on new distance
        float camX = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        float camY = sin(glm::radians(m_Rotation.x));
        float camZ = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));

        m_Position = m_Target + glm::vec3(camX, camY, camZ) * m_OrbitDistance;
    }
}

void EditorCamera::OnMouseMove(float, float)
{
    // This can be used for additional mouse movement handling if needed
}

void EditorCamera::UpdateVectors()
{
    // Calculate forward vector from euler angles
    glm::vec3 forward;
    forward.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
    forward.y = sin(glm::radians(m_Rotation.x));
    forward.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
    m_Front = glm::normalize(forward);

    // Calculate right and up vectors
    m_Right = glm::normalize(glm::cross(m_Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void EditorCamera::UpdateView()
{
    if (m_Mode == Mode::Orbit)
    {
        m_ViewMatrix = glm::lookAt(m_Position, m_Target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else
    {
        m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }
}

void EditorCamera::UpdateProjection()
{
    m_ProjectionMatrix = glm::perspective(
        glm::radians(m_Fov),
        m_AspectRatio,
        m_Near,
        m_Far
    );
}

void EditorCamera::FocusOn(const glm::vec3& target, float distance)
{
    m_Target = target;
    m_OrbitDistance = distance;

    if (m_Mode == Mode::Orbit)
    {
        // Position camera at distance from target
        float camX = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        float camY = sin(glm::radians(m_Rotation.x));
        float camZ = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));

        m_Position = m_Target + glm::vec3(camX, camY, camZ) * m_OrbitDistance;
    }
    else
    {
        // In fly mode, just look at target
        glm::vec3 direction = glm::normalize(target - m_Position);
        m_Rotation.x = glm::degrees(asin(direction.y));
        m_Rotation.y = glm::degrees(atan2(direction.z, direction.x));
    }

    UpdateVectors();
    UpdateView();
}

void EditorCamera::Reset()
{
    m_Position = glm::vec3(0.0f, 5.0f, 10.0f);
    m_Rotation = glm::vec3(-25.0f, -90.0f, 0.0f);
    m_Target = glm::vec3(0.0f, 0.0f, 0.0f);
    m_OrbitDistance = 10.0f;
    m_MoveSpeed = 5.0f;
    m_FirstMouse = true;

    UpdateVectors();
    UpdateView();
    UpdateProjection();
}