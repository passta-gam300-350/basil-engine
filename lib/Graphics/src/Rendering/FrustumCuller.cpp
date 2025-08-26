#include "Rendering/FrustumCuller.h"
#include "../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/ComponentInterfaces.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <entt/entt.hpp>

void FrustumCuller::CullAgainstCamera(Scene* scene, Camera& camera)
{
    if (!scene)
        return;

    auto* componentAccessor = ComponentUtils::GetComponentAccessor();
    if (!componentAccessor) {
        std::cout << "Warning: No component accessor set for FrustumCuller" << std::endl;
        return;
    }

    // Extract frustum planes from camera
    Frustum frustum;
    frustum.ExtractPlanes(camera.GetViewProjectionMatrix());

    auto& registry = scene->GetRegistry();
    
    // Update visibility based on camera frustum for entities with mesh components
    auto view = registry.view<MeshComponent>();
    
    view.each([&](auto entity, auto& mesh) {
        // Check if entity has required components using interface
        if (!componentAccessor->HasTransform(registry, entity) || 
            !componentAccessor->HasVisibility(registry, entity)) {
            return;
        }
        
        // Get components via interface
        auto* transform = componentAccessor->GetTransform(registry, entity);
        auto* visibility = componentAccessor->GetVisibility(registry, entity);
        
        if (!transform || !visibility) return;
        
        // Only cull if the entity was previously visible (don't override other visibility systems)
        if (!visibility->IsVisible()) {
            return;
        }
        
        // Check if entity is in camera frustum - use transform position
        glm::vec3 position = transform->GetPosition();
        bool inFrustum = IsEntityInFrustum(position, 1.0f, frustum); // Default radius for now
        
        // If culled by frustum, mark as invisible
        if (!inFrustum) {
            visibility->SetVisible(false);
        }
    });
}

void FrustumCuller::Frustum::ExtractPlanes(const glm::mat4& viewProj)
{
    // Left plane
    planes[Left].x = viewProj[0][3] + viewProj[0][0];
    planes[Left].y = viewProj[1][3] + viewProj[1][0];
    planes[Left].z = viewProj[2][3] + viewProj[2][0];
    planes[Left].w = viewProj[3][3] + viewProj[3][0];

    // Right plane
    planes[Right].x = viewProj[0][3] - viewProj[0][0];
    planes[Right].y = viewProj[1][3] - viewProj[1][0];
    planes[Right].z = viewProj[2][3] - viewProj[2][0];
    planes[Right].w = viewProj[3][3] - viewProj[3][0];

    // Bottom plane
    planes[Bottom].x = viewProj[0][3] + viewProj[0][1];
    planes[Bottom].y = viewProj[1][3] + viewProj[1][1];
    planes[Bottom].z = viewProj[2][3] + viewProj[2][1];
    planes[Bottom].w = viewProj[3][3] + viewProj[3][1];

    // Top plane
    planes[Top].x = viewProj[0][3] - viewProj[0][1];
    planes[Top].y = viewProj[1][3] - viewProj[1][1];
    planes[Top].z = viewProj[2][3] - viewProj[2][1];
    planes[Top].w = viewProj[3][3] - viewProj[3][1];

    // Near plane
    planes[Near].x = viewProj[0][3] + viewProj[0][2];
    planes[Near].y = viewProj[1][3] + viewProj[1][2];
    planes[Near].z = viewProj[2][3] + viewProj[2][2];
    planes[Near].w = viewProj[3][3] + viewProj[3][2];

    // Far plane
    planes[Far].x = viewProj[0][3] - viewProj[0][2];
    planes[Far].y = viewProj[1][3] - viewProj[1][2];
    planes[Far].z = viewProj[2][3] - viewProj[2][2];
    planes[Far].w = viewProj[3][3] - viewProj[3][2];

    // Normalize all planes
    for (int i = 0; i < Planes::Count; ++i) {
        float length = glm::sqrt(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
        planes[i] /= length;
    }
}

bool FrustumCuller::Frustum::IsSphereVisible(const glm::vec3& center, float radius) const
{
    for (int i = 0; i < Planes::Count; ++i) {
        if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius) {
            return false;
        }
    }
    return true;
}

bool FrustumCuller::IsEntityInFrustum(const glm::vec3& position, float boundingSphereRadius, const Frustum& frustum)
{
    return frustum.IsSphereVisible(position, boundingSphereRadius);
}