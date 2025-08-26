#pragma once

#include <entt/entt.hpp>
#include "../../Scene/Scene.h"

// Pure ECS system - handles transform updates, hierarchies, animations
// Owned by Scene, not SceneRenderer - no rendering concerns
class TransformSystem
{
public:
    TransformSystem() = default;
    ~TransformSystem() = default;

    // Pure ECS logic - no OpenGL, no cameras, no rendering
    void OnUpdate(Scene* scene, float deltaTime);

private:
    void UpdateAnimations(entt::registry& registry, float deltaTime);
    void UpdateTransformHierarchies(entt::registry& registry);
};