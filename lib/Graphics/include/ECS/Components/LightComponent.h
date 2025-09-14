#pragma once

#include <glm/glm.hpp>

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct LightComponent
{
    LightType Type = LightType::Directional;
    glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;

    // Point light properties
    float Range = 10.0f;
    float Attenuation = 1.0f;

    // Spot light properties
    float InnerCutOff = glm::cos(glm::radians(12.5f));
    float OuterCutOff = glm::cos(glm::radians(17.5f));

    LightComponent() = default;
    LightComponent(LightType type, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f)
        : Type(type), Color(color), Intensity(intensity)
    {
    }
};