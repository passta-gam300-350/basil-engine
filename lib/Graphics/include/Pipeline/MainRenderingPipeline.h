#pragma once

#include "RenderPipeline.h"
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
 * Main Rendering Pipeline - Handles forward rendering of the scene
 *
 * Encapsulates all main rendering logic previously in SceneRenderer.
 * Manages the main forward rendering pass with PBR lighting and instanced rendering.
 */
class MainRenderingPipeline : public RenderPipeline {
public:
    MainRenderingPipeline(Renderer* renderer,
                         InstancedRenderer* instancedRenderer,
                         PBRLightingRenderer* lightingRenderer);
    ~MainRenderingPipeline() = default;

    // Data submission API - called each frame by SceneRenderer
    void SetRenderables(const std::vector<RenderableData>& renderables);
    void SetLights(const std::vector<SubmittedLightData>& lights);
    void SetAmbientLight(const glm::vec3& ambient);
    void UpdateFrameData(const FrameData& frameData);

    // Access to updated frame data after rendering
    const FrameData& GetFrameData() const { return m_FrameData; }

    // Override Execute to handle frame data updates
    void Execute();

private:
    void InitializeMainPass();

    // Rendering system references (not owned)
    Renderer* m_Renderer;
    InstancedRenderer* m_InstancedRenderer;
    PBRLightingRenderer* m_PBRLightingRenderer;

    // Frame data (updated each frame)
    std::vector<RenderableData> m_Renderables;
    std::vector<SubmittedLightData> m_Lights;
    glm::vec3 m_AmbientLight = glm::vec3(0.1f);
    FrameData m_FrameData;
};