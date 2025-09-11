#pragma once

#include "../../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include "../Pipeline/RenderPipeline.h"
#include "../Utility/Camera.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "Resources/Shader.h"

struct FrameData
{
    // Shadow mapping data
    std::vector<std::shared_ptr<FrameBuffer>> shadowMaps;
    std::vector<glm::mat4> shadowMatrices;


    // Main rendering output
    std::shared_ptr<FrameBuffer> mainColorBuffer;

    // Post-processing chain
    std::shared_ptr<FrameBuffer> postProcessBuffer;

    // Camera data (shared across all pipelines)
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f);

    // Timing data
    float deltaTime = 0.0f;
    uint32_t frameNumber = 0;
};

// Forward declarations for rendering coordinators
class MeshRenderer;
class FrustumCuller;
class InstancedRenderer;
class PBRLightingRenderer;

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();
    

    void SetScene(const std::shared_ptr<Scene>& scene) { m_Scene = scene; }
    void SetCamera(const std::shared_ptr<Camera>& camera) { m_Camera = camera; }

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

private:
    //void InitializePipeline();
    void InitializeRenderingCoordinators();

    // Multi pipelines methods
    void InitializeDefaultPipelines();
    
    void UpdateFrameData()
    {
        m_FrameData.viewMatrix = m_Camera->GetViewMatrix();
        m_FrameData.projectionMatrix = m_Camera->GetProjectionMatrix();
        m_FrameData.cameraPosition = m_Camera->GetPosition();
    }

    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Camera> m_Camera;

    // NEW: Multiple pipelines with order
    std::unordered_map<std::string, std::unique_ptr<RenderPipeline>> m_Pipelines;
    std::vector<std::string> m_PipelineOrder;
    std::unordered_map<std::string, bool> m_PipelineEnabled;

    // Shared frame data
    FrameData m_FrameData;
    
    // Rendering coordinators - SceneRenderer owns these
    std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
    std::unique_ptr<InstancedRenderer> m_InstancedRenderer;
    std::unique_ptr<PBRLightingRenderer> m_PBRLightingRenderer;
    
};