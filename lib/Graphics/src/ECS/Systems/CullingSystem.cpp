#include "ECS/Systems/CullingSystem.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include <glm/gtc/matrix_transform.hpp>

// Helper structure for frustum planes
struct Frustum
{
    enum Planes
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    glm::vec4 planes[Planes::Count];

    void ExtractPlanes(const glm::mat4& viewProj)
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
        for (int i = 0; i < Planes::Count; ++i)
        {
            float length = glm::sqrt(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
            planes[i] /= length;
        }
    }

    bool IsSphereVisible(const glm::vec3& center, float radius) const
    {
        for (int i = 0; i < Planes::Count; ++i) {
            if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius)
            {
                return false;
            }
        }
        return true;
    }
};

void CullingSystem::OnUpdate(Scene* scene, const Camera& camera)
{
    if (!scene)
    {
        return;
    }

    // Extract frustum planes from view-projection matrix
    Frustum frustum;
    frustum.ExtractPlanes(camera.GetViewProjectionMatrix());

    // Get all entities with mesh and transform components
    auto view = scene->GetRegistry().view<MeshComponent, TransformComponent>();

    // Test each entity against the frustum
    view.each([&](auto entity, auto& mesh, auto& transform)
    {
        // Get position and bounding sphere radius
        const glm::vec3& position = transform.Translation;
        float boundingSphereRadius = 1.0f; // This should be calculated or stored per mesh

        // Check if entity is visible
        bool visible = IsEntityVisible(camera, position, boundingSphereRadius);

        // Mark entity as visible or not (you'd need a VisibilityComponent or similar)
        // For now, we'll just print debug info
        // std::cout << "Entity " << (uint32_t)entity << " is " << (visible ? "visible" : "culled") << std::endl;
    });
}

bool CullingSystem::IsEntityVisible(const Camera& camera, const glm::vec3& position, float boundingSphereRadius)
{
    // Extract frustum planes from view-projection matrix
    Frustum frustum;
    frustum.ExtractPlanes(camera.GetViewProjectionMatrix());

    // Check if bounding sphere is inside frustum
    return frustum.IsSphereVisible(position, boundingSphereRadius);
}