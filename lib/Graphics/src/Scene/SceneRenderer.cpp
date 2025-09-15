#include "Scene/SceneRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/MainRenderingPipeline.h"
#include "Buffer/FrameBuffer.h"
#include <glad/gl.h>

SceneRenderer::SceneRenderer(GLFWwindow* window) {
    // Initialize core systems first
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Initialize(window);

    m_ResourceManager = std::make_unique<ResourceManager>();
    m_ResourceManager->Initialize();

    // Initialize rendering coordinators with dependencies
    InitializeRenderingCoordinators();
    InitializeDefaultPipelines();
}

SceneRenderer::~SceneRenderer() {
    // Pipeline and coordinators will be cleaned up by unique_ptr
}

void SceneRenderer::SubmitRenderable(const RenderableData& renderable) {
    m_SubmittedRenderables.push_back(renderable);
}

void SceneRenderer::SubmitLight(const SubmittedLightData& light) {
    m_SubmittedLights.push_back(light);
}

void SceneRenderer::ClearFrame() {
    // Don't clear renderables or lights - they are static and submitted once during initialization
}

void SceneRenderer::AddPipeline(std::string const &name, std::unique_ptr<RenderPipeline> pipeline)
{
    m_Pipelines[name] = std::move(pipeline);
    m_PipelineEnabled[name] = true;

    // Add to order if not already there
    if (std::find(m_PipelineOrder.begin(), m_PipelineOrder.end(), name) == m_PipelineOrder.end())
    {
        m_PipelineOrder.push_back(name);
    }
}

void SceneRenderer::RemovePipeline(std::string const &name)
{
    m_Pipelines.erase(name);
    m_PipelineEnabled.erase(name);
    m_PipelineOrder.erase(
        std::remove(m_PipelineOrder.begin(), m_PipelineOrder.end(), name),
        m_PipelineOrder.end()
    );
}

void SceneRenderer::SetPipelineOrder(std::vector<std::string> const &order)
{
    m_PipelineOrder = order;
}

RenderPipeline *SceneRenderer::GetPipeline(std::string const &name)
{
    auto it = m_Pipelines.find(name);
    return it != m_Pipelines.end() ? it->second.get() : nullptr;
}

void SceneRenderer::EnablePipeline(std::string const &name, bool enabled)
{
    m_PipelineEnabled[name] = enabled;
}

bool SceneRenderer::IsPipelineEnabled(std::string const &name) const
{
    auto it = m_PipelineEnabled.find(name);
    return it != m_PipelineEnabled.end() ? it->second : false;
}

void SceneRenderer::InitializeDefaultPipelines() {
    // Create main rendering pipeline using dedicated class
    auto mainPipeline = std::make_unique<MainRenderingPipeline>(
        m_Renderer.get(),
        m_InstancedRenderer.get(),
        m_PBRLightingRenderer.get()
    );

    AddPipeline("MainRendering", std::move(mainPipeline));

    // Set pipeline order
    SetPipelineOrder({ "MainRendering" });
}

void SceneRenderer::InitializeRenderingCoordinators() {
    // Create rendering coordinators with explicit dependencies
    m_PBRLightingRenderer = std::make_unique<PBRLightingRenderer>();  // Initialize lighting first
    m_MeshRenderer = std::make_unique<MeshRenderer>(m_Renderer.get());
    m_FrustumCuller = std::make_unique<FrustumCuller>();
    m_InstancedRenderer = std::make_unique<InstancedRenderer>(m_Renderer.get(), m_PBRLightingRenderer.get());
}

void SceneRenderer::Render() {
    // Begin frame
    m_Renderer->BeginFrame();

    // Update shared frame data
    UpdateFrameData();

    // Pass data to main pipeline
    auto mainPipeline = static_cast<MainRenderingPipeline*>(GetPipeline("MainRendering"));
    if (mainPipeline) {
        mainPipeline->SetRenderables(m_SubmittedRenderables);
        mainPipeline->SetLights(m_SubmittedLights);
        mainPipeline->SetAmbientLight(m_AmbientLight);
        mainPipeline->UpdateFrameData(m_FrameData);
    }

    // Execute pipelines in order
    for (const auto &pipelineName : m_PipelineOrder)
    {
        auto pipelineIt = m_Pipelines.find(pipelineName);
        if (pipelineIt != m_Pipelines.end() && IsPipelineEnabled(pipelineName))
        {
            pipelineIt->second->Execute();
        }
    }

    // Update frame data with any changes from main pipeline
    if (mainPipeline) {
        m_FrameData = mainPipeline->GetFrameData();
    }

    m_FrameData.frameNumber++;

    // End frame
    m_Renderer->EndFrame();
}

