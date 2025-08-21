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

	void SetViewMatrix(const glm::mat4& view) { m_ViewMatrix = view; }
	void SetPosition(const glm::vec3& position) { m_Position = position; }
	void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }

	const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
	glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetRotation() const { return m_Rotation; }

	float GetNearClip() const { return m_NearClip; }
	float GetFarClip() const { return m_FarClip; }

	CameraType GetType() const { return m_Type; }

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