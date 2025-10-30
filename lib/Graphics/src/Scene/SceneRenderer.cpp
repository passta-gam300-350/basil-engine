#include "Scene/SceneRenderer.h"
#include "Pipeline/MainRenderingPass.h"
#include "Pipeline/DebugRenderPass.h"
#include "Pipeline/OutlineRenderPass.h"
#include "Pipeline/EditorResolvePass.h"
#include "Pipeline/PickingRenderPass.h"
#include "Pipeline/RenderContext.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Pipeline/PresentPass.h"
#include "Pipeline/DirectionalShadowMappingPass.h"
#include "Pipeline/PointShadowMappingPass.h"
#include "Pipeline/SpotShadowMappingPass.h"
#include "Pipeline/HDRLuminancePass.h"
#include "Pipeline/HDRResolvePass.h"
#include "Pipeline/ToneMapRenderPass.h"
#include "Pipeline/BloomRenderPass.h"
#include "Resources/Shader.h"
#include "Resources/Mesh.h"
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

	// Clear SSBO-based shadow data (will be repopulated by enabled shadow passes)
	m_FrameData.shadowDataArray.clear();
	m_FrameData.shadow2DTextures.clear();
	m_FrameData.shadowCubemapTextures.clear();
}


void SceneRenderer::InitializeDefaultPipeline()
{
    // Create main rendering pipeline
    auto mainPipeline = std::make_unique<RenderPipeline>();

    // 1. Add directional shadow mapping pass
    // Shadow pass will need shader to be set after creation by the application
    auto shadowPass = std::make_shared<DirectionalShadowMappingPass>();
    mainPipeline->AddPass(shadowPass);
    //mainPipeline->EnablePass("DirectionalShadowPass", false);

    // 2. Add point shadow mapping pass (geometry shader method)
    // Point shadow pass will need shader to be set after creation by the application
    auto pointShadowPass = std::make_shared<PointShadowMappingPass>();
    mainPipeline->AddPass(pointShadowPass);
    //mainPipeline->EnablePass("PointShadowPass", false);

    // 3. Add spot shadow mapping pass (perspective projection, reuses directional shader)
    // Spot shadow pass will need shader to be set after creation by the application
    auto spotShadowPass = std::make_shared<SpotShadowMappingPass>();
    mainPipeline->AddPass(spotShadowPass);
    //mainPipeline->EnablePass("SpotShadowPass", false);  // Disabled by default

    // 4. Add main rendering pass (HDR output - RGB16F with 4x MSAA)
    auto mainPass = std::make_shared<MainRenderingPass>();
    mainPipeline->AddPass(mainPass);

    // 4. Add debug rendering pass (shows light cubes) - BEFORE HDR resolve!
    auto debugPass = std::make_shared<DebugRenderPass>();
    mainPipeline->AddPass(debugPass);
    mainPipeline->EnablePass("DebugPass", false);  // Enabled to visualize lights

    // 5. Add outline rendering pass (stencil-based outlines) - BEFORE HDR resolve!
    auto outlinePass = std::make_shared<OutlineRenderPass>();
    mainPipeline->AddPass(outlinePass);
    //mainPipeline->EnablePass("OutlinePass", false);  // Enabled by default

    // 6. Add HDR resolve pass (resolve MSAA HDR buffer for tone mapping)
    auto hdrResolvePass = std::make_shared<HDRResolvePass>();
    mainPipeline->AddPass(hdrResolvePass);
    //mainPipeline->EnablePass("HDRResolvePass", true);  // Disabled by default

    // 7. Add HDR luminance pass (auto-exposure calculation via compute shader)
    auto hdrLuminancePass = std::make_shared<HDRLuminancePass>();
    mainPipeline->AddPass(hdrLuminancePass);
    //mainPipeline->EnablePass("HDRLuminancePass", false);  // Disabled by default

    // 8. Add physically based bloom pass (multi-scale blur with Karis average)
    auto bloomPass = std::make_shared<BloomRenderPass>();
    mainPipeline->AddPass(bloomPass);
    //spdlog::info("SceneRenderer: Added BloomRenderPass to pipeline");

    // 9. Add tone mapping pass (HDR → LDR conversion with bloom compositing)
    auto toneMapPass = std::make_shared<ToneMapRenderPass>();
    toneMapPass->EnableGammaCorrection(false);  // Disable manual gamma - ToneMapPass uses SRGB8 format for hardware gamma via GL_FRAMEBUFFER_SRGB
    mainPipeline->AddPass(toneMapPass);
    //mainPipeline->EnablePass("ToneMapPass", false);  // Disabled by default

    // 10. Add editor resolve pass (resolve MSAA editor buffer for ImGui)
    auto editorResolvePass = std::make_shared<EditorResolvePass>();
    mainPipeline->AddPass(editorResolvePass);

    // 11. Add picking pass (executes when needed, disabled by default)
    auto pickingPass = std::make_shared<PickingRenderPass>();
    mainPipeline->AddPass(pickingPass);
    //mainPipeline->EnablePass("PickingPass", false);  // Disabled by default

    // 12. Add present pass (executes last)
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
}

void SceneRenderer::Render()
{
    assert(m_InstancedRenderer && "InstancedRenderer must be initialized before rendering");
    assert(m_PBRLightingRenderer && "PBRLightingRenderer must be initialized before rendering");
    assert(m_ResourceManager && "ResourceManager must be initialized before rendering");
    assert(m_TextureSlotManager && "TextureSlotManager must be initialized before rendering");

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

void SceneRenderer::SetDebugAABBs(const std::vector<DebugAABB>& aabbs)
{
    m_FrameData.debugAABBs = aabbs;
}

void SceneRenderer::ToggleAABBVisualization()
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass)
        {
            bool newState = !debugPass->GetShowAABBs();
            debugPass->SetShowAABBs(newState);
            spdlog::info("SceneRenderer: AABB wireframe visualization {}",
                         newState ? "ENABLED" : "DISABLED");
        }
        else
        {
            spdlog::warn("SceneRenderer: Debug pass not found - cannot toggle AABB visualization");
        }
    }
}

void SceneRenderer::SetAABBVisualization(bool show)
{
    assert(m_Pipeline && "Pipeline must be initialized");

    if (m_Pipeline)
    {
        auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(m_Pipeline->GetPass("DebugPass"));
        if (debugPass)
        {
            debugPass->SetShowAABBs(show);
            spdlog::info("SceneRenderer: AABB wireframe visualization set to {}",
                         show ? "ENABLED" : "DISABLED");
        }
        else
        {
            spdlog::warn("SceneRenderer: Debug pass not found - cannot set AABB visualization");
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
