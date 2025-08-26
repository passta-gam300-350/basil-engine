#pragma once

#include <entt/entt.hpp>
#include "../../Scene/Scene.h"
#include <glm/glm.hpp>

// Pure ECS system - determines which entities should be visible
// Owned by Scene, not SceneRenderer - no rendering, just visibility logic
class VisibilitySystem
{
public:
    VisibilitySystem() = default;
    ~VisibilitySystem() = default;

    // Pure ECS logic - updates VisibilityComponent based on distance, LOD, etc.
    // No camera frustum culling (that's rendering-specific)
    void OnUpdate(Scene* scene, float deltaTime);

private:
    void UpdateDistanceBasedVisibility(entt::registry& registry, const glm::vec3& viewerPosition);
    void UpdateLODVisibility(entt::registry& registry, const glm::vec3& viewerPosition);
};