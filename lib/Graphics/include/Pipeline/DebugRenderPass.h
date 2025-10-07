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
    DebugRenderPass(std::shared_ptr<Shader> primitiveShader);
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
    void SetLightCubeMesh(const std::shared_ptr<Mesh>& mesh) { m_LightCube = mesh; }
    void SetDirectionalRayMesh(const std::shared_ptr<Mesh>& mesh) { m_DirectionalRay = mesh; }
    void SetAABBWireframeMesh(const std::shared_ptr<Mesh>& mesh) { m_AABBWireframe = mesh; }

    // Light visualization controls
    void SetShowLightCubes(bool show) { m_ShowLightCubes = show; }
    bool GetShowLightCubes() const { return m_ShowLightCubes; }
    void SetBaseLightCubeSize(float size) { m_BaseLightCubeSize = size; }
    void SetIntensityScaleFactor(float factor) { m_IntensityScaleFactor = factor; }
    void SetCubeSizeRange(float minSize, float maxSize) { m_MinCubeSize = minSize; m_MaxCubeSize = maxSize; }

    // Light ray visualization controls
    void SetShowLightRays(bool show) { m_ShowLightRays = show; }
    bool GetShowLightRays() const { return m_ShowLightRays; }
    void SetRayLength(float length) { m_RayLength = length; }
    void SetRayIntensityFactor(float factor) { m_RayIntensityFactor = factor; }  // Direct multiplier: rayLength = intensity * factor

    // AABB visualization controls
    void SetShowAABBs(bool show) { m_ShowAABBs = show; }
    bool GetShowAABBs() const { return m_ShowAABBs; }

    // Debug buffer controls
    void SetClearColorBuffer(bool clear) { m_ClearColorBuffer = clear; }
    bool GetClearColorBuffer() const { return m_ClearColorBuffer; }

private:
    // Light visualization
    void RenderLightCubes(RenderContext& context);
    void RenderLightRays(RenderContext& context);

    // AABB visualization
    void RenderAABBs(RenderContext& context);

    // Helper methods for ray rendering
    void RenderSingleRay(RenderContext& context, const std::shared_ptr<Mesh>& rayMesh, const glm::mat4& modelMatrix, const SubmittedLightData& light);

    // Shader storage
    std::shared_ptr<Shader> m_PrimitiveShader;

    // Debug visualization resources (injected, not owned)
    std::shared_ptr<Mesh> m_LightCube;
    std::shared_ptr<Mesh> m_DirectionalRay;
    std::shared_ptr<Mesh> m_AABBWireframe;

    // Light cube settings
    bool m_ShowLightCubes = true;          // Toggle for light cube visualization
    float m_BaseLightCubeSize = 0.3f;      // Base size of the light cubes
    float m_IntensityScaleFactor = 0.2f;   // How much intensity affects size
    float m_MinCubeSize = 0.1f;            // Minimum cube size
    float m_MaxCubeSize = 2.0f;            // Maximum cube size

    // Light ray settings
    bool m_ShowLightRays = true;          // Toggle for light ray visualization
    float m_RayLength = 3.0f;              // Base length of the light rays (unused - kept for compatibility)
    float m_RayIntensityFactor = 2.0f;     // Direct multiplier for intensity-to-length conversion

    // AABB settings
    bool m_ShowAABBs = true;              // Toggle for AABB wireframe visualization

    // Debug buffer settings
    bool m_ClearColorBuffer = true;        // Whether to clear color buffer (enabled by default)
};