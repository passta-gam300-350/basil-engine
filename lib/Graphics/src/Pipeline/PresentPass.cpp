#include "../../include/Pipeline/PresentPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include <glfw/glfw3.h>

PresentPass::PresentPass()
    : RenderPass("PresentPass")  // No FBO needed - we blit to screen
{
}

void PresentPass::Execute(RenderContext& context)
{
    // Begin pass (clears command buffer, no FBO binding since we have empty spec)
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Determine which buffer to present (post-process takes priority)
    std::shared_ptr<FrameBuffer> sourceBuffer;
    if (context.frameData.postProcessBuffer) {
        // Present tone-mapped LDR result (HDR pipeline active)
        sourceBuffer = context.frameData.postProcessBuffer;
    } else if (context.frameData.mainColorBuffer) {
        // Present main color buffer directly (no HDR/post-processing)
        sourceBuffer = context.frameData.mainColorBuffer;
    } else {
        // No buffer to present
        End();
        return;
    }

    auto mainFBO = sourceBuffer;

    // Get current window size dynamically
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    int windowWidth = 1280, windowHeight = 720;  // Default fallback
    if (currentWindow) {
        glfwGetFramebufferSize(currentWindow, &windowWidth, &windowHeight);
    }

    // Get main FBO dimensions
    const auto &mainFBOSpecs = mainFBO->GetSpecification();
    int srcWidth = static_cast<int>(mainFBOSpecs.Width);
    int srcHeight = static_cast<int>(mainFBOSpecs.Height);

    // Blit main scene to screen (already contains debug overlay)
    RenderCommands::BlitFramebufferData mainBlitCmd{
        mainFBO->GetFBOHandle(),  // Source FBO (main render target with debug composited)
        0,                        // Destination FBO (screen)
        0, 0, srcWidth, srcHeight,           // Source rectangle (full main FBO)
        0, 0, windowWidth, windowHeight,     // Destination rectangle (full screen)
        GL_COLOR_BUFFER_BIT,     // Copy color buffer
        GL_LINEAR                // Linear filtering for potential scaling
    };
    Submit(mainBlitCmd);

    // Execute commands for this pass
    ExecuteCommands();

    // End pass
    End();
}