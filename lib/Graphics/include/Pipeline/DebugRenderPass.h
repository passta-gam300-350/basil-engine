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
 * Uses the debug_line shader to render physics debug lines (wireframes,
 * collision shapes, contact points, velocities, etc). This pass renders
 * after the main scene to overlay debug information. Renders only in
 * editor camera context (Scene viewport).
 */
class DebugRenderPass : public RenderPass
{
public:
    DebugRenderPass();
    explicit DebugRenderPass(const std::shared_ptr<Shader>& debugLineShader);
    DebugRenderPass(const DebugRenderPass&) = delete;
    DebugRenderPass& operator=(const DebugRenderPass&) = delete;
    DebugRenderPass(DebugRenderPass&&) = delete;
    DebugRenderPass& operator=(DebugRenderPass&&) = delete;
    ~DebugRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set debug line shader (alternative to constructor injection)
    void SetDebugLineShader(const std::shared_ptr<Shader>& shader) { m_DebugLineShader = shader; }

    // Physics debug visualization controls
    void SetShowPhysicsDebug(bool show) { m_ShowPhysicsDebug = show; }
    bool GetShowPhysicsDebug() const { return m_ShowPhysicsDebug; }

    // Debug buffer controls
    void SetClearColorBuffer(bool clear) { m_ClearColorBuffer = clear; }
    bool GetClearColorBuffer() const { return m_ClearColorBuffer; }

private:
    // Physics debug line visualization
    void RenderDebugLines(RenderContext& context);

    // Shader storage
    std::shared_ptr<Shader> m_DebugLineShader;  // Dedicated shader for debug line rendering

    // Physics debug settings
    bool m_ShowPhysicsDebug = true;        // Toggle for physics debug line visualization

    // OpenGL resources for physics debug lines
    uint32_t m_DebugLineVAO = 0;           // VAO for debug lines
    uint32_t m_DebugLineVBO = 0;           // VBO for debug line vertices

    // Debug buffer settings
    bool m_ClearColorBuffer = true;        // Whether to clear color buffer (enabled by default)
};