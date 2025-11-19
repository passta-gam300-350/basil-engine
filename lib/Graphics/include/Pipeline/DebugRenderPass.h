/******************************************************************************/
/*!
\file   DebugRenderPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the DebugRenderPass class for rendering debug visualizations like lights and AABBs

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include "../Resources/Mesh.h"
#include <memory>

#include "Utility/RenderData.h"

// Forward declarations
class Shader;

/**
 * Debug Rendering Pass - Handles debug visualizations
 *
 * Uses the primitive shader to render debug geometry like light cubes,
 * wireframes, bounding boxes, etc. This pass renders after the main
 * scene to overlay debug information.
 */
class DebugRenderPass : public RenderPass
{
public:
    DebugRenderPass();
    explicit DebugRenderPass(const std::shared_ptr<Shader>& primitiveShader);
    DebugRenderPass(const DebugRenderPass&) = delete;
    DebugRenderPass& operator=(const DebugRenderPass&) = delete;
    DebugRenderPass(DebugRenderPass&&) = delete;
    DebugRenderPass& operator=(DebugRenderPass&&) = delete;
    ~DebugRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set primitive shader (alternative to constructor injection)
    void SetPrimitiveShader(const std::shared_ptr<Shader>& shader) { m_PrimitiveShader = shader; }

    // Set debug meshes (injected from application layer)
    void SetDirectionalRayMesh(const std::shared_ptr<Mesh>& mesh) { m_DirectionalRay = mesh; }
    void SetAABBWireframeMesh(const std::shared_ptr<Mesh>& mesh) { m_AABBWireframe = mesh; }

    // Light ray visualization controls
    void SetShowLightRays(bool show) { m_ShowLightRays = show; }
    bool GetShowLightRays() const { return m_ShowLightRays; }
    void SetRayLength(float length) { m_RayLength = length; }
    void SetRayIntensityFactor(float factor) { m_RayIntensityFactor = factor; }  // Direct multiplier: rayLength = intensity * factor

    // AABB visualization controls
    void SetShowAABBs(bool show) { m_ShowAABBs = show; }
    bool GetShowAABBs() const { return m_ShowAABBs; }

    // Physics debug visualization controls
    void SetShowPhysicsDebug(bool show) { m_ShowPhysicsDebug = show; }
    bool GetShowPhysicsDebug() const { return m_ShowPhysicsDebug; }

    // Debug buffer controls
    void SetClearColorBuffer(bool clear) { m_ClearColorBuffer = clear; }
    bool GetClearColorBuffer() const { return m_ClearColorBuffer; }

private:
    // Light visualization
    void RenderLightRays(RenderContext& context);

    // AABB visualization
    void RenderAABBs(RenderContext& context);

    // Physics debug line visualization
    void RenderDebugLines(RenderContext& context);

    // Helper methods for ray rendering
    void RenderSingleRay(RenderContext& context, const std::shared_ptr<Mesh>& rayMesh, const glm::mat4& modelMatrix, const SubmittedLightData& light);

    // Shader storage
    std::shared_ptr<Shader> m_PrimitiveShader;

    // Debug visualization resources (injected, not owned)
    std::shared_ptr<Mesh> m_DirectionalRay;
    std::shared_ptr<Mesh> m_AABBWireframe;

    // Light ray settings
    bool m_ShowLightRays = true;          // Toggle for light ray visualization
    float m_RayLength = 3.0f;              // Base length of the light rays (unused - kept for compatibility)
    float m_RayIntensityFactor = 2.0f;     // Direct multiplier for intensity-to-length conversion

    // AABB settings
    bool m_ShowAABBs = false;              // Toggle for AABB wireframe visualization

    // Physics debug settings
    bool m_ShowPhysicsDebug = true;        // Toggle for physics debug line visualization

    // OpenGL resources for physics debug lines
    uint32_t m_DebugLineVAO = 0;           // VAO for debug lines
    uint32_t m_DebugLineVBO = 0;           // VBO for debug line vertices

    // Debug buffer settings
    bool m_ClearColorBuffer = true;        // Whether to clear color buffer (enabled by default)
};