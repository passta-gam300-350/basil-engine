#pragma once

#include "RenderPass.h"

/**
 * Present Pass - Blits main render target to screen
 *
 * Final pass in the rendering pipeline that copies the main color buffer
 * to the screen framebuffer (FBO 0). Uses command buffer for proper
 * state management and no raw OpenGL calls.
 *
 * Uses pass ID 3 to ensure execution after debug rendering pass (pass ID 2).
 */
class PresentPass : public RenderPass {
public:
    PresentPass();
	PresentPass(const PresentPass&) = delete;
	PresentPass& operator=(const PresentPass&) = delete;
	PresentPass(PresentPass&&) = delete;
	PresentPass& operator=(PresentPass&&) = delete;
    ~PresentPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    static constexpr uint8_t PRESENT_PASS_ID = 3;  // Execute after debug pass (ID 2)
};