#pragma once

#include "RenderPass.h"
#include "../Resources/Mesh.h"
#include <memory>

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
    DebugRenderPass(const DebugRenderPass&) = delete;
    DebugRenderPass& operator=(const DebugRenderPass&) = delete;
    DebugRenderPass(DebugRenderPass&&) = delete;
    DebugRenderPass& operator=(DebugRenderPass&&) = delete;
    ~DebugRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Light visualization controls
    void SetShowLightCubes(bool show) { m_ShowLightCubes = show; }
    bool GetShowLightCubes() const { return m_ShowLightCubes; }
    void SetBaseLightCubeSize(float size) { m_BaseLightCubeSize = size; InitializeLightVisualization(); }
    void SetIntensityScaleFactor(float factor) { m_IntensityScaleFactor = factor; }
    void SetCubeSizeRange(float minSize, float maxSize) { m_MinCubeSize = minSize; m_MaxCubeSize = maxSize; }

    // Debug buffer controls
    void SetClearColorBuffer(bool clear) { m_ClearColorBuffer = clear; }
    bool GetClearColorBuffer() const { return m_ClearColorBuffer; }

private:
    // Light visualization
    void RenderLightCubes(RenderContext& context);
    void InitializeLightVisualization();

    // Light visualization resources
    std::unique_ptr<Mesh> m_LightCube;
    bool m_ShowLightCubes = true;          // Toggle for light cube visualization
    float m_BaseLightCubeSize = 0.3f;      // Base size of the light cubes
    float m_IntensityScaleFactor = 0.2f;   // How much intensity affects size
    float m_MinCubeSize = 0.1f;            // Minimum cube size
    float m_MaxCubeSize = 2.0f;            // Maximum cube size

    // Debug buffer settings
    bool m_ClearColorBuffer = true;        // Whether to clear color buffer (enabled by default)

    static constexpr uint8_t DEBUG_PASS_ID = 2;  // Execute after main pass (ID 1)
};