#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"
#include <glfw/glfw3.h>

MainRenderingPass::MainRenderingPass()
    : RenderPass("MainPass", FBOSpecs
	{
        1280, 720,
{
            { FBOTextureFormat::RGBA8 },
            { FBOTextureFormat::DEPTH24STENCIL8 }
		}
    })
{
}

void MainRenderingPass::Execute(RenderContext& context)
{
    // Update framebuffer size to match current window before rendering
    UpdateFramebufferSize();

    // New context-based execution - use references instead of copies!
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Clear color and depth buffers using command buffer
    RenderCommands::ClearData clearCmd{
        0.7f, 0.7f, 0.7f, 1.0f,  // r, g, b, a (fully opaque background)
        true,                      // clearColor
        true                       // clearDepth
    };

    Submit(clearCmd);

    // Ensure proper depth testing state for opaque objects
    RenderCommands::SetDepthTestData depthTestCmd{
        true,           // enable depth testing
        GL_LESS,        // depth function (objects closer to camera pass)
        true            // enable depth writing
    };
    Submit(depthTestCmd);

    // Standard forward rendering with context data (no copies!)
    if (!context.renderables.empty())
    {
        // 1. Update scene-wide lighting with submitted lights
        context.pbrLighting.UpdateLighting(context.lights, context.ambientLight, context.frameData);

        // 2. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(context.renderables, context.frameData);

        // 3. Forward instanced rendering with visible renderables using pass-local buffer
        context.instancedRenderer.RenderToPass(*this, context.renderables, context.frameData);
    }

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Store main color buffer in frame data (direct update via reference!)
    context.frameData.mainColorBuffer = GetFramebuffer();

    // Create separate editor FBO copy to avoid conflicts with PresentPass
    CreateEditorFBOCopy(context);

    End();
}

void MainRenderingPass::UpdateFramebufferSize()
{
    // Get current window size
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (!currentWindow) return;

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(currentWindow, &windowWidth, &windowHeight);

    // Check if we need to resize the framebuffer
    auto currentSpecs = m_Framebuffer->GetSpecification();
    if (currentSpecs.Width != static_cast<uint32_t>(windowWidth) ||
        currentSpecs.Height != static_cast<uint32_t>(windowHeight)) {

        // Resize the framebuffer to match window size
        m_Framebuffer->Resize(static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight));

        // Update the viewport to match the new framebuffer size
        SetViewport(Viewport(0, 0, static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight)));
    }
}

void MainRenderingPass::CreateEditorFBOCopy(RenderContext& context)
{
    // Create editor FBO if it doesn't exist or size changed
    auto mainFBO = GetFramebuffer();
    const auto& mainSpec = mainFBO->GetSpecification();

    if (!context.frameData.editorColorBuffer ||
        context.frameData.editorColorBuffer->GetSpecification().Width != mainSpec.Width ||
        context.frameData.editorColorBuffer->GetSpecification().Height != mainSpec.Height)
    {
        // Create identical FBO specs for editor copy
        FBOSpecs editorSpec = mainSpec;
        context.frameData.editorColorBuffer = std::make_shared<FrameBuffer>(editorSpec);
    }

    // Blit main FBO to editor FBO
    auto editorFBO = context.frameData.editorColorBuffer;

    // Use OpenGL blit directly since we're not going through command buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO->GetFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, editorFBO->GetFBOHandle());

    glBlitFramebuffer(
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // Restore framebuffer binding
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}