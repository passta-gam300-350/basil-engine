#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Simple TransformComponent using GLM but with proper EnTT compatibility
struct TransformComponent
{
    glm::vec3 Translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

    // Default constructor
    TransformComponent() = default;
    
    // Constructor with glm::vec3
    TransformComponent(const glm::vec3& translation)
        : Translation(translation)
    {
    }

    // Explicit copy constructor and assignment to help EnTT
    TransformComponent(const TransformComponent& other)
        : Translation(other.Translation), Rotation(other.Rotation), Scale(other.Scale)
    {
    }
    
    TransformComponent& operator=(const TransformComponent& other)
    {
        if (this != &other) {
            Translation = other.Translation;
            Rotation = other.Rotation;
            Scale = other.Scale;
        }
        return *this;
    }

    // Move constructor and assignment
    TransformComponent(TransformComponent&& other) noexcept = default;
    TransformComponent& operator=(TransformComponent&& other) noexcept = default;

    glm::mat4 GetTransform() const
    {
        glm::mat4 rotationMat = glm::toMat4(glm::quat(Rotation));

        return glm::translate(glm::mat4(1.0f), Translation)
            * rotationMat
            * glm::scale(glm::mat4(1.0f), Scale);
    }
};