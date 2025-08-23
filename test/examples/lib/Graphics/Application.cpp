#include "Application.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <Scene/Entity.h>
#include <unordered_map>

Application::Application(const std::string& name, uint32_t width, uint32_t height)
    : m_ApplicationName(name), m_LastX(width / 2.0f), m_LastY(height / 2.0f)
{
    std::cout << "=== Initializing " << name << " ===" << std::endl;
    
    InitializeGraphicsEngine();
    
    // Note: Initialize() and LoadResources() will be called in Run()
    // to ensure virtual functions work correctly
    
    std::cout << "=== Application Initialized Successfully ===" << std::endl;
}

Application::~Application()
{
    std::cout << "=== Shutting Down Application ===" << std::endl;
    
    Shutdown();
    ShutdownGraphicsEngine();
    
    std::cout << "=== Application Shutdown Complete ===" << std::endl;
}

void Application::Run()
{
    // Call derived class initialization and resource loading here
    // This ensures virtual functions work correctly
    Initialize();
    LoadResources();
    
    std::cout << "=== Starting Application Main Loop ===" << std::endl;
    
    while (!ShouldClose() && m_Running)
    {
        CalculateDeltaTime();
        ProcessInput();
        
        // User update
        Update(m_DeltaTime);
        
        // Graphics lib handles all rendering
        RenderFrame();
    }
    
    std::cout << "=== Application Main Loop Ended ===" << std::endl;
}

void Application::LoadModel(const std::string& name, const std::string& filepath)
{
    auto model = m_ResourceManager->LoadModel(name, filepath);
    if (model) {
        m_LoadedModels.push_back(name);
        std::cout << "✓ Model '" << name << "' loaded successfully" << std::endl;
    } else {
        std::cerr << "✗ Failed to load model '" << name << "'" << std::endl;
    }
}

void Application::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    auto shader = m_ResourceManager->LoadShader(name, vertexPath, fragmentPath);
    if (shader) {
        m_LoadedShaders.push_back(name);
        std::cout << "✓ Shader '" << name << "' loaded successfully" << std::endl;
    } else {
        std::cerr << "✗ Failed to load shader '" << name << "'" << std::endl;
    }
}

void Application::RenderModel(const std::string& modelName, const std::string& shaderName, const glm::mat4& transform)
{
    auto model = m_ResourceManager->GetModel(modelName);
    auto shader = m_ResourceManager->GetShader(shaderName);
    
    if (!model || !shader) {
        std::cerr << "Cannot render - model or shader not found: " << modelName << ", " << shaderName << std::endl;
        return;
    }
    
    // Create or get an entity for this model
    static std::unordered_map<std::string, entt::entity> modelEntities;
    static std::unordered_map<std::string, std::vector<entt::entity>> modelChildEntities;
    
    entt::entity entity;
    auto it = modelEntities.find(modelName);
    if (it == modelEntities.end()) {
        // Create a new entity for this model
        entity = m_CurrentScene->CreateEntity(modelName);
        modelEntities[modelName] = entity;
        std::cout << "RenderModel: Created main entity for model '" << modelName << "'" << std::endl;
        
        // For models with multiple meshes, create a child entity for each mesh
        if (model->meshes.size() == 1) {
            // Single mesh - attach directly to this entity
            std::cout << "RenderModel: Single mesh model, adding components to main entity" << std::endl;
            auto meshPtr = std::make_shared<Mesh>(model->meshes[0].vertices, model->meshes[0].indices, model->meshes[0].textures);
            m_CurrentScene->GetRegistry().emplace<MeshComponent>(entity, meshPtr);
            
            // Create a material with the shader
            auto material = std::make_shared<Material>(shader, shaderName);
            m_CurrentScene->GetRegistry().emplace<MaterialComponent>(entity, material);
            
            // TransformComponent is already added by CreateEntity
            std::cout << "RenderModel: Added all components to single mesh entity" << std::endl;
        } else {
            // Multiple meshes - create child entities
            std::cout << "RenderModel: Multi-mesh model (" << model->meshes.size() << " meshes), creating child entities" << std::endl;
            std::vector<entt::entity> childEntities;
            for (size_t i = 0; i < model->meshes.size(); ++i) {
                auto childEntity = m_CurrentScene->CreateEntity(modelName + "_mesh_" + std::to_string(i));
                childEntities.push_back(childEntity);
                std::cout << "RenderModel: Created child entity " << i << " for mesh" << std::endl;
                
                auto meshPtr = std::make_shared<Mesh>(model->meshes[i].vertices, model->meshes[i].indices, model->meshes[i].textures);
                m_CurrentScene->GetRegistry().emplace<MeshComponent>(childEntity, meshPtr);
                std::cout << "RenderModel: Added MeshComponent to child entity " << i << std::endl;
                
                auto material = std::make_shared<Material>(shader, shaderName);
                m_CurrentScene->GetRegistry().emplace<MaterialComponent>(childEntity, material);
                std::cout << "RenderModel: Added MaterialComponent to child entity " << i << std::endl;
                
                // Transform component is already added by CreateEntity, just update it if needed
                auto& childTransform = m_CurrentScene->GetRegistry().get<TransformComponent>(childEntity);
                std::cout << "RenderModel: Child entity " << i << " has all three components" << std::endl;
                // Could set specific transform properties here if needed
            }
            modelChildEntities[modelName] = childEntities;
            std::cout << "RenderModel: Completed creating " << childEntities.size() << " child entities" << std::endl;
        }
    } else {
        entity = it->second;
        std::cout << "RenderModel: Using existing entity for model '" << modelName << "'" << std::endl;
        
        // Update the material's shader if it changed
        // Only update if this entity actually has a MaterialComponent (single mesh case)
        if (m_CurrentScene->GetRegistry().all_of<MaterialComponent>(entity)) {
            auto& matComp = m_CurrentScene->GetRegistry().get<MaterialComponent>(entity);
            if (!matComp.Materials.empty()) {
                matComp.Materials[0] = std::make_shared<Material>(shader, shaderName);
            }
        } else {
            // For multi-mesh models, update child entities' materials
            auto childIt = modelChildEntities.find(modelName);
            if (childIt != modelChildEntities.end()) {
                for (auto childEntity : childIt->second) {
                    auto& matComp = m_CurrentScene->GetRegistry().get<MaterialComponent>(childEntity);
                    if (!matComp.Materials.empty()) {
                        matComp.Materials[0] = std::make_shared<Material>(shader, shaderName);
                    }
                }
            }
        }
    }
    
    // Update transform component (CreateEntity always adds TransformComponent)
    auto& transformComp = m_CurrentScene->GetRegistry().get<TransformComponent>(entity);
    // Extract translation, rotation, and scale from the transform matrix
    transformComp.Translation = glm::vec3(transform[3]);
    // For now, just set the transform matrix directly via a helper
    // In a real implementation, you'd decompose the matrix properly
}

void Application::SetCameraPosition(const glm::vec3& position)
{
    m_ActiveCamera->SetPosition(position);
}

void Application::SetCameraRotation(const glm::vec3& rotation)
{
    m_ActiveCamera->SetRotation(rotation);
}

uint32_t Application::GetWindowWidth() const
{
    return m_Window->GetWidth();
}

uint32_t Application::GetWindowHeight() const
{
    return m_Window->GetHeight();
}

bool Application::ShouldClose() const
{
    return m_Window->ShouldClose();
}

