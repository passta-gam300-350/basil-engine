#pragma once

#include "../Utility/Camera.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

// Forward declaration to avoid cross-library dependencies
class Scene;

// Rendering coordinator - handles camera frustum culling
// Owned by SceneRenderer, not Scene - this is graphics-specific  
class FrustumCuller
{
public:
    FrustumCuller() = default;
    ~FrustumCuller() = default;

    // Graphics-specific culling - updates VisibilityComponent based on camera frustum
    void CullAgainstCamera(Scene* scene, Camera& camera);

private:
    struct Frustum {
        enum Planes { Left = 0, Right, Bottom, Top, Near, Far, Count };
        glm::vec4 planes[Planes::Count];
        
        void ExtractPlanes(const glm::mat4& viewProj);
        bool IsSphereVisible(const glm::vec3& center, float radius) const;
    };
    
    bool IsEntityInFrustum(const glm::vec3& position, float boundingSphereRadius, const Frustum& frustum);
};