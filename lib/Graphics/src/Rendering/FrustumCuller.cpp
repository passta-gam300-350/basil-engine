#include "Rendering/FrustumCuller.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include <glm/gtc/matrix_transform.hpp>

std::vector<RenderableData> FrustumCuller::CullRenderables(const std::vector<RenderableData>& renderables, const FrameData& frameData)
{
    std::vector<RenderableData> visibleRenderables;
    
    if (renderables.empty()) {
        return visibleRenderables;
    }

    // Extract frustum planes from frame data
    Frustum frustum{};
    glm::mat4 viewProj = frameData.projectionMatrix * frameData.viewMatrix;
    frustum.ExtractPlanes(viewProj);
    
    // Filter renderables based on frustum culling
    for (const auto& renderable : renderables)
    {
        // Skip if already marked invisible
        if (!renderable.visible) {
            continue;
        }
            
        // Check if renderable is in camera frustum
        if (IsRenderableInFrustum(renderable, frustum))
        {
            visibleRenderables.push_back(renderable);
        }
    }
    
    return visibleRenderables;
}

bool FrustumCuller::IsRenderableInFrustum(const RenderableData& renderable, const Frustum& frustum)
{
    // Extract position from transform matrix
    glm::vec3 position = ExtractPosition(renderable.transform);
    
    // Use a default bounding sphere radius - could be improved by getting actual mesh bounds
    float boundingSphereRadius = 1.0f;
    
    return frustum.IsSphereVisible(position, boundingSphereRadius);
}

glm::vec3 FrustumCuller::ExtractPosition(const glm::mat4& transform)
{
    return glm::vec3(transform[3]); // Translation component
}

void FrustumCuller::Frustum::ExtractPlanes(const glm::mat4& viewProj)
{
    // Left plane
    // NOLINT(cppcoreguidelines-pro-type-union-access)
    planes[Left] = glm::vec4(viewProj[0][3] + viewProj[0][0],
                             viewProj[1][3] + viewProj[1][0],
                             viewProj[2][3] + viewProj[2][0],
                             viewProj[3][3] + viewProj[3][0]);

    // Right plane
    planes[Right] = glm::vec4(viewProj[0][3] - viewProj[0][0],
                              viewProj[1][3] - viewProj[1][0],
                              viewProj[2][3] - viewProj[2][0],
                              viewProj[3][3] - viewProj[3][0]);

    // Bottom plane
    planes[Bottom] = glm::vec4(viewProj[0][3] + viewProj[0][1],
                               viewProj[1][3] + viewProj[1][1],
                               viewProj[2][3] + viewProj[2][1],
                               viewProj[3][3] + viewProj[3][1]);

    // Top plane
    planes[Top] = glm::vec4(viewProj[0][3] - viewProj[0][1],
                            viewProj[1][3] - viewProj[1][1],
                            viewProj[2][3] - viewProj[2][1],
                            viewProj[3][3] - viewProj[3][1]);

    // Near plane
    planes[Near] = glm::vec4(viewProj[0][3] + viewProj[0][2],
                             viewProj[1][3] + viewProj[1][2],
                             viewProj[2][3] + viewProj[2][2],
                             viewProj[3][3] + viewProj[3][2]);

    // Far plane
    planes[Far] = glm::vec4(viewProj[0][3] - viewProj[0][2],
                            viewProj[1][3] - viewProj[1][2],
                            viewProj[2][3] - viewProj[2][2],
                            viewProj[3][3] - viewProj[3][2]);

    // Normalize all planes
    for (int i = 0; i < Planes::Count; ++i) {
        float length = glm::length(glm::vec3(planes[i]));
        planes[i] /= length;
    }
}

bool FrustumCuller::Frustum::IsSphereVisible(const glm::vec3& center, float radius) const
{
    for (int i = 0; i < Planes::Count; ++i) {
        // Extract plane normal (xyz) and distance (w)
        glm::vec3 normal(planes[i][0], planes[i][1], planes[i][2]);
        float distance = planes[i][3];
        if (glm::dot(normal, center) + distance < -radius) {
            return false;
        }
    }
    return true;
}

