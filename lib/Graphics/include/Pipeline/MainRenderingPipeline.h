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
    MainRenderingPipeline();
    ~MainRenderingPipeline() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    void InitializePasses();
};