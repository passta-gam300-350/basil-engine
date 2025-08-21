#pragma once

#include "Scene.h"
#include "../Pipeline/RenderPipeline.h"
#include "../Utility/Camera.h"
#include <memory>

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    void SetScene(const std::shared_ptr<Scene>& scene) { m_Scene = scene; }
    void SetCamera(const std::shared_ptr<Camera>& camera) { m_Camera = camera; }

    void Render();

private:
    void InitializePipeline();

    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Camera> m_Camera;
    std::unique_ptr<RenderPipeline> m_Pipeline;
};