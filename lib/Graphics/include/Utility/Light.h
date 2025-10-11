#pragma once

#include <glm/glm.hpp>

class Light
{
public:
	enum class Type
	{
		Directional,
		Point,
		Spot
	};

	Light(Type type = Type::Directional);

    void SetColor(const glm::vec3& color) { m_Color = color; }
    void SetIntensity(float intensity) { m_Intensity = intensity; }
    void SetDiffuseIntensity(float intensity) { m_DiffuseIntensity = intensity; }
    void SetAmbientIntensity(float intensity) { m_AmbientIntensity = intensity; }
    void SetDirection(const glm::vec3& direction) { m_Direction = glm::normalize(direction); }
    void SetPosition(const glm::vec3& position) { m_Position = position; }

    // For spot and point lights
    void SetRange(float range) { m_Range = range; }
    void SetAttenuation(float constant, float linear, float quadratic);

    // For spot lights
    void SetCutoff(float innerCutoff, float outerCutoff);

    Type GetType() const { return m_Type; }
    const glm::vec3& GetColor() const { return m_Color; }
    float GetIntensity() const { return m_Intensity; }
    float GetDiffuseIntensity() const { return m_DiffuseIntensity; }
    float GetAmbientIntensity() const { return m_AmbientIntensity; }
    const glm::vec3& GetDirection() const { return m_Direction; }
    const glm::vec3& GetPosition() const { return m_Position; }

    float GetRange() const { return m_Range; }
    const glm::vec3& GetAttenuation() const { return m_Attenuation; }

    float GetInnerCutoff() const { return m_InnerCutoff; }
    float GetOuterCutoff() const { return m_OuterCutoff; }
private:
	Type m_Type;
	glm::vec3 m_Color = { 1.f,1.f,1.f };
	float m_Intensity = 1.f;             // Legacy: for backward compatibility
	float m_DiffuseIntensity = 1.f;      // Ogldev-style: direct light intensity
	float m_AmbientIntensity = 0.0f;     // Ogldev-style: ambient contribution
	glm::vec3 m_Direction = { 0.f,-1.f,0.f };
	glm::vec3 m_Position = { 0.f,0.f,0.f };

	float m_Range = 10.f;
	glm::vec3 m_Attenuation = { 0.1f, 0.01f, 0.001f }; // constant, linear, quadratic (ogldev tutorial 63)

	float m_InnerCutoff = 0.91f; // cos(glm::radians(25.0f))
	float m_OuterCutoff = 0.82f; // cos(glm::radians(35.0f))
};