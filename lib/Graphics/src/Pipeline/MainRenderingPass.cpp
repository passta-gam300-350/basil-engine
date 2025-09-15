#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Core/Renderer.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"

MainRenderingPass::MainRenderingPass(Renderer* renderer,
                                   InstancedRenderer* instancedRenderer,
                                   PBRLightingRenderer* lightingRenderer)
    : RenderPass("MainPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGBA8 },
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    })
    , m_Renderer(renderer)
    , m_InstancedRenderer(instancedRenderer)
    , m_PBRLightingRenderer(lightingRenderer)
{
}

void MainRenderingPass::SetRenderData(const std::vector<RenderableData>& renderables,
                                     const std::vector<SubmittedLightData>& lights,
                                     const glm::vec3& ambientLight,
                                     const FrameData& frameData)
{
    m_Renderables = renderables;
    m_Lights = lights;
    m_AmbientLight = ambientLight;
    m_FrameData = frameData;
}

void MainRenderingPass::Execute()
{
    // Call base class Begin() to bind framebuffer and set viewport
    Begin();

    // Main rendering logic extracted from SceneRenderer::InitializeDefaultPipelines()

    // Clear color and depth buffers using command buffer
    RenderCommands::ClearData clearCmd{
        0.7f, 0.7f, 0.7f, 0.5f,  // r, g, b, a
        true,                      // clearColor
        true                       // clearDepth
    };
    m_Renderer->Submit(clearCmd);

    // Standard forward rendering with submitted data
    if (!m_Renderables.empty())
    {
        // 1. Update scene-wide lighting with submitted lights
        m_PBRLightingRenderer->UpdateLighting(m_Lights, m_AmbientLight, m_FrameData);

        // 2. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(m_Renderables, m_FrameData);
        // auto visibleRenderables = m_Renderables; // Skip culling - render all objects

        // 3. Forward instanced rendering with visible renderables
        m_InstancedRenderer->Render(m_Renderables, m_FrameData);
    }

    // Store main color buffer in frame data
    // Note: This updates our local copy - the pipeline should handle propagating this back
    m_FrameData.mainColorBuffer = GetFramebuffer();

    // Call base class End() to unbind framebuffer
    End();
}