#pragma once

#include "../../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include "../Pipeline/RenderPipeline.h"
#include "../Utility/Camera.h"
#include <memory>

// Forward declarations for rendering coordinators
class MeshRenderer;
class FrustumCuller;

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    void SetScene(const std::shared_ptr<Scene>& scene) { m_Scene = scene; }
    void SetCamera(const std::shared_ptr<Camera>& camera) { m_Camera = camera; }

    // High-level scene rendering coordination
    void Render();

private:
    void InitializePipeline();
    void InitializeRenderingCoordinators();

    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Camera> m_Camera;
    std::unique_ptr<RenderPipeline> m_Pipeline;
    
    // Rendering coordinators - SceneRenderer owns these (graphics-specific)
    std::unique_ptr<MeshRenderer> m_MeshRenderer;
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
};