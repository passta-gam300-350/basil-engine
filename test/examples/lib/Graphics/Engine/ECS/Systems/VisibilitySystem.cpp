#include "VisibilitySystem.h"
#include "../Components/TransformComponent.h"
#include "../Components/VisibilityComponent.h"

void VisibilitySystem::OnUpdate(Scene* scene, float deltaTime)
{
    if (!scene)
        return;

    auto& registry = scene->GetRegistry();
    
    // For now, assume viewer is at origin (engine doesn't know about cameras)
    // In a real system, this might come from a ViewerComponent or similar
    glm::vec3 viewerPosition(0.0f, 0.0f, 0.0f);
    
    // Update visibility based on distance
    UpdateDistanceBasedVisibility(registry, viewerPosition);
    
    // Update visibility based on level-of-detail
    UpdateLODVisibility(registry, viewerPosition);
    
    // This is PURE ECS logic - no cameras, no frustum culling, no OpenGL
}

void VisibilitySystem::UpdateDistanceBasedVisibility(entt::registry& registry, const glm::vec3& viewerPosition)
{
    // Hide objects beyond a certain distance
    const float maxVisibilityDistance = 100.0f;
    
    auto view = registry.view<TransformComponent, VisibilityComponent>();
    view.each([&](auto entity, auto& transform, auto& visibility) {
        float distance = glm::distance(transform.Translation, viewerPosition);
        
        // Simple distance-based visibility
        visibility.IsVisible = (distance <= maxVisibilityDistance);
    });
}

void VisibilitySystem::UpdateLODVisibility(entt::registry& registry, const glm::vec3& viewerPosition)
{
    // Example LOD system - could disable small objects at distance
    // This is game logic, not rendering logic
    
    auto view = registry.view<TransformComponent, VisibilityComponent>();
    view.each([&](auto entity, auto& transform, auto& visibility) {
        float distance = glm::distance(transform.Translation, viewerPosition);
        
        // Example: Objects with small bounding spheres become invisible at distance
        if (visibility.BoundingSphereRadius < 0.5f && distance > 50.0f) {
            visibility.IsVisible = false;
        }
    });
}