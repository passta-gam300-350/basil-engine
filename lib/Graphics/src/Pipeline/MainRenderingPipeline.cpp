#include "../../include/Pipeline/MainRenderingPipeline.h"
#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Pipeline/ShadowMappingPass.h"
#include "../../include/Pipeline/PresentPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/Renderer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"

MainRenderingPipeline::MainRenderingPipeline()
    : RenderPipeline("MainRendering")
{
    InitializePasses();
}


void MainRenderingPipeline::Execute(RenderContext& context)
{
    // New context-based execution - no data copying needed!
    // The context contains references to all the data we need

    // Call base class Execute with context to run all passes
    RenderPipeline::Execute(context);

    // FrameData updates happen through the context reference automatically
    // No need for manual synchronization!
}

void MainRenderingPipeline::InitializePasses()
{
    // 1. Add shadow mapping pass (executes first with pass ID 0)
    auto shadowPass = std::make_shared<ShadowMappingPass>();
    AddPass(shadowPass);

    // 2. Add main rendering pass (executes second with pass ID 1)
    auto mainPass = std::make_shared<MainRenderingPass>();
    AddPass(mainPass);

    // 3. Add present pass (executes third with pass ID 2)
    auto presentPass = std::make_shared<PresentPass>();
    AddPass(presentPass);
}