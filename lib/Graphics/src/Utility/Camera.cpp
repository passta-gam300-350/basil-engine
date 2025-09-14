#include "../../include/Utility/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(CameraType type)
	: m_Type(type)
{
	UpdateCameraVectors();
	RecalculateProjection();
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip)
{
	m_Type = CameraType::Perspective;
	m_FOV = fov;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearClip;
	m_FarClip = farClip;
	RecalculateProjection();
}

void Camera::SetOrthographic(float size, float aspectRatio, float nearClip, float farClip)
{
	m_Type = CameraType::Orthographic;
	m_OrthographicSize = size;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearClip;
	m_FarClip = farClip;
	RecalculateProjection();
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = m_MovementSpeed * deltaTime;
	
	switch(direction)
	{
		case CameraMovement::FORWARD:
			m_Position += m_Front * velocity;
			break;
		case CameraMovement::BACKWARD:
			m_Position -= m_Front * velocity;
			break;
		case CameraMovement::LEFT:
			m_Position -= m_Right * velocity;
			break;
		case CameraMovement::RIGHT:
			m_Position += m_Right * velocity;
			break;
		case CameraMovement::UP:
			m_Position += m_WorldUp * velocity;
			break;
		case CameraMovement::DOWN:
			m_Position -= m_WorldUp * velocity;
			break;
	}
	
	m_ViewMatrixDirty = true;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= m_MouseSensitivity;
	yoffset *= m_MouseSensitivity;

	// Update rotation (Yaw = Y rotation, Pitch = X rotation)
	m_Rotation.y += xoffset;  // Yaw
	m_Rotation.x += yoffset;  // Pitch

	// Constrain pitch to prevent screen flipping
	if (constrainPitch)
	{
		m_Rotation.x = std::clamp(m_Rotation.x, -89.0f, 89.0f);
	}

	UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
	m_FOV -= yoffset;
	m_FOV = std::clamp(m_FOV, 1.0f, 45.0f);

	if (m_Type == CameraType::Perspective)
	{
		RecalculateProjection();
	}
}

void Camera::UpdateCameraVectors()
{
	// Calculate the new Front vector from the updated Euler angles
	glm::vec3 front;
	front.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	front.y = sin(glm::radians(m_Rotation.x));
	front.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	m_Front = glm::normalize(front);
	
	// Re-calculate the Right and Up vector
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	
	m_ViewMatrixDirty = true;
}

void Camera::UpdateViewMatrix() const
{
	if (m_ViewMatrixDirty)
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_ViewMatrixDirty = false;
	}
}

void Camera::RecalculateProjection()
{
	if (m_Type == CameraType::Perspective)
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}
	else
	{
		float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
		float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
		float orthoBottom = -m_OrthographicSize * 0.5f;
		float orthoTop = m_OrthographicSize * 0.5f;

		m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_NearClip, m_FarClip);
	}
}