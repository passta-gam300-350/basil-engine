#include "Scene/SceneRenderer.h"
#include "Pipeline/MainRenderingPass.h"
#include "Pipeline/DebugRenderPass.h"
#include "Pipeline/PickingRenderPass.h"
#include "Pipeline/RenderContext.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Pipeline/PresentPass.h"
#include "Pipeline/ShadowMappingPass.h"
#include "Resources/Shader.h"
#include "Resources/Mesh.h"
#include <cassert>

SceneRenderer::SceneRenderer()
{
    m_ResourceManager = std::make_unique<ResourceManager>();
    assert(m_ResourceManager && "Failed to create ResourceManager");

    m_ResourceManager->Initialize();

    // Initialize texture slot manager
    m_TextureSlotManager = std::make_unique<TextureSlotManager>();
    assert(m_TextureSlotManager && "Failed to create TextureSlotManager");

    // Initialize rendering coordinators with dependencies
    InitializeRenderingCoordinators();
    InitializeDefaultPipeline();
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

void SceneRenderer::ClearFrame()
{
    m_SubmittedRenderables.clear();
	m_SubmittedLights.clear();
}


void SceneRenderer::InitializeDefaultPipeline()
{
    // Create main rendering pipeline
    auto mainPipeline = std::make_unique<RenderPipeline>();

    // 1. Add shadow mapping pass
    // Shadow pass will need shader to be set after creation by the application
    auto shadowPass = std::make_shared<ShadowMappingPass>();
    mainPipeline->AddPass(shadowPass);

    // 2. Add main rendering pass (now includes skybox rendering)
    auto mainPass = std::make_shared<MainRenderingPass>();
    mainPipeline->AddPass(mainPass);

    // 3. Add debug rendering pass
    auto debugPass = std::make_shared<DebugRenderPass>();
    mainPipeline->AddPass(debugPass);
    mainPipeline->EnablePass("DebugPass", false);

    // 4. Add picking pass (executes when needed, disabled by default)
    auto pickingPass = std::make_shared<PickingRenderPass>();
    mainPipeline->AddPass(pickingPass);
    mainPipeline->EnablePass("PickingPass", false);  // Disabled by default

    // 5. Add present pass (executes last)
    auto presentPass = std::make_shared<PresentPass>();
    mainPipeline->AddPass(presentPass);

    m_Pipeline = std::move(mainPipeline);
}

void SceneRenderer::InitializeRenderingCoordinators()
{
    // Create rendering coordinators with explicit dependencies
    m_PBRLightingRenderer = std::make_unique<PBRLightingRenderer>();  // Initialize lighting first
    assert(m_PBRLightingRenderer && "Failed to create PBRLightingRenderer");

    //m_MeshRenderer = std::make_unique<MeshRenderer>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
    assert(m_FrustumCuller && "Failed to create FrustumCuller");

    m_InstancedRenderer = std::make_unique<InstancedRenderer>(m_PBRLightingRenderer.get());
    assert(m_InstancedRenderer && "Failed to create InstancedRenderer");
}

void SceneRenderer::Render()
{
    assert(m_InstancedRenderer && "InstancedRenderer must be initialized before rendering");
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized before rendering");
    assert(m_ResourceManager && "ResourceManager must be initialized before rendering");
    assert(m_TextureSlotManager && "TextureSlotManager must be initialized before rendering");

    // Create context with references to our data - NO COPYING!
    RenderContext context(
        m_SubmittedRenderables,  // const ref to renderables
        m_SubmittedLights,       // const ref to lights
        m_AmbientLight,          // const ref to ambient light
        m_FrameData,             // mutable ref to frame data
        *m_InstancedRenderer,    // ref to instanced renderer
        *m_PBRLightingRenderer,  // ref to PBR lighting
        *m_ResourceManager,      // ref to resource manager
        *m_TextureSlotManager    // ref to texture slot manager
    );

    // Execute the single pipeline
    if (m_Pipeline) {
        m_Pipeline->Execute(context);
    } else {
        assert(false && "Render pipeline must be initialized before rendering");
    }

    // Frame data updates happened automatically through context reference
    // No manual synchronization needed!
    ++m_FrameData.frameNumber;
}

void SceneRenderer::SetShadowDepthShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Shadow depth shader cannot be null");
    assert(shader->ID != 0 && "Shadow depth shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting shadow shader");

    if (m_Pipeline) {
        auto shadowPass = std::dynamic_pointer_cast<ShadowMappingPass>(m_Pipeline->GetPass("ShadowPass"));
        if (shadowPass) {
            shadowPass->SetShadowDepthShader(shader);
        }
    }
}

