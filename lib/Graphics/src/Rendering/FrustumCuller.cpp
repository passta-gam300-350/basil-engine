#include "Rendering/FrustumCuller.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

std::vector<RenderableData> FrustumCuller::CullRenderables(const std::vector<RenderableData>& renderables, const FrameData& frameData)
{
    std::vector<RenderableData> visibleRenderables;
    
    if (renderables.empty())
        return visibleRenderables;

    // Extract frustum planes from frame data
    Frustum frustum;
    glm::mat4 viewProj = frameData.projectionMatrix * frameData.viewMatrix;
    frustum.ExtractPlanes(viewProj);
    
    // Filter renderables based on frustum culling
    for (const auto& renderable : renderables)
    {
        // Skip if already marked invisible
        if (!renderable.visible)
            continue;
            
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

glm::vec3 FrustumCuller::ExtractPosition(const glm::mat4& transform) const
{
    return glm::vec3(transform[3]); // Translation component
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

