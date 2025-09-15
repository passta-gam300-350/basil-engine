#include "../../include/Pipeline/MainRenderingPipeline.h"
#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Core/Renderer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"

MainRenderingPipeline::MainRenderingPipeline(Renderer* renderer,
                                           InstancedRenderer* instancedRenderer,
                                           PBRLightingRenderer* lightingRenderer)
    : RenderPipeline("MainRendering")
    , m_Renderer(renderer)
    , m_InstancedRenderer(instancedRenderer)
    , m_PBRLightingRenderer(lightingRenderer)
{
    InitializeMainPass();
}

void MainRenderingPipeline::SetRenderables(const std::vector<RenderableData>& renderables)
{
    m_Renderables = renderables;
}

void MainRenderingPipeline::SetLights(const std::vector<SubmittedLightData>& lights)
{
    m_Lights = lights;
}

void MainRenderingPipeline::SetAmbientLight(const glm::vec3& ambient)
{
    m_AmbientLight = ambient;
}

void MainRenderingPipeline::UpdateFrameData(const FrameData& frameData)
{
    m_FrameData = frameData;

    // Update the main pass with current frame data
    auto mainPass = std::static_pointer_cast<MainRenderingPass>(GetPass("MainPass"));
    if (mainPass) {
        mainPass->SetRenderData(m_Renderables, m_Lights, m_AmbientLight, m_FrameData);
    }
}

void MainRenderingPipeline::Execute()
{
    // Call base class Execute to run all passes
    RenderPipeline::Execute();

    // Update our frame data with any changes from the main pass
    auto mainPass = std::static_pointer_cast<MainRenderingPass>(GetPass("MainPass"));
    if (mainPass) {
        m_FrameData = mainPass->GetFrameData();
    }
}

void MainRenderingPipeline::InitializeMainPass()
{
    // Create the main rendering pass with the same specs as SceneRenderer
    auto mainPass = std::make_shared<MainRenderingPass>(
        m_Renderer,
        m_InstancedRenderer,
        m_PBRLightingRenderer
    );

    AddPass(mainPass);
}