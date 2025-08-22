#pragma once

#include <Core/Window.h>
#include <Core/Renderer.h>
#include <Resources/ResourceManager.h>
#include <Scene/Scene.h>
#include <Scene/SceneRenderer.h>
#include <ECS/Systems/RenderSystem.h>
#include <ECS/Systems/CullingSystem.h>
#include <Utility/Camera.h>
#include <memory>
#include <chrono>

class Application
{
public:
    Application(std::string const &name = "Graphics Engine", uint32_t width = 1280, uint32_t height = 720);
    virtual ~Application();

    // Main engine loop
    void Run();

    // Override these in derived classes for custom behavior
    virtual void Initialize()
    {
    }
    virtual void LoadResources()
    {
    }
    virtual void Update(float deltaTime)
    {
    }
    virtual void Render()
    {
    }
    virtual void OnEvent(/* Event& event */)
    {
    } // For future input system
    virtual void Shutdown()
    {
    }

protected:
    // Access to core engine systems
    Window &GetWindow()
    {
        return *m_Window;
    }
    Renderer &GetRenderer()
    {
        return *m_Renderer;
    }
    ResourceManager &GetResourceManager()
    {
        return *m_ResourceManager;
    }
    Scene &GetScene()
    {
        return *m_CurrentScene;
    }
    SceneRenderer &GetSceneRenderer()
    {
        return *m_SceneRenderer;
    }

    // Utility methods
    void SetActiveScene(std::shared_ptr<Scene> scene);
    void SetActiveCamera(std::shared_ptr<Camera> camera);

private:
    // Core engine initialization
    void InitializeEngine();
    void ShutdownEngine();
    void CalculateDeltaTime();

    // Core systems
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;

    // ECS Systems
    std::unique_ptr<RenderSystem> m_RenderSystem;
    std::unique_ptr<CullingSystem> m_CullingSystem;

    // Scene management
    std::shared_ptr<Scene> m_CurrentScene;
    std::shared_ptr<Camera> m_ActiveCamera;

    // Application state
    bool m_Running = true;
    std::string m_ApplicationName;

    // Timing
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime = 0.0f;
    float m_FrameTime = 0.0f;
    uint32_t m_FrameCount = 0;
    float m_FPSTimer = 0.0f;
};