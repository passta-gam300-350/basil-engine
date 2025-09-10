#include "Scene/SceneRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
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
    // Main Rendering Pipeline - coordinates your existing systems
    auto mainPipeline = std::make_unique<RenderPipeline>("MainRendering");

    // Create geometry pass
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGBA8 },     // Color
            { FBOTextureFormat::RGBA8 },     // Normal
            { FBOTextureFormat::RED_INTEGER },// Entity ID
            { FBOTextureFormat::DEPTH24STENCIL8 } // Depth
        }
        });

    // Set up render function for geometry pass - now properly coordinates rendering
    geometryPass->SetRenderFunction([this, geometryPass]()
        {
            // Clear the framebuffer
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Coordinate rendering pipeline properly
            if (m_Scene && m_Camera)
            {
                // 1. Frustum culling updates VisibilityComponent based on camera
                m_FrustumCuller->CullAgainstCamera(m_Scene.get(), *m_Camera);

                // 2. Mesh renderer queries visible entities and generates commands  
                m_MeshRenderer->Render(m_Scene.get(), *m_Camera);

                // 3. Instanced renderer generates instanced draw commands
                m_InstancedRenderer->Render(m_Scene.get(), *m_Camera);
            }

            // Store output for potential post-processing pipeline
            m_FrameData.mainColorBuffer = geometryPass->GetFramebuffer();
        });

    mainPipeline->AddPass(geometryPass);
    AddPipeline("MainRendering", std::move(mainPipeline));

    // Set default pipeline order (just main for now)
    SetPipelineOrder({ "MainRendering" });

    // Add other passes as needed (e.g., lighting, post-processing)
}

void SceneRenderer::InitializeRenderingCoordinators() {
    // Create rendering coordinators - graphics-specific, not ECS systems
    m_MeshRenderer = std::make_unique<MeshRenderer>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
    m_InstancedRenderer = std::make_unique<InstancedRenderer>();
}

void SceneRenderer::Render() {
    if (!m_Scene || !m_Camera) {
        return;
    }

    // Update shared frame data
    UpdateFrameData();

    // Execute pipelines in order
    for (const auto &pipelineName : m_PipelineOrder)
    {
        auto pipelineIt = m_Pipelines.find(pipelineName);
        if (pipelineIt != m_Pipelines.end() && IsPipelineEnabled(pipelineName))
        {
            // Debug output for pipeline execution (only show first few times)
            static std::unordered_map<std::string, int> pipelineExecuteCount;
            pipelineExecuteCount[pipelineName]++;
            
            if (pipelineExecuteCount[pipelineName] <= 3) {
                std::cout << "Executing pipeline: " << pipelineName << " (frame #" << m_FrameData.frameNumber << ")" << std::endl;
            }
            
            pipelineIt->second->Execute();
        }
    }

    m_FrameData.frameNumber++;
}