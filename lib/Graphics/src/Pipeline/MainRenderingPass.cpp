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
        0.7f, 0.7f, 0.7f, 0.5f,  // r, g, b, a
        true,                      // clearColor
        true                       // clearDepth
    };

    Submit(clearCmd);

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