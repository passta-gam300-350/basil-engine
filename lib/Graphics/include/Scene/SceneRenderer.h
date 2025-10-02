#pragma once

#include "../Pipeline/RenderPipeline.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include "../Resources/ResourceManager.h"
#include "../Resources/TextureSlotManager.h"
#include <memory>
#include <vector>

// Forward declarations for rendering coordinators
//class MeshRenderer;
class FrustumCuller;
class InstancedRenderer;
class PBRLightingRenderer;
class PickingRenderPass;

class SceneRenderer {
public:
    SceneRenderer();
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer& operator=(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator=(SceneRenderer&&) = delete;
    ~SceneRenderer();
    
    // Data submission API - application pushes data each frame
    void SubmitRenderable(const RenderableData& renderable);
    void SubmitLight(const SubmittedLightData& light);
    void SetAmbientLight(const glm::vec3& ambient) { m_AmbientLight = ambient; }
    
    // Clear submitted data (call at start of frame)
    void ClearFrame();

    // High-level scene rendering coordination
    void Render();

    // Pipeline access
    RenderPipeline* GetPipeline() const { return m_Pipeline.get(); }

    //Access to frame data for custom pipeline setup
    FrameData &GetFrameData()
    {
        return m_FrameData;
    }
    
    // Access to rendering coordinators for advanced usage
    InstancedRenderer* GetInstancedRenderer() const { return m_InstancedRenderer.get(); }
    PBRLightingRenderer* GetPBRLightingRenderer() const { return m_PBRLightingRenderer.get(); }
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
    TextureSlotManager* GetTextureSlotManager() const { return m_TextureSlotManager.get(); }

    // Configuration methods for application layer
    void SetShadowDepthShader(const std::shared_ptr<Shader>& shader) const;
    void SetDebugPrimitiveShader(const std::shared_ptr<Shader>& shader) const;
    void SetDebugLightCubeMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetDebugDirectionalRayMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetDebugAABBWireframeMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetPickingShader(const std::shared_ptr<Shader>& shader) const;

    // Picking functionality
    PickingResult QueryObjectPicking(const MousePickingQuery& query);
    void EnablePicking(bool enable) const;

    void SetSkyboxCubemap(unsigned int cubemapID);
    void SetSkyboxShader(const std::shared_ptr<Shader> &shader);
    void EnableSkybox(bool enable);
    bool IsSkyboxEnabled() const;

private:
    //void InitializePipeline();
    void InitializeRenderingCoordinators();

    void InitializeDefaultPipeline();

    // Frame-submitted data
    std::vector<RenderableData> m_SubmittedRenderables;
    std::vector<SubmittedLightData> m_SubmittedLights;
    glm::vec3 m_AmbientLight = glm::vec3(0.1f);

    // Single render pipeline
    std::unique_ptr<RenderPipeline> m_Pipeline;

    // Shared frame data
    FrameData m_FrameData;

    // Core systems - SceneRenderer owns these
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<TextureSlotManager> m_TextureSlotManager;

    // Rendering coordinators - SceneRenderer owns these
    //std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
    std::unique_ptr<InstancedRenderer> m_InstancedRenderer;
    std::unique_ptr<PBRLightingRenderer> m_PBRLightingRenderer;
    
};