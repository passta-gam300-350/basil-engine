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
    MainRenderingPass();
    ~MainRenderingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    static constexpr uint8_t MAIN_PASS_ID = 1;  // Execute after shadow pass (ID 0)
};