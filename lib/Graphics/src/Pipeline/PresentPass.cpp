#include "../../include/Pipeline/PresentPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"

PresentPass::PresentPass()
    : RenderPass("PresentPass", FBOSpecs{0, 0, {}})  // No FBO needed - we blit to screen
{
}

void PresentPass::Execute(RenderContext& context)
{

    // Begin pass (clears command buffer, no FBO binding since we have empty spec)
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Check if we have a main color buffer to present
    if (!context.frameData.mainColorBuffer) {
        End();
        return;
    }

    auto mainFBO = context.frameData.mainColorBuffer;

    // Create blit command to copy main FBO to screen
    RenderCommands::BlitFramebufferData blitCmd{
        mainFBO->GetFBOHandle(),  // Source FBO (main render target)
        0,                        // Destination FBO (screen)
        0, 0, 1280, 720,         // Source rectangle (full main FBO)
        0, 0, 1280, 720,         // Destination rectangle (full screen)
        GL_COLOR_BUFFER_BIT,     // Copy color buffer
        GL_LINEAR                // Linear filtering for potential scaling
    };

    // Create sort key for present pass - use pass ID 2 to execute after main pass

    // Submit blit command to pass command buffer
    Submit(blitCmd);

    // Execute commands for this pass
    ExecuteCommands();


    // End pass
    End();
}