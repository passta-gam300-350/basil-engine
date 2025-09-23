#pragma once

#include "RenderPass.h"

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
class MainRenderingPass : public RenderPass
{
public:
    MainRenderingPass();
	MainRenderingPass(const MainRenderingPass&) = delete;
	MainRenderingPass& operator=(const MainRenderingPass&) = delete;
	MainRenderingPass(MainRenderingPass&&) = delete;
	MainRenderingPass& operator=(MainRenderingPass&&) = delete;
    ~MainRenderingPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    // Update framebuffer to match current window size
    void UpdateFramebufferSize();

    static constexpr uint8_t MAIN_PASS_ID = 1;  // Execute after shadow pass (ID 0)
};