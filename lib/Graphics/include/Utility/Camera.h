#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType
{
	Perspective,
	Orthographic
};

// Camera movement directions
enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:
	Camera(CameraType type = CameraType::Perspective);
	
	void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip);
	void SetOrthographic(float size, float aspectRatio, float nearClip, float farClip);

	// Matrix getters
	const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { UpdateViewMatrix(); return m_ViewMatrix; }
	glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * GetViewMatrix(); }

	// Position and rotation
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetRotation() const { return m_Rotation; }
	glm::vec3 GetFront() const { return m_Front; }
	glm::vec3 GetRight() const { return m_Right; }
	glm::vec3 GetUp() const { return m_Up; }

	void SetPosition(const glm::vec3& position) { m_Position = position; UpdateCameraVectors(); }
	void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; UpdateCameraVectors(); }

	// Camera controls
	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
	void ProcessMouseScroll(float yoffset);

	// Camera properties
	float GetFOV() const { return m_FOV; }
	float GetNearClip() const { return m_NearClip; }
	float GetFarClip() const { return m_FarClip; }
	CameraType GetType() const { return m_Type; }

	// Settings
	float MovementSpeed = 2.5f;
	float MouseSensitivity = 0.1f;
	float Zoom = 45.0f;

private:
	void RecalculateProjection();
	void UpdateCameraVectors();
	void UpdateViewMatrix() const;

	CameraType m_Type;

	glm::mat4 m_ProjectionMatrix = glm::mat4(1.f);
	mutable glm::mat4 m_ViewMatrix = glm::mat4(1.f);
	mutable bool m_ViewMatrixDirty = true;

	glm::vec3 m_Position = { 0.f, 0.f, 3.f };
	glm::vec3 m_Rotation = { 0.f, 0.f, 0.f }; // Pitch, Yaw, Roll

	// Camera vectors
	glm::vec3 m_Front = { 0.f, 0.f, -1.f };
	glm::vec3 m_Up = { 0.f, 1.f, 0.f };
	glm::vec3 m_Right = { 1.f, 0.f, 0.f };
	glm::vec3 m_WorldUp = { 0.f, 1.f, 0.f };

	// Perspective
	float m_FOV = 45.f;
	float m_AspectRatio = 1.778f;
	float m_NearClip = 0.1f;
	float m_FarClip = 1000.f;

	// Orthographic
	float m_OrthographicSize = 10.f;
};