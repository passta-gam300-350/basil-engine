#pragma once

#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct RenderableData;
struct FrameData;

// Rendering coordinator - handles camera frustum culling
// Owned by SceneRenderer - this is graphics-specific  
class FrustumCuller
{
public:
    FrustumCuller() = default;
    ~FrustumCuller() = default;

    // Graphics-specific culling - filters renderables based on camera frustum
    std::vector<RenderableData> CullRenderables(const std::vector<RenderableData>& renderables, const FrameData& frameData);

private:
    struct Frustum {
        enum Planes { Left = 0, Right, Bottom, Top, Near, Far, Count };
        glm::vec4 planes[Planes::Count];
        
        void ExtractPlanes(const glm::mat4& viewProj);
        bool IsSphereVisible(const glm::vec3& center, float radius) const;
    };
    
    bool IsRenderableInFrustum(const RenderableData& renderable, const Frustum& frustum);
    glm::vec3 ExtractPosition(const glm::mat4& transform);
};