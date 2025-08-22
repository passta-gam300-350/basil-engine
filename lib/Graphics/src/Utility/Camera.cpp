#include "../../include/Utility/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(CameraType type)
	: m_Type(type)
{
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

	// Update view matrix based on position and rotation
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));

	m_ViewMatrix = glm::inverse(transform);
}