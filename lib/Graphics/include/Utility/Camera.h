#pragma once

#include <glm/glm.hpp>

enum class CameraType
{
	Perspective,
	Orthographic
};

class Camera
{
public:
	Camera(CameraType type = CameraType::Perspective);
	
	void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip);
	void SetOrthographic(float size, float aspectRatio, float nearClip, float farClip);

	void SetViewMatrix(const glm::mat4& view) {}

private:
	void RecalculateProjection();

	CameraType m_Type;

	glm::mat4 m_ProjectionMatrix = glm::mat4(1.f);
	glm::mat4 m_ViewMatrix = glm::mat4(1.f);

	glm::vec3 m_Position = { 0.f, 0.f, 0.f };
	glm::vec3 m_Rotation = { 0.f, 0.f, 0.f };

	// Perspective
	float m_FOV = 45.f;
	float m_AspectRatio = 1.778f;
	float m_NearClip = 0.1f;
	float m_FarClip = 1000.f;

	// Orthographic
	float m_OrthographicSize = 10.f;
};