#include "Scene/SceneRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Buffer/FrameBuffer.h"
#include <glad/gl.h>

SceneRenderer::SceneRenderer() {
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
    // Default forward rendering pipeline
    auto mainPipeline = std::make_unique<RenderPipeline>("MainRendering");

    // Single forward rendering pass
    auto mainPass = std::make_shared<RenderPass>("MainPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGBA8 },
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    });

    mainPass->SetRenderFunction([this, mainPass]()
        {
            // Clear color and depth buffers
            glClearColor(0.7f, 0.7f, 0.7f, 0.5f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Standard forward rendering with submitted data
            if (!m_SubmittedRenderables.empty())
            {
                // 1. Update scene-wide lighting with submitted lights
                m_PBRLightingRenderer->UpdateLighting(m_SubmittedLights, m_AmbientLight, m_FrameData);

                // 2. Frustum culling on submitted renderables
                auto visibleRenderables = m_FrustumCuller->CullRenderables(m_SubmittedRenderables, m_FrameData);
                //auto visibleRenderables = m_SubmittedRenderables; // Skip culling - render all objects

                // 3. Forward instanced rendering with visible renderables
                m_InstancedRenderer->Render(visibleRenderables, m_FrameData);
            }

            // Store main color buffer
            m_FrameData.mainColorBuffer = mainPass->GetFramebuffer();
        });

    mainPipeline->AddPass(mainPass);
    AddPipeline("MainRendering", std::move(mainPipeline));

    // Set pipeline order
    SetPipelineOrder({ "MainRendering" });
}

void SceneRenderer::InitializeRenderingCoordinators() {
    // Create rendering coordinators - graphics-specific, not ECS systems
    m_PBRLightingRenderer = std::make_unique<PBRLightingRenderer>();  // Initialize lighting first
    m_MeshRenderer = std::make_unique<MeshRenderer>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
    m_InstancedRenderer = std::make_unique<InstancedRenderer>();
}

void SceneRenderer::Render() {
    // Update shared frame data
    UpdateFrameData();

    // Execute pipelines in order
    for (const auto &pipelineName : m_PipelineOrder)
    {
        auto pipelineIt = m_Pipelines.find(pipelineName);
        if (pipelineIt != m_Pipelines.end() && IsPipelineEnabled(pipelineName))
        {
            pipelineIt->second->Execute();
        }
    }

    m_FrameData.frameNumber++;
}

