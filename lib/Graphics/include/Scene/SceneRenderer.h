#pragma once

#include "../Pipeline/RenderPipeline.h"
#include "../Pipeline/MainRenderingPipeline.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include "../Core/Renderer.h"
#include "../Resources/ResourceManager.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

// Forward declarations for rendering coordinators
class MeshRenderer;
class FrustumCuller;
class InstancedRenderer;
class PBRLightingRenderer;

class SceneRenderer {
public:
    SceneRenderer(GLFWwindow* window);
    ~SceneRenderer();
    
    // Data submission API - application pushes data each frame
    void SubmitRenderable(const RenderableData& renderable);
    void SubmitLight(const SubmittedLightData& light);
    void SetAmbientLight(const glm::vec3& ambient) { m_AmbientLight = ambient; }
    
    // Clear submitted data (call at start of frame)
    void ClearFrame();

    // High-level scene rendering coordination
    void Render();

    // Multi pipeline support
    void AddPipeline(std::string const &name, std::unique_ptr<RenderPipeline> pipeline);
    void RemovePipeline(std::string const &name);
    void SetPipelineOrder(std::vector<std::string> const &order);
    RenderPipeline *GetPipeline(std::string const &name);

    // Pipeline state mgmt
    void EnablePipeline(std::string const &name, bool enabled = true);
    bool IsPipelineEnabled(std::string const &name) const;

    //Access to frame data for custom pipeline setup
    FrameData &GetFrameData()
    {
        return m_FrameData;
    }
    
    // Access to rendering coordinators for advanced usage
    InstancedRenderer* GetInstancedRenderer() const { return m_InstancedRenderer.get(); }
    PBRLightingRenderer* GetPBRLightingRenderer() const { return m_PBRLightingRenderer.get(); }
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }

private:
    //void InitializePipeline();
    void InitializeRenderingCoordinators();

    // Multi pipelines methods
    void InitializeDefaultPipelines();
    
    void UpdateFrameData()
    {
        // Camera data is now set directly by applications via GetFrameData()
        // This method can be used for other frame-level updates if needed
    }

    // Frame-submitted data
    std::vector<RenderableData> m_SubmittedRenderables;
    std::vector<SubmittedLightData> m_SubmittedLights;
    glm::vec3 m_AmbientLight = glm::vec3(0.1f);
    

    // NEW: Multiple pipelines with order
    std::unordered_map<std::string, std::unique_ptr<RenderPipeline>> m_Pipelines;
    std::vector<std::string> m_PipelineOrder;
    std::unordered_map<std::string, bool> m_PipelineEnabled;

    // Shared frame data
    FrameData m_FrameData;

    // Core systems - SceneRenderer owns these
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ResourceManager> m_ResourceManager;

    // Rendering coordinators - SceneRenderer owns these
    std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
    std::unique_ptr<InstancedRenderer> m_InstancedRenderer;
    std::unique_ptr<PBRLightingRenderer> m_PBRLightingRenderer;
    
};