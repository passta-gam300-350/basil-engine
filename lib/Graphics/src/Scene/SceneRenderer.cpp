/******************************************************************************/
/*!
\file   SceneRenderer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Implementation of the scene rendering orchestration

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Scene/SceneRenderer.h"
#include "Pipeline/MainRenderingPass.h"
#include "Pipeline/DebugRenderPass.h"
#include "Pipeline/OutlineRenderPass.h"
#include "Pipeline/EditorResolvePass.h"
#include "Pipeline/GameResolvePass.h"
#include "Pipeline/RenderTextureResolvePass.h"
#include "Pipeline/PickingRenderPass.h"
#include "Pipeline/HUDRenderPass.h"
#include "Pipeline/RenderContext.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Rendering/HUDRenderer.h"
#include "Rendering/TextRenderer.h"
#include "Rendering/WorldUIRenderer.h"
#include "Pipeline/PresentPass.h"
//#include "Pipeline/ShadowMappingPass.h"
#include "Rendering/ParticleRenderer.h"
#include "Pipeline/DirectionalShadowMappingPass.h"
#include "Pipeline/PointShadowMappingPass.h"
#include "Pipeline/SpotShadowMappingPass.h"
#include "Pipeline/HDRLuminancePass.h"
#include "Pipeline/HDRResolvePass.h"
#include "Pipeline/ToneMapRenderPass.h"
#include "Pipeline/BloomRenderPass.h"
#include "Resources/Shader.h"
#include "Resources/Mesh.h"
#include "Rendering/ParticleRenderer.h"
#include <glfw/glfw3.h>
#include <cassert>
#include <spdlog/spdlog.h>

SceneRenderer::SceneRenderer()
{
    m_ResourceManager = std::make_unique<ResourceManager>();
    assert(m_ResourceManager && "Failed to create ResourceManager");

    m_ResourceManager->Initialize();

    // Initialize texture slot manager
    m_TextureSlotManager = std::make_unique<TextureSlotManager>();
    assert(m_TextureSlotManager && "Failed to create TextureSlotManager");

    // Create unified shadow texture array before initializing pipeline
    CreateShadow2DTextureArray();

    // Initialize rendering coordinators with dependencies
    InitializeRenderingCoordinators();
    InitializeDefaultPipeline();
}

SceneRenderer::~SceneRenderer() {
    // Destroy shadow texture array
    DestroyShadow2DTextureArray();

    // Pipeline and coordinators will be cleaned up by unique_ptr
}

void SceneRenderer::SubmitRenderable(const RenderableData& renderable) {
    m_SubmittedRenderables.push_back(renderable);
}

void SceneRenderer::SubmitParticles(const ParticleRenderData& particleData) {
    if (m_ParticleRenderer) {
        m_ParticleRenderer->SubmitParticleSystem(particleData);
    }
}

void SceneRenderer::SubmitHUDElement(const HUDElementData& hudElement) {
    if (m_HUDRenderer) {
        m_HUDRenderer->SubmitElement(hudElement);
    }
}

void SceneRenderer::SubmitText(const TextElementData& textElement) {
    if (m_TextRenderer) {
        m_TextRenderer->SubmitText(textElement);
    }
}

void SceneRenderer::SubmitWorldText(const WorldTextElementData& worldText) {
    if (m_TextRenderer) {
        m_TextRenderer->SubmitWorldText(worldText);
    }
}

void SceneRenderer::SubmitWorldUI(const WorldUIElementData& worldUI) {
    if (m_WorldUIRenderer) {
        m_WorldUIRenderer->SubmitElement(worldUI);
    }
}

void SceneRenderer::SubmitLight(const SubmittedLightData& light) {
    m_SubmittedLights.push_back(light);
}


void SceneRenderer::ClearFrame()
{
    m_SubmittedRenderables.clear();
	m_SubmittedLights.clear();

    // OPTIMIZATION: Reset change tracking for this frame
    // This allows HasRenderablesChanged() to be called multiple times per frame
    // (shadow pass, main pass, etc.) but only do the expensive check once
    if (m_InstancedRenderer) {
        m_InstancedRenderer->ResetFrameChangeTracking();
    }

    if (m_ParticleRenderer) {
        m_ParticleRenderer->ClearFrame();
    }
    if (m_HUDRenderer) {
        m_HUDRenderer->BeginFrame();
    }
    if (m_TextRenderer) {
        m_TextRenderer->BeginFrame();
    }
    if (m_WorldUIRenderer) {
        m_WorldUIRenderer->BeginFrame();
    }

	// Clear SSBO-based shadow data (will be repopulated by enabled shadow passes)
	m_FrameData.shadowDataArray.clear();
	m_FrameData.shadow2DTextureArray = m_Shadow2DTextureArray;  // Set shared texture array
	m_FrameData.shadowCubemapTextures.clear();

	// Clear physics debug lines for next frame
	m_FrameData.debugLines.clear();

    if (m_ParticleRenderer)
    {
        m_ParticleRenderer->ClearFrame();
    }
}


void SceneRenderer::InitializeDefaultPipeline()
{
    // Create main rendering pipeline
    auto mainPipeline = std::make_unique<RenderPipeline>();

    // 1. Add directional shadow mapping pass
    // Shadow pass will need shader to be set after creation by the application
    auto shadowPass = std::make_shared<DirectionalShadowMappingPass>();
    shadowPass->SetShadowTextureArray(m_Shadow2DTextureArray);  // Pass shared texture array
    mainPipeline->AddPass(shadowPass);
    mainPipeline->EnablePass("DirectionalShadowPass", false);

    // 2. Add point shadow mapping pass (geometry shader method)
    // Point shadow pass will need shader to be set after creation by the application
    auto pointShadowPass = std::make_shared<PointShadowMappingPass>();
    mainPipeline->AddPass(pointShadowPass);
    mainPipeline->EnablePass("PointShadowPass", false);

    // 3. Add spot shadow mapping pass (perspective projection, uses shared texture array)
    // Spot shadow pass will need shader to be set after creation by the application
    auto spotShadowPass = std::make_shared<SpotShadowMappingPass>();
    spotShadowPass->SetShadowTextureArray(m_Shadow2DTextureArray);  // Pass shared texture array
    mainPipeline->AddPass(spotShadowPass);
    mainPipeline->EnablePass("SpotShadowPass", false);  // Disabled by default

    // 4. Add main rendering pass (HDR output - RGB16F with 4x MSAA)
    auto mainPass = std::make_shared<MainRenderingPass>();
    mainPipeline->AddPass(mainPass);

    // 4. Add debug rendering pass (shows light cubes) - BEFORE HDR resolve!
    auto debugPass = std::make_shared<DebugRenderPass>();
    mainPipeline->AddPass(debugPass);

    // 5. Add outline rendering pass (stencil-based outlines) - BEFORE HDR resolve!
    auto outlinePass = std::make_shared<OutlineRenderPass>();
    mainPipeline->AddPass(outlinePass);
    //mainPipeline->EnablePass("OutlinePass", false);  // Enabled by default

    // 6. Add HDR resolve pass (resolve MSAA HDR buffer for tone mapping)
    auto hdrResolvePass = std::make_shared<HDRResolvePass>();
    mainPipeline->AddPass(hdrResolvePass);
    //mainPipeline->EnablePass("HDRResolvePass", false);  // Disabled by default

    // 7. Add HDR luminance pass (auto-exposure calculation via compute shader)
    auto hdrLuminancePass = std::make_shared<HDRLuminancePass>();
    mainPipeline->AddPass(hdrLuminancePass);
    //mainPipeline->EnablePass("HDRLuminancePass", false);  // Disabled by default

    // 8. Add physically based bloom pass (multi-scale blur with Karis average)
    auto bloomPass = std::make_shared<BloomRenderPass>();
    mainPipeline->AddPass(bloomPass);
    //mainPipeline->EnablePass("BloomPass", false);
    //spdlog::info("SceneRenderer: Added BloomRenderPass to pipeline");

    // 9. Add tone mapping pass (HDR → LDR conversion with bloom compositing)
    auto toneMapPass = std::make_shared<ToneMapRenderPass>();
    toneMapPass->SetGamma(2.2f);  // Standard sRGB gamma - outputs RGB8 to avoid ImGui brightness issues
    mainPipeline->AddPass(toneMapPass);
    //mainPipeline->EnablePass("ToneMapPass", true);  // Disabled by default

    // 10. Add HUD rendering pass (renders on top of final tone-mapped image)
    auto hudPass = std::make_shared<HUDRenderPass>();
    mainPipeline->AddPass(hudPass);
    mainPipeline->EnablePass("HUDPass", true);  // Disabled by default, enable when needed

    // 11. Add picking pass (executes when needed, disabled by default)
    auto pickingPass = std::make_shared<PickingRenderPass>();
    mainPipeline->AddPass(pickingPass);
    mainPipeline->EnablePass("PickingPass", false);  // Disabled by default

    // 12. Add editor resolve pass (resolve MSAA editor buffer for ImGui Scene viewport)
    auto editorResolvePass = std::make_shared<EditorResolvePass>();
    mainPipeline->AddPass(editorResolvePass);

    // 13. Add game resolve pass (resolve MSAA game buffer for ImGui Game viewport - always enabled)
    auto gameResolvePass = std::make_shared<GameResolvePass>();
    mainPipeline->AddPass(gameResolvePass);

    // 14. Add render texture resolve pass (captures secondary camera output for HUD/WorldUI use)
    auto renderTextureResolvePass = std::make_shared<RenderTextureResolvePass>();
    mainPipeline->AddPass(renderTextureResolvePass);
    mainPipeline->EnablePass("RenderTextureResolvePass", false);  // Disabled by default, enabled per-frame

    // 15. Add present pass (executes last)
    auto presentPass = std::make_shared<PresentPass>();
    mainPipeline->AddPass(presentPass);

    m_Pipeline = std::move(mainPipeline);
}

void SceneRenderer::InitializeRenderingCoordinators()
{
    // Create rendering coordinators with explicit dependencies
    m_PBRLightingRenderer = std::make_unique<PBRLightingRenderer>();  // Initialize lighting first
    assert(m_PBRLightingRenderer && "Failed to create PBRLightingRenderer");

    m_FrustumCuller = std::make_unique<FrustumCuller>();
    assert(m_FrustumCuller && "Failed to create FrustumCuller");

    m_InstancedRenderer = std::make_unique<InstancedRenderer>(m_PBRLightingRenderer.get());
    assert(m_InstancedRenderer && "Failed to create InstancedRenderer");

    m_ParticleRenderer = std::make_unique<ParticleRenderer>();
    assert(m_ParticleRenderer && "Failed to create ParticleRenderers");

    m_HUDRenderer = std::make_unique<HUDRenderer>();
    assert(m_HUDRenderer && "Failed to create HUDRenderer");

    m_TextRenderer = std::make_unique<TextRenderer>();
    assert(m_TextRenderer && "Failed to create TextRenderer");

    m_WorldUIRenderer = std::make_unique<WorldUIRenderer>();
    assert(m_WorldUIRenderer && "Failed to create WorldUIRenderer");
}

void SceneRenderer::Render()
{
    assert(m_InstancedRenderer && "InstancedRenderer must be initialized before rendering");
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized before rendering");
    assert(m_ResourceManager && "ResourceManager must be initialized before rendering");
    assert(m_TextureSlotManager && "TextureSlotManager must be initialized before rendering");
    assert(m_ParticleRenderer && "ParticleRenderer must be initialized before rendering");
    // Update viewport dimensions in frame data
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (currentWindow) {
        int width, height;
        glfwGetFramebufferSize(currentWindow, &width, &height);

        // Skip rendering if window is minimized (framebuffer size = 0)
        if (width == 0 || height == 0) {
            return;
        }

        m_FrameData.viewportWidth = static_cast<uint32_t>(width);
        m_FrameData.viewportHeight = static_cast<uint32_t>(height);
    }

    // Finalize HUD elements before rendering
    if (m_HUDRenderer) {
        m_HUDRenderer->EndFrame();
    }
    if (m_TextRenderer) {
        m_TextRenderer->EndFrame();
    }
    if (m_WorldUIRenderer) {
        m_WorldUIRenderer->EndFrame();
    }

    // Set fog data pointer in frame data for access by rendering systems
    m_FrameData.fogData = &m_FogData; // currently not used, but is filled with data

    // Create context with references to our data - NO COPYING!
    RenderContext context(
        m_SubmittedRenderables,  // const ref to renderables
        m_SubmittedLights,       // const ref to lights
        m_AmbientLight,          // const ref to ambient light
        m_FogData,               // const ref to fog data
        m_FrameData,             // mutable ref to frame data
        *m_InstancedRenderer,    // ref to instanced renderer
        *m_PBRLightingRenderer,  // ref to PBR lighting
        *m_ResourceManager,      // ref to resource manager
        *m_TextureSlotManager,   // ref to texture slot manager
        *m_ParticleRenderer,     // ref to particle renderer
        *m_HUDRenderer,          // ref to HUD renderer
        *m_TextRenderer,         // ref to text renderer
        *m_WorldUIRenderer       // ref to world UI renderer
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
        auto shadowPass = std::dynamic_pointer_cast<DirectionalShadowMappingPass>(m_Pipeline->GetPass("DirectionalShadowPass"));
        if (shadowPass) {
            shadowPass->SetShadowDepthShader(shader);
        }
    }
}

void SceneRenderer::SetPointShadowShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Point shadow shader cannot be null");
    assert(shader->ID != 0 && "Point shadow shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting point shadow shader");

    if (m_Pipeline) {
        auto pointShadowPass = std::dynamic_pointer_cast<PointShadowMappingPass>(m_Pipeline->GetPass("PointShadowPass"));
        if (pointShadowPass) {
            pointShadowPass->SetPointShadowShader(shader);
        }
    }
}

void SceneRenderer::SetSpotShadowShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Spot shadow shader cannot be null");
    assert(shader->ID != 0 && "Spot shadow shader must be compiled and linked before setting");
    assert(m_Pipeline && "Pipeline must be initialized before setting shaders");

    if (m_Pipeline) {
        auto spotShadowPass = std::dynamic_pointer_cast<SpotShadowMappingPass>(
            m_Pipeline->GetPass("SpotShadowPass")
        );
        if (spotShadowPass) {
            spotShadowPass->SetShadowDepthShader(shader);
            spdlog::info("SceneRenderer: Spot shadow depth shader configured");
        } else {
            spdlog::warn("SceneRenderer: SpotShadowPass not found in pipeline");
        }
    }
}

void SceneRenderer::SetDebugPrimitiveShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Primitive shader cannot be null");
    assert(shader->ID != 0 && "Primitive shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting primitive shader");

    if (m_Pipeline) {
        // Set primitive shader on MainRenderingPass (for light cubes)
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass) {
            mainPass->SetPrimitiveShader(shader);
        }
    }
}

void SceneRenderer::SetDebugLineShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Debug line shader cannot be null");
    assert(shader->ID != 0 && "Debug line shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting debug line shader");

    if (m_Pipeline) {
        // Set debug line shader on DebugRenderPass (for physics debug visualization)
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass) {
            debugPass->SetDebugLineShader(shader);
            spdlog::info("SceneRenderer: Debug line shader set on DebugRenderPass");
        } else {
            spdlog::warn("SceneRenderer: DebugPass not found in pipeline");
        }
    }
}

void SceneRenderer::SetDebugLightCubeMesh(const std::shared_ptr<Mesh>& mesh) const
{
    assert(mesh && "Light cube mesh cannot be null");
    assert(mesh->GetVertexArray() && "Light cube mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Light cube mesh VAO handle must be valid");
    assert(m_Pipeline && "Pipeline must be initialized before setting light cube mesh");

    if (m_Pipeline) {
        // Set light cube mesh on MainRenderingPass (light cubes are now rendered in main pass)
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass) {
            mainPass->SetLightCubeMesh(mesh);
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
            RenderContext context(m_SubmittedRenderables, m_SubmittedLights, m_AmbientLight, m_FogData, m_FrameData, *m_InstancedRenderer, *m_PBRLightingRenderer, *m_ResourceManager, *m_TextureSlotManager, *m_ParticleRenderer, *m_HUDRenderer, *m_TextRenderer, *m_WorldUIRenderer);

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

void SceneRenderer::SetSkyboxExposure(float exposure)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting skybox exposure");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetSkyboxExposure(exposure);
        }
    }
}

void SceneRenderer::SetSkyboxRotation(const glm::vec3& rotation)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting skybox rotation");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetSkyboxRotation(rotation);
        }
    }
}

void SceneRenderer::SetSkyboxTint(const glm::vec3& tint)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting skybox tint");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetSkyboxTint(tint);
        }
    }
}

float SceneRenderer::GetSkyboxExposure() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetSkyboxExposure();
        }
    }
    return 1.0f;
}

glm::vec3 SceneRenderer::GetSkyboxRotation() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetSkyboxRotation();
        }
    }
    return glm::vec3(0.0f);
}

glm::vec3 SceneRenderer::GetSkyboxTint() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetSkyboxTint();
        }
    }
    return glm::vec3(1.0f);
}

void SceneRenderer::SetBackgroundColor(const glm::vec4& color)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting background color");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetClearColor(color);
        }
    }
}

glm::vec4 SceneRenderer::GetBackgroundColor() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetClearColor();
        }
    }
    return glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);  // Default gray
}

void SceneRenderer::SetDefaultLightCubeSize(float size)
{
    assert(size > 0.0f && "Light cube size must be positive");
    assert(m_Pipeline && "Pipeline must be initialized before setting light cube size");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetBaseLightCubeSize(size);
        }
    }
}

float SceneRenderer::GetDefaultLightCubeSize() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetBaseLightCubeSize();
        }
    }
    return 0.3f;  // Default value
}

void SceneRenderer::SetShowLightCubes(bool show)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting light cube visibility");

    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            mainPass->SetShowLightCubes(show);
        }
    }
}

bool SceneRenderer::GetShowLightCubes() const
{
    if (m_Pipeline)
    {
        auto mainPass = std::dynamic_pointer_cast<MainRenderingPass>(m_Pipeline->GetPass("MainPass"));
        if (mainPass)
        {
            return mainPass->GetShowLightCubes();
        }
    }
    return true;  // Default: visible
}

void SceneRenderer::SetHDRComputeShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "HDR compute shader cannot be null");
    assert(shader->ID != 0 && "HDR compute shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting HDR compute shader");

    if (m_Pipeline)
    {
        auto hdrLumPass = std::dynamic_pointer_cast<HDRLuminancePass>(m_Pipeline->GetPass("HDRLuminancePass"));
        if (hdrLumPass)
        {
            hdrLumPass->SetComputeShader(shader);
        }
    }
}

void SceneRenderer::SetToneMappingShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Tone mapping shader cannot be null");
    assert(shader->ID != 0 && "Tone mapping shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting tone mapping shader");

    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            toneMapPass->SetToneMappingShader(shader);
        }
    }
}

//void SceneRenderer::SetEditorResolveShader(const std::shared_ptr<Shader>& shader) const
//{
//    assert(shader && "Editor resolve shader cannot be null");
//    assert(shader->ID != 0 && "Editor resolve shader must be compiled and linked");
//    assert(m_Pipeline && "Pipeline must be initialized before setting editor resolve shader");
//
//    if (m_Pipeline)
//    {
//        auto editorResolvePass = std::dynamic_pointer_cast<EditorResolvePass>(m_Pipeline->GetPass("EditorResolvePass"));
//        if (editorResolvePass)
//        {
//            editorResolvePass->SetConversionShader(shader);
//        }
//    }
//}

// ===== FACADE METHODS FOR DECOUPLING =====
void SceneRenderer::ToggleRenderPass(const std::string& passName)
{
    assert(m_Pipeline && "Pipeline must be initialized before toggling render passes");

    if (m_Pipeline)
    {
        bool currentlyEnabled = m_Pipeline->IsPassEnabled(passName);
        bool newState = !currentlyEnabled;
        m_Pipeline->EnablePass(passName, newState);
        spdlog::info("SceneRenderer: Toggled render pass '{}' to {}",
                     passName, newState ? "enabled" : "disabled");
    }
}

void SceneRenderer::SetShadowIntensity(float directional, float point, float spot)
{
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized");

    if (m_PBRLightingRenderer)
    {
        m_PBRLightingRenderer->SetShadowIntensity(directional, point);
        m_PBRLightingRenderer->SetSpotShadowIntensity(spot);
        spdlog::debug("SceneRenderer: Set shadow intensities (dir: {}, point: {}, spot: {})",
                     directional, point, spot);
    }
}

void SceneRenderer::ClearInstanceCache()
{
    assert(m_InstancedRenderer && "InstancedRenderer must be initialized");

    if (m_InstancedRenderer)
    {
        m_InstancedRenderer->Clear();
        spdlog::debug("SceneRenderer: Cleared instance cache for animation");
    }
}

void SceneRenderer::ForceRebuildInstanceCache()
{
    if (m_InstancedRenderer)
    {
        m_InstancedRenderer->ForceRebuildCache();
    }
}

void SceneRenderer::SetShadowFilterSize(int filterSize)
{
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized");

    if (m_PBRLightingRenderer) {
        m_PBRLightingRenderer->SetShadowFilterSize(filterSize);
        spdlog::info("SceneRenderer: Shadow filter size set to {}x{} ({} samples)",
                     filterSize, filterSize, filterSize * filterSize);
    }
}

void SceneRenderer::SetShadowRandomRadius(float radius)
{
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized");

    if (m_PBRLightingRenderer) {
        m_PBRLightingRenderer->SetShadowRandomRadius(radius);
        spdlog::info("SceneRenderer: Shadow softness (random radius) set to {}", radius);
    }
}

int SceneRenderer::GetShadowFilterSize() const
{
    if (m_PBRLightingRenderer) {
        return m_PBRLightingRenderer->GetShadowFilterSize();
    }
    return 0;
}

float SceneRenderer::GetShadowRandomRadius() const
{
    if (m_PBRLightingRenderer) {
        return m_PBRLightingRenderer->GetShadowRandomRadius();
    }
    return 0.0f;
}

void SceneRenderer::SetCameraData(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos)
{
    m_FrameData.viewMatrix = view;
    m_FrameData.projectionMatrix = proj;
    m_FrameData.cameraPosition = pos;
}

// ===== Fog Control (OGLDev Tutorial 39-style) =====

void SceneRenderer::SetLinearFog(float start, float end, const glm::vec3& color)
{
    m_FogData.type = FogType::Linear;
    m_FogData.start = start;
    m_FogData.end = end;
    m_FogData.color = color;
}

void SceneRenderer::SetExpFog(float end, float density, const glm::vec3& color)
{
    m_FogData.type = FogType::Exponential;
    m_FogData.end = end;
    m_FogData.density = density;
    m_FogData.color = color;
}

void SceneRenderer::SetExpSquaredFog(float end, float density, const glm::vec3& color)
{
    m_FogData.type = FogType::ExponentialSquared;
    m_FogData.end = end;
    m_FogData.density = density;
    m_FogData.color = color;
}

void SceneRenderer::DisableFog()
{
    m_FogData.Disable();
}

void SceneRenderer::EnablePhysicsDebugVisualization(bool enable)
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass)
        {
            debugPass->SetShowPhysicsDebug(enable);
            spdlog::info("SceneRenderer: Physics debug line visualization {}",
                         enable ? "ENABLED" : "DISABLED");
        }
        else
        {
            spdlog::warn("SceneRenderer: Debug pass not found - cannot set physics debug visualization");
        }
    }
}

bool SceneRenderer::IsPassEnabled(const std::string& passName) const
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        return m_Pipeline->IsPassEnabled(passName);
    }
    return false;
}

void SceneRenderer::EnablePass(const std::string& passName, bool enable)
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        m_Pipeline->EnablePass(passName, enable);
        spdlog::debug("SceneRenderer: Pass '{}' {}",
                     passName, enable ? "enabled" : "disabled");
    }
}

void SceneRenderer::ToggleHDRPipeline(bool enable)
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        m_Pipeline->EnablePass("HDRResolvePass", enable);
        m_Pipeline->EnablePass("HDRLuminancePass", enable);
        m_Pipeline->EnablePass("ToneMapPass", enable);
        spdlog::info("SceneRenderer: HDR tone mapping pipeline {}",
                     enable ? "enabled" : "disabled");
    }
}

// ===== OUTLINE RENDERING METHODS =====
void SceneRenderer::SetOutlineShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Outline shader cannot be null");
    assert(shader->ID != 0 && "Outline shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting outline shader");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->SetOutlineShader(shader);
        }
    }
}

void SceneRenderer::SetParticleShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Particle shader cannot be null");
    assert(shader->ID != 0 && "Particle shader must be compiled and linked");

    if (m_ParticleRenderer) 
    {
        m_ParticleRenderer->SetParticleShader(shader);
    }
}

void SceneRenderer::SetHUDShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "HUD shader cannot be null");
    assert(shader->ID != 0 && "HUD shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting HUD shader");

    if (m_Pipeline)
    {
        auto hudPass = std::dynamic_pointer_cast<HUDRenderPass>(m_Pipeline->GetPass("HUDPass"));
        if (hudPass)
        {
            hudPass->SetHUDShader(shader);
            // Also set shader in renderer
            if (m_HUDRenderer)
            {
                m_HUDRenderer->SetHUDShader(shader);
            }
            spdlog::info("SceneRenderer: HUD shader configured");
        }
    }
}

void SceneRenderer::SetTextShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "Text shader cannot be null");
    assert(shader->ID != 0 && "Text shader must be compiled and linked");

    if (m_TextRenderer)
    {
        m_TextRenderer->SetTextShader(shader);
        spdlog::info("SceneRenderer: Text shader configured");
    }
}

void SceneRenderer::SetWorldTextShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "World text shader cannot be null");
    assert(shader->ID != 0 && "World text shader must be compiled and linked");

    if (m_TextRenderer)
    {
        m_TextRenderer->SetWorldTextShader(shader);
        spdlog::info("SceneRenderer: World text shader configured");
    }
}

void SceneRenderer::SetWorldUIShader(const std::shared_ptr<Shader>& shader) const
{
    assert(shader && "World UI shader cannot be null");
    assert(shader->ID != 0 && "World UI shader must be compiled and linked");

    if (m_WorldUIRenderer)
    {
        m_WorldUIRenderer->SetWorldUIShader(shader);
        spdlog::info("SceneRenderer: World UI shader configured");
    }
}

void SceneRenderer::AddOutlinedObject(uint32_t objectID) const
{
    assert(m_Pipeline && "Pipeline must be initialized before adding outlined object");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->AddOutlinedObject(objectID);
        }
    }
}

void SceneRenderer::RemoveOutlinedObject(uint32_t objectID) const
{
    assert(m_Pipeline && "Pipeline must be initialized before removing outlined object");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->RemoveOutlinedObject(objectID);
        }
    }
}

void SceneRenderer::ClearOutlinedObjects() const
{
    assert(m_Pipeline && "Pipeline must be initialized before clearing outlined objects");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->ClearOutlinedObjects();
        }
    }
}

void SceneRenderer::SetOutlineColor(const glm::vec3& color) const
{
    assert(m_Pipeline && "Pipeline must be initialized before setting outline color");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->SetOutlineColor(color);
        }
    }
}

void SceneRenderer::SetOutlineScale(float scale) const
{
    assert(m_Pipeline && "Pipeline must be initialized before setting outline scale");
    assert(scale > 1.0f && "Outline scale must be greater than 1.0");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->SetOutlineScale(scale);
        }
    }
}

void SceneRenderer::EnableOutlineRendering(bool enable) const
{
    assert(m_Pipeline && "Pipeline must be initialized before enabling outline rendering");

    if (m_Pipeline)
    {
        auto outlinePass = std::dynamic_pointer_cast<OutlineRenderPass>(m_Pipeline->GetPass("OutlinePass"));
        if (outlinePass)
        {
            outlinePass->SetEnabled(enable);
            m_Pipeline->EnablePass("OutlinePass", enable);
        }
    }
}

void SceneRenderer::SetBloomStrength(float strength)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting bloom strength");

    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            toneMapPass->SetBloomStrength(strength);
        }
    }
}

float SceneRenderer::GetBloomStrength() const
{
    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            return toneMapPass->GetBloomStrength();
        }
    }
    return 0.04f; // Default value
}

void SceneRenderer::EnableBloom(bool enable)
{
    assert(m_Pipeline && "Pipeline must be initialized before enabling bloom");

    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            toneMapPass->EnableBloom(enable);
        }
    }
}

bool SceneRenderer::IsBloomEnabled() const
{
    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            return toneMapPass->IsBloomEnabled();
        }
    }
    return true; // Default value
}

void SceneRenderer::SetToneMappingMethod(int method)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting tone mapping method");

    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            toneMapPass->SetMethod(static_cast<ToneMapRenderPass::Method>(method));
        }
    }
}

int SceneRenderer::GetToneMappingMethod() const
{
    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            return static_cast<int>(toneMapPass->GetMethod());
        }
    }
    return 2; // Default to ACES
}

void SceneRenderer::SetExposureClampRange(float minExposure, float maxExposure)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting exposure clamp range");

    if (m_Pipeline)
    {
        auto hdrLuminancePass = std::dynamic_pointer_cast<HDRLuminancePass>(m_Pipeline->GetPass("HDRLuminancePass"));
        if (hdrLuminancePass)
        {
            hdrLuminancePass->SetExposureClampRange(minExposure, maxExposure);
        }
    }
}

void SceneRenderer::GetExposureClampRange(float& outMin, float& outMax) const
{
    if (m_Pipeline)
    {
        auto hdrLuminancePass = std::dynamic_pointer_cast<HDRLuminancePass>(m_Pipeline->GetPass("HDRLuminancePass"));
        if (hdrLuminancePass)
        {
            hdrLuminancePass->GetExposureClampRange(outMin, outMax);
            return;
        }
    }
    // Default values
    outMin = 0.1f;
    outMax = 2.0f;
}

void SceneRenderer::SetGamma(float gamma)
{
    assert(m_Pipeline && "Pipeline must be initialized before setting gamma");

    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            toneMapPass->SetGamma(gamma);
        }
    }
}

float SceneRenderer::GetGamma() const
{
    if (m_Pipeline)
    {
        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(m_Pipeline->GetPass("ToneMapPass"));
        if (toneMapPass)
        {
            return toneMapPass->GetGamma();
        }
    }
    return 2.2f; // Default standard sRGB gamma
}

// ===== SHADOW TEXTURE ARRAY MANAGEMENT =====
void SceneRenderer::CreateShadow2DTextureArray()
{
    // Create layered 2D texture array for directional and spot shadows
    glGenTextures(1, &m_Shadow2DTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Shadow2DTextureArray);

    // Allocate storage for all layers (GL_TEXTURE_3D-style allocation)
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,                          // Mipmap level
        GL_DEPTH_COMPONENT24,       // Internal format (24-bit depth)
        SHADOW_MAP_SIZE,            // Width
        SHADOW_MAP_SIZE,            // Height
        SHADOW_ARRAY_LAYERS,        // Number of layers
        0,                          // Border (must be 0)
        GL_DEPTH_COMPONENT,         // Format
        GL_FLOAT,                   // Type
        nullptr                     // Data (null = allocate but don't fill)
    );

    // Set texture parameters (same as regular shadow maps)
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set border color to maximum depth (1.0) so fragments outside shadow map are not in shadow
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Hardware PCF disabled for better performance on some GPUs
    // (Software PCF is still active via shader-based adaptive sampling)
    // glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    // glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    spdlog::info("SceneRenderer: Created unified 2D shadow texture array ({} layers, {}x{})",
                 SHADOW_ARRAY_LAYERS, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
}

void SceneRenderer::DestroyShadow2DTextureArray()
{
    if (m_Shadow2DTextureArray != 0)
    {
        glDeleteTextures(1, &m_Shadow2DTextureArray);
        m_Shadow2DTextureArray = 0;
        spdlog::info("SceneRenderer: Destroyed unified 2D shadow texture array");
    }
}