void SceneRenderer::SetDebugPrimitiveShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Debug primitive shader cannot be null");
    assert(shader->ID != 0 && "Debug primitive shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting debug shader");

    if (m_Pipeline) {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass) {
            debugPass->SetPrimitiveShader(shader);
        }
    }
}

void SceneRenderer::SetDebugLightCubeMesh(const std::shared_ptr<Mesh>& mesh) const
{
    assert(mesh && "Debug light cube mesh cannot be null");
    assert(mesh->GetVertexArray() && "Debug mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Debug mesh VAO handle must be valid");
    assert(m_Pipeline && "Pipeline must be initialized before setting debug mesh");

    if (m_Pipeline) {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass) {
            debugPass->SetLightCubeMesh(mesh);
        }
    }
}

void SceneRenderer::SetDebugDirectionalRayMesh(const std::shared_ptr<Mesh>& mesh) const
{
    assert(mesh && "Debug directional ray mesh cannot be null");
    assert(mesh->GetVertexArray() && "Debug mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Debug mesh VAO handle must be valid");
    assert(m_Pipeline && "Pipeline must be initialized before setting debug mesh");

    if (m_Pipeline) {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass) {
            debugPass->SetDirectionalRayMesh(mesh);
        }
    }
}

void SceneRenderer::SetDebugAABBWireframeMesh(const std::shared_ptr<Mesh>& mesh) const
{
    assert(mesh && "Debug AABB wireframe mesh cannot be null");
    assert(mesh->GetVertexArray() && "Debug mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Debug mesh VAO handle must be valid");
    assert(m_Pipeline && "Pipeline must be initialized before setting debug mesh");

    if (m_Pipeline) {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass) {
            debugPass->SetAABBWireframeMesh(mesh);
        }
    }
}

void SceneRenderer::SetPickingShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Picking shader cannot be null");
    assert(shader->ID != 0 && "Picking shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting picking shader");

    if (m_Pipeline) {
        auto pickingPass = std::dynamic_pointer_cast<PickingRenderPass>(m_Pipeline->GetPass("PickingPass"));
        if (pickingPass) {
            pickingPass->SetPickingShader(shader);
        }
    }
}

PickingResult SceneRenderer::QueryObjectPicking(const MousePickingQuery& query)
{
    assert(m_Pipeline && "Pipeline must be initialized before querying picking");

    if (m_Pipeline) {
        auto pickingPass = std::dynamic_pointer_cast<PickingRenderPass>(m_Pipeline->GetPass("PickingPass"));
        if (pickingPass && pickingPass->IsEnabled()) {
            // Create temporary context for picking query
            RenderContext context(m_SubmittedRenderables, m_SubmittedLights, m_AmbientLight, m_FrameData, *m_InstancedRenderer, *m_PBRLightingRenderer, *m_ResourceManager, *m_TextureSlotManager);

            return pickingPass->QueryPicking(query, context);
        }
    }

    // Return empty result if picking is not available
    return PickingResult{};
}

void SceneRenderer::EnablePicking(bool enable) const
{
    assert(m_Pipeline && "Pipeline must be initialized before enabling picking");

    if (m_Pipeline) {
        auto pickingPass = std::dynamic_pointer_cast<PickingRenderPass>(m_Pipeline->GetPass("PickingPass"));
        if (pickingPass) {
            pickingPass->SetEnabled(enable);
            m_Pipeline->EnablePass("PickingPass", enable);
        }
    }
}

void SceneRenderer::SetSkyboxCubemap(unsigned int cubemapID)
{
    assert(cubemapID != 0 && "Skybox cubemap ID must be valid");
    assert(m_Pipeline && "Pipeline must be initialized before setting skybox");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetSkyboxCubemap(cubemapID);
        }
    }
}

void SceneRenderer::SetSkyboxShader(const std::shared_ptr<Shader> &shader)
{
    assert(shader && "Skybox shader cannot be null");
    assert(shader->ID != 0 && "Skybox shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting skybox shader");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetSkyboxShader(shader);
        }
    }
}

void SceneRenderer::EnableSkybox(bool enable)
{
    assert(m_Pipeline && "Pipeline must be initialized before enabling skybox");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->EnableSkybox(enable);
        }
    }
}

bool SceneRenderer::IsSkyboxEnabled() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->IsSkyboxEnabled();
        }
    }
    return false;
}