void Application::InitializeGraphicsEngine()
{
    // Initialize core graphics systems in proper order
    
    // 1. Window (creates OpenGL context)
    m_Window = std::make_unique<Window>(m_ApplicationName, 1280, 720);
    
    // Set up input callbacks
    glfwSetWindowUserPointer(m_Window->GetNativeWindow(), this);
    glfwSetKeyCallback(m_Window->GetNativeWindow(), KeyCallback);
    glfwSetCursorPosCallback(m_Window->GetNativeWindow(), MouseCallback);
    glfwSetScrollCallback(m_Window->GetNativeWindow(), ScrollCallback);
    glfwSetInputMode(m_Window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // 2. Renderer (depends on context)
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Initialize(m_Window->GetNativeWindow());
    
    // 3. Resource Manager
    m_ResourceManager = std::make_unique<ResourceManager>();
    m_ResourceManager->Initialize();
    
    // 4. Scene
    m_CurrentScene = std::make_shared<Scene>("MainScene");
    
    // 5. Camera with proper controls
    m_ActiveCamera = std::make_shared<Camera>(CameraType::Perspective);
    m_ActiveCamera->SetPerspective(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
    m_ActiveCamera->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    
    // 6. Scene Renderer
    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_SceneRenderer->SetScene(m_CurrentScene);
    m_SceneRenderer->SetCamera(m_ActiveCamera);
    
    // 7. ECS Systems
    m_RenderSystem = std::make_unique<RenderSystem>();
    m_CullingSystem = std::make_unique<CullingSystem>();
    
    // Initialize timing
    m_LastFrameTime = std::chrono::steady_clock::now();
    
    std::cout << "Graphics Engine initialized successfully" << std::endl;
}

void Application::ShutdownGraphicsEngine()
{
    // Clean up in reverse order
    m_CullingSystem.reset();
    m_RenderSystem.reset();
    m_SceneRenderer.reset();
    m_CurrentScene.reset();
    m_ActiveCamera.reset();
    
    if (m_ResourceManager) {
        m_ResourceManager->Shutdown();
        m_ResourceManager.reset();
    }
    
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }
    
    m_Window.reset();
}

void Application::ProcessInput()
{
    m_Window->PollEvents();
    
    GLFWwindow* window = m_Window->GetNativeWindow();
    
    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::FORWARD, m_DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::BACKWARD, m_DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::LEFT, m_DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::RIGHT, m_DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::UP, m_DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        m_ActiveCamera->ProcessKeyboard(CameraMovement::DOWN, m_DeltaTime);
        
    // Escape to close
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        m_Running = false;
}

void Application::CalculateDeltaTime()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_LastFrameTime);
    m_DeltaTime = duration.count() / 1000000.0f;
    m_LastFrameTime = currentTime;
}

void Application::RenderFrame()
{
    // Graphics library handles all rendering pipeline
    m_Renderer->BeginFrame();
    
    // Call user's render method to update scene/entities
    Render();
    
    // Update ECS systems - this will submit render commands to the queue
    if (m_ActiveCamera) {
        m_CullingSystem->OnUpdate(m_CurrentScene.get(), *m_ActiveCamera);
        m_RenderSystem->OnUpdate(m_CurrentScene.get(), *m_ActiveCamera);  // Use the new overload with camera
    }
    
    // Scene rendering through pipeline - this executes the render queue
    m_SceneRenderer->Render();
    
    m_Renderer->EndFrame();
}

// Static callback functions
void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->OnKeyboard(key, action, app->m_DeltaTime);
    }
}

void Application::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        if (app->m_FirstMouse) {
            app->m_LastX = xpos;
            app->m_LastY = ypos;
            app->m_FirstMouse = false;
        }
        
        float xoffset = xpos - app->m_LastX;
        float yoffset = app->m_LastY - ypos; // Reversed since y-coordinates go from bottom to top
        
        app->m_LastX = xpos;
        app->m_LastY = ypos;
        
        app->m_ActiveCamera->ProcessMouseMovement(xoffset, yoffset);
        app->OnMouse(xpos, ypos);
    }
}

void Application::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_ActiveCamera->ProcessMouseScroll(yoffset);
        app->OnMouseScroll(yoffset);
    }
}