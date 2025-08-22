#include "Application.h"
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightComponent.h>
#include <iostream>
#include <GLFW/glfw3.h>

Application::Application(const std::string &name, uint32_t width, uint32_t height)
    : m_ApplicationName(name)
{

    std::cout << "=== Initializing " << name << " ===" << std::endl;

    // Create core systems in dependency order

    // 1. Window (must be first - creates OpenGL context)
    m_Window = std::make_unique<Window>(name, width, height);

    // 2. Renderer (depends on window for context)
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Initialize(m_Window->GetNativeWindow());

    // 3. Resource Manager (depends on OpenGL context)
    m_ResourceManager = std::make_unique<ResourceManager>();
    m_ResourceManager->Initialize();

    // 4. Scene (independent)
    m_CurrentScene = std::make_shared<Scene>("MainScene");

    // 5. Scene Renderer (depends on renderer being initialized)
    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_SceneRenderer->SetScene(m_CurrentScene);

    // 6. ECS Systems
    m_RenderSystem = std::make_unique<RenderSystem>();
    m_CullingSystem = std::make_unique<CullingSystem>();

    // Initialize timing
    m_LastFrameTime = std::chrono::steady_clock::now();

    // Initialize engine-specific setup
    InitializeEngine();

    // Call derived class initialization
    Initialize();
    LoadResources();

    std::cout << "=== Engine Initialized Successfully ===" << std::endl;
}

Application::~Application()
{
    std::cout << "=== Shutting Down Engine ===" << std::endl;

    // Call derived class cleanup
    Shutdown();

    // Clean up engine systems
    ShutdownEngine();

    std::cout << "=== Engine Shutdown Complete ===" << std::endl;
}

void Application::Run()
{
    std::cout << "=== Starting Engine Main Loop ===" << std::endl;

    while (!m_Window->ShouldClose() && m_Running)
    {

        // Calculate frame timing
        CalculateDeltaTime();

        // Poll window events
        m_Window->PollEvents();

        // Update phase
        Update(m_DeltaTime);
        m_CurrentScene->OnUpdate(m_DeltaTime);

        // Culling phase (if camera is available)
        if (m_ActiveCamera)
        {
            m_CullingSystem->OnUpdate(m_CurrentScene.get(), *m_ActiveCamera);
        }

        // Rendering phase
        m_Renderer->BeginFrame();

        // Update render system
        m_RenderSystem->OnUpdate(m_CurrentScene.get());

        // Custom rendering
        Render();

        // Scene rendering (pipeline-based)
        m_SceneRenderer->Render();

        // End frame and present
        m_Renderer->EndFrame();

        // Update frame statistics
        m_FrameCount++;
        m_FPSTimer += m_DeltaTime;
        if (m_FPSTimer >= 1.0f)
        {
            std::cout << "FPS: " << m_FrameCount << " | Frame Time: "
                << (m_FPSTimer / m_FrameCount) * 1000.0f << "ms" << std::endl;
            m_FrameCount = 0;
            m_FPSTimer = 0.0f;
        }
    }

    std::cout << "=== Engine Main Loop Ended ===" << std::endl;
}

void Application::SetActiveScene(std::shared_ptr<Scene> scene)
{
    m_CurrentScene = scene;
    m_SceneRenderer->SetScene(scene);
    std::cout << "Active scene set to: " << scene->GetName() << std::endl;
}

void Application::SetActiveCamera(std::shared_ptr<Camera> camera)
{
    m_ActiveCamera = camera;
    m_SceneRenderer->SetCamera(camera);
    std::cout << "Active camera set" << std::endl;
}

void Application::InitializeEngine()
{
    // Set up default engine state
    m_Renderer->GetContext()->SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    m_Renderer->GetContext()->SetVSync(true);

    // Print system information
    std::cout << "Window Size: " << m_Window->GetWidth() << "x" << m_Window->GetHeight() << std::endl;

    // You could add default resources here
    // LoadDefaultShaders();
    // LoadDefaultTextures();
}

void Application::ShutdownEngine()
{
    // Clean up in reverse order of creation
    m_CullingSystem.reset();
    m_RenderSystem.reset();
    m_SceneRenderer.reset();
    m_CurrentScene.reset();
    m_ActiveCamera.reset();

    if (m_ResourceManager)
    {
        m_ResourceManager->Shutdown();
        m_ResourceManager.reset();
    }

    if (m_Renderer)
    {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }

    m_Window.reset();
}

void Application::CalculateDeltaTime()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_LastFrameTime);
    m_DeltaTime = duration.count() / 1000000.0f; // Convert to seconds
    m_FrameTime = m_DeltaTime;
    m_LastFrameTime = currentTime;
}