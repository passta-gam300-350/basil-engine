/******************************************************************************/
/*!
\file   FrustumCuller.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the FrustumCuller class for camera frustum culling of renderable objects

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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