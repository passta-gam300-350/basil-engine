#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class Renderer;
class InstancedRenderer;
class PBRLightingRenderer;

/**
 * Main Rendering Pass - Executes the main forward rendering
 *
 * Handles the main scene rendering with PBR lighting, clearing, and instanced rendering.
 * Contains the render function logic previously in SceneRenderer's main pass.
 */
class MainRenderingPass : public RenderPass {
public:
    MainRenderingPass(Renderer* renderer,
                     InstancedRenderer* instancedRenderer,
                     PBRLightingRenderer* lightingRenderer);
    ~MainRenderingPass() = default;

    // Data update API - called each frame before execution
    void SetRenderData(const std::vector<RenderableData>& renderables,
                      const std::vector<SubmittedLightData>& lights,
                      const glm::vec3& ambientLight,
                      const FrameData& frameData);

    // Override Execute to implement main rendering logic
    void Execute() override;

    // Access to updated frame data after rendering
    const FrameData& GetFrameData() const { return m_FrameData; }

private:
    // Rendering system references (not owned)
    Renderer* m_Renderer;
    InstancedRenderer* m_InstancedRenderer;
    PBRLightingRenderer* m_PBRLightingRenderer;

    // Render data (set each frame)
    std::vector<RenderableData> m_Renderables;
    std::vector<SubmittedLightData> m_Lights;
    glm::vec3 m_AmbientLight = glm::vec3(0.1f);
    FrameData m_FrameData;
};