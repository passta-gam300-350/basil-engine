#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/Renderer.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"

MainRenderingPass::MainRenderingPass()
    : RenderPass("MainPass", FBOSpecs{
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
    // New context-based execution - use references instead of copies!
    Begin();

    // Clear color and depth buffers using command buffer
    RenderCommands::ClearData clearCmd{
        0.7f, 0.7f, 0.7f, 0.5f,  // r, g, b, a
        true,                      // clearColor
        true                       // clearDepth
    };
    context.renderer.Submit(clearCmd);

    // Standard forward rendering with context data (no copies!)
    if (!context.renderables.empty())
    {
        // 1. Update scene-wide lighting with submitted lights
        context.pbrLighting.UpdateLighting(context.lights, context.ambientLight, context.frameData);

        // 2. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(context.renderables, context.frameData);

        // 3. Forward instanced rendering with visible renderables
        context.instancedRenderer.Render(context.renderables, context.frameData);
    }

    // Store main color buffer in frame data (direct update via reference!)
    context.frameData.mainColorBuffer = GetFramebuffer();

    End();
}