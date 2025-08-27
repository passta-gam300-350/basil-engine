#pragma once

#include <Core/Window.h>
#include <Core/Renderer.h>
#include <Resources/ResourceManager.h>
#include "Engine/Scene/Scene.h"
#include <Scene/SceneRenderer.h>
// Old hybrid systems removed - now using pure ECS + rendering coordinators
#include <Utility/Camera.h>
#include <memory>
#include <chrono>
#include <GLFW/glfw3.h>

// Forward declaration for component accessor
class EngineComponentAccessor;

class Application
{
public:
    Application(const std::string& name = "Graphics Engine", uint32_t width = 1280, uint32_t height = 720);
    virtual ~Application();

    // Main engine loop
    void Run();

    // Override these in derived classes for custom behavior
    virtual void Initialize() {}
    virtual void LoadResources() {}
    virtual void Update(float deltaTime) {}
    virtual void Render() {}
    virtual void OnKeyboard(int key, int action, float deltaTime) {}
    virtual void OnMouse(double xpos, double ypos) {}
    virtual void OnMouseScroll(double yoffset) {}
    virtual void Shutdown() {}

protected:
    // Access to core engine systems - clean graphics lib interface
    ResourceManager& GetResourceManager() { return *m_ResourceManager; }
    Scene& GetScene() { return *m_CurrentScene; }
    Camera& GetCamera() { return *m_ActiveCamera; }
    SceneRenderer* GetSceneRenderer() { return m_SceneRenderer.get(); }
    
    // High-level graphics operations
    void LoadModel(const std::string& name, const std::string& filepath);
    void LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    void RenderModel(const std::string& modelName, const std::string& shaderName, const glm::mat4& transform = glm::mat4(1.0f));
    
    // Scene and Camera management
    void SetCameraPosition(const glm::vec3& position);
    void SetCameraRotation(const glm::vec3& rotation);

    // Window properties
    uint32_t GetWindowWidth() const;
    uint32_t GetWindowHeight() const;
    bool ShouldClose() const;

protected:
    // Graphics engine - handles all graphics operations internally
    void InitializeGraphicsEngine();
    void ShutdownGraphicsEngine();
    void ProcessInput();
    void CalculateDeltaTime();
    void UpdateCamera();
    void RenderFrame();

    // GLFW callbacks - static
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Core graphics systems - fully managed by graphics lib  
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    // Note: Rendering coordinators (MeshRenderer, FrustumCuller) now owned by SceneRenderer

    // Scene and camera
    std::shared_ptr<Scene> m_CurrentScene;
    std::shared_ptr<Camera> m_ActiveCamera;
    
    // Component accessor for Graphics/Engine communication
    std::unique_ptr<EngineComponentAccessor> m_ComponentAccessor;

    // Application state
    bool m_Running = true;
    std::string m_ApplicationName;

    // Timing
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime = 0.0f;
    
    // Input state
    bool m_FirstMouse = true;
    float m_LastX = 640.0f;
    float m_LastY = 360.0f;
    
    // Loaded resources tracking
    std::vector<std::string> m_LoadedModels;
    std::vector<std::string> m_LoadedShaders;
};