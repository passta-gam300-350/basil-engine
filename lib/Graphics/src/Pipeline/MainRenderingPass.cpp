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

    // Create sort key for main pass - use pass ID 1 to execute after shadow pass
    RenderCommands::CommandSortKey mainSortKey;
    mainSortKey.pass = MAIN_PASS_ID;
    mainSortKey.material = 0;
    mainSortKey.mesh = 0;
    mainSortKey.instance = 0;

    Submit(clearCmd, mainSortKey);
    std::cout << "MainRenderingPass: Submitted clear command" << std::endl;

    // Standard forward rendering with context data (no copies!)
    if (!context.renderables.empty())
    {
        std::cout << "MainRenderingPass: Processing " << context.renderables.size() << " renderables" << std::endl;

        // 1. Update scene-wide lighting with submitted lights
        context.pbrLighting.UpdateLighting(context.lights, context.ambientLight, context.frameData);

        // 2. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(context.renderables, context.frameData);

        // 3. Forward instanced rendering with visible renderables using pass-local buffer
        context.instancedRenderer.RenderToPass(*this, context.renderables, context.frameData);
    } else {
        std::cout << "MainRenderingPass: No renderables to process!" << std::endl;
    }

    // Execute all commands submitted to this pass's command buffer
    std::cout << "MainRenderingPass: Executing " << m_PassCommandBuffer->GetCommandCount() << " commands" << std::endl;
    ExecuteCommands();

    // DEBUG: Check if FBO has rendered content
    std::cout << "MainPass: Rendered to FBO handle = " << GetFramebuffer()->GetFBOHandle() << std::endl;
    std::cout << "MainPass: Color texture ID = " << GetFramebuffer()->GetColorAttachmentRendererID(0) << std::endl;
    std::cout << "MainPass: Depth texture ID = " << GetFramebuffer()->GetDepthAttachmentRendererID() << std::endl;

    // Store main color buffer in frame data (direct update via reference!)
    context.frameData.mainColorBuffer = GetFramebuffer();

    End();
}