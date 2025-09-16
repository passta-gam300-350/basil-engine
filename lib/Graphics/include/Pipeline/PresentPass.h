#pragma once

#include "RenderPass.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <glad/gl.h>

/**
 * Present Pass - Blits main render target to screen
 *
 * Final pass in the rendering pipeline that copies the main color buffer
 * to the screen framebuffer (FBO 0). Uses command buffer for proper
 * state management and no raw OpenGL calls.
 *
 * Uses pass ID 2 to ensure execution after main rendering pass (pass ID 1).
 */
class PresentPass : public RenderPass {
public:
    PresentPass();
    ~PresentPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    static constexpr uint8_t PRESENT_PASS_ID = 2;  // Execute after main pass (ID 1)
};