#include "main.h"

#include <Resources/Material.h>
#include <Resources/Mesh.h>
#include <Resources/Model.h>
#include <Resources/Shader.h>
#include <Utility/Light.h>

#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

// Static instance for callbacks
GraphicsTestDriver* GraphicsTestDriver::s_Instance = nullptr;

GraphicsTestDriver::GraphicsTestDriver()
    : m_Window(nullptr)
    , m_Renderer(nullptr)
    , m_SceneRenderer(nullptr)
    , m_ResourceManager(nullptr)
    , m_Camera(nullptr)
    , m_Time(0.0f)
    , m_DeltaTime(0.0f)
    , m_CameraEnabled(true)
    , m_FirstMouse(true)
    , m_LastX(640.0f)
    , m_LastY(360.0f)
{
    s_Instance = this;
}

GraphicsTestDriver::~GraphicsTestDriver()
{
    s_Instance = nullptr;
}

bool GraphicsTestDriver::Initialize()
{
    std::cout << "=== Graphics Library Test Driver ===\n";
    std::cout << "Initializing graphics systems...\n\n";

    // Create window
    m_Window = std::make_unique<Window>("Graphics Library Test Driver", 1280, 720);
    if (!m_Window->GetNativeWindow()) {
        std::cerr << "Failed to create window!\n";
        return false;
    }

    // Create and initialize renderer
    static Renderer rendererInstance; // Create the singleton instance
    m_Renderer = &Renderer::Get(); // Now Get() will work since s_Instance is set
    m_Renderer->Initialize(m_Window->GetNativeWindow());

    // Create and initialize resource manager
    static ResourceManager resourceManagerInstance; // Create the singleton instance
    m_ResourceManager = &ResourceManager::Get(); // Now Get() will work since s_Instance is set
    m_ResourceManager->Initialize();

    // Create scene renderer
    m_SceneRenderer = std::make_unique<SceneRenderer>();

    // Setup camera
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
    m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));
    
    // We'll manually update the scene renderer's frame data with camera matrices

    // Setup input callbacks
    glfwSetKeyCallback(m_Window->GetNativeWindow(), KeyCallback);
    glfwSetCursorPosCallback(m_Window->GetNativeWindow(), MouseCallback);
    glfwSetScrollCallback(m_Window->GetNativeWindow(), ScrollCallback);
    glfwSetInputMode(m_Window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load resources
    if (!LoadTestResources()) {
        std::cerr << "Failed to load test resources!\n";
        return false;
    }

    // Setup the advanced demo scene
    SetupAdvancedScene();

    // Print system info
    PrintSystemInfo();

    m_LastFrameTime = std::chrono::steady_clock::now();

    std::cout << "Graphics system initialized successfully!\n\n";
    return true;
}

void GraphicsTestDriver::Run()
{
    std::cout << "Starting render loop...\n";
    std::cout << "Advanced Graphics Demo - Instanced + Bindless + PBR\n";
    std::cout << "Controls:\n";
    std::cout << "  - WASD: Move camera\n";
    std::cout << "  - Mouse: Look around\n";
    std::cout << "  - ESC: Exit\n";
    std::cout << "  - F1: Toggle camera controls\n";
    std::cout << "  - F2: Print scene info\n\n";

    while (!m_Window->ShouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        m_DeltaTime = std::chrono::duration<float>(currentTime - m_LastFrameTime).count();
        m_LastFrameTime = currentTime;
        m_Time += m_DeltaTime;

        // Process input
        ProcessInput();

        // Begin frame
        m_Renderer->BeginFrame();
        m_SceneRenderer->ClearFrame();
        
        // Update camera data manually
        if (m_Camera) {
            auto& frameData = m_SceneRenderer->GetFrameData();
            frameData.viewMatrix = m_Camera->GetViewMatrix();
            frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
            frameData.cameraPosition = m_Camera->GetPosition();
        }

        // Update the advanced scene
        UpdateAdvancedScene();

        // Submit scene data
        for (const auto& renderable : m_SceneObjects) {
            m_SceneRenderer->SubmitRenderable(renderable);
        }
        for (const auto& light : m_SceneLights) {
            m_SceneRenderer->SubmitLight(light);
        }

        // Render scene
        m_SceneRenderer->Render();

        // End frame
        m_Renderer->EndFrame();

        // Poll events
        m_Window->PollEvents();
    }

    std::cout << "Render loop ended.\n";
}

void GraphicsTestDriver::Shutdown()
{
    std::cout << "Shutting down graphics systems...\n";
    
    m_SceneRenderer.reset();
    m_Camera.reset();
    
    if (m_ResourceManager) {
        m_ResourceManager->Shutdown();
    }
    
    if (m_Renderer) {
        m_Renderer->Shutdown();
    }
    
    m_Window.reset();
    
    std::cout << "Shutdown complete.\n";
}

bool GraphicsTestDriver::LoadTestResources()
{
    std::cout << "Loading test resources...\n";

    try {
        // Load shaders
        auto basicShader = m_ResourceManager->LoadShader("basic", 
            "assets/shaders/basic.vert", 
            "assets/shaders/basic.frag");
        
        if (!basicShader) {
            std::cerr << "Failed to load basic shader!\n";
            return false;
        }

        // Load advanced bindless shaders for maximum visual quality
        auto instancedShader = m_ResourceManager->LoadShader("instanced_bindless",
            "assets/shaders/instanced_bindless.vert",
            "assets/shaders/instanced_bindless.frag");
        
        if (instancedShader) {
            std::cout << "Advanced bindless shaders loaded successfully!\n";
        } else {
            std::cout << "Warning: Could not load bindless shaders, will use basic shaders for fallback\n";
        }

        // Load models
        auto tinBoxModel = m_ResourceManager->LoadModel("tinbox", 
            "assets/models/tinbox/tin_box.obj");
        
        if (!tinBoxModel) {
            std::cerr << "Failed to load tin box model!\n";
            return false;
        }

        // Create test materials
        CreateTestMaterials();

        std::cout << "Resources loaded successfully!\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while loading resources: " << e.what() << "\n";
        return false;
    }
}

void GraphicsTestDriver::CreateTestMaterials()
{
    auto basicShader = m_ResourceManager->GetShader("basic");
    if (!basicShader) return;

    // Red material
    auto redMaterial = std::make_shared<Material>(basicShader, "RedMaterial");
    redMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.2f, 0.2f));
    redMaterial->SetFloat("u_Metallic", 0.1f);
    redMaterial->SetFloat("u_Roughness", 0.8f);

    // Green material
    auto greenMaterial = std::make_shared<Material>(basicShader, "GreenMaterial");
    greenMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.8f, 0.2f));
    greenMaterial->SetFloat("u_Metallic", 0.3f);
    greenMaterial->SetFloat("u_Roughness", 0.6f);

    // Blue material
    auto blueMaterial = std::make_shared<Material>(basicShader, "BlueMaterial");
    blueMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.2f, 0.8f));
    blueMaterial->SetFloat("u_Metallic", 0.5f);
    blueMaterial->SetFloat("u_Roughness", 0.4f);

    // Metallic gold material
    auto goldMaterial = std::make_shared<Material>(basicShader, "GoldMaterial");
    goldMaterial->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.8f, 0.2f));
    goldMaterial->SetFloat("u_Metallic", 0.9f);
    goldMaterial->SetFloat("u_Roughness", 0.1f);

    // White material
    auto whiteMaterial = std::make_shared<Material>(basicShader, "WhiteMaterial");
    whiteMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.9f, 0.9f, 0.9f));
    whiteMaterial->SetFloat("u_Metallic", 0.0f);
    whiteMaterial->SetFloat("u_Roughness", 0.9f);

    std::cout << "Test materials created.\n";
}

void GraphicsTestDriver::SetupAdvancedScene()
{
    std::cout << "Setting up Advanced Graphics Demo...\n";
    
    // Create a massive field of objects for instanced rendering
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos(-25.0f, 25.0f);
    std::uniform_real_distribution<float> scale(0.3f, 2.5f);
    std::uniform_int_distribution<int> materialIdx(0, 4);
    
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial", "GoldMaterial", "WhiteMaterial"};
    
    // Create 500+ objects for impressive instanced rendering
    for (int i = 0; i < 500; ++i) {
        glm::vec3 position(pos(gen), pos(gen) * 0.3f, pos(gen));
        glm::vec3 scaleVec(scale(gen));
        std::string material = materials[materialIdx(gen)];
        
        auto renderable = CreateRenderable("tinbox", material, position, scaleVec);
        m_SceneObjects.push_back(renderable);
    }
    
    // Complex PBR lighting setup
    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(0.2f, -0.8f, 0.3f),
        glm::vec3(1.0f, 0.95f, 0.85f),
        1.5f
    ));
    
    // Multiple dynamic point lights
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(-12.0f, 8.0f, -12.0f),
        glm::vec3(1.0f, 0.3f, 0.1f),
        4.0f,
        25.0f
    ));
    
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(12.0f, 6.0f, 12.0f),
        glm::vec3(0.1f, 0.3f, 1.0f),
        3.5f,
        20.0f
    ));
    
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(0.0f, 10.0f, 0.0f),
        glm::vec3(0.2f, 1.0f, 0.4f),
        5.0f,
        30.0f
    ));
    
    // Dramatic spot lights
    m_SceneLights.push_back(CreateSpotLight(
        glm::vec3(-20.0f, 15.0f, -20.0f),
        glm::vec3(1.0f, 0.2f, 0.8f),
        glm::vec3(1.0f, 0.5f, 0.2f),
        6.0f,
        35.0f,
        12.0f,
        25.0f
    ));
    
    m_SceneLights.push_back(CreateSpotLight(
        glm::vec3(20.0f, 12.0f, 20.0f),
        glm::vec3(-0.8f, -0.5f, -0.6f),
        glm::vec3(0.3f, 0.8f, 1.0f),
        4.5f,
        28.0f,
        15.0f,
        30.0f
    ));
    
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.05f, 0.08f, 0.12f));
    
    std::cout << "Advanced scene created: " << m_SceneObjects.size() << " instances, " 
              << m_SceneLights.size() << " dynamic lights\n";
}

void GraphicsTestDriver::UpdateAdvancedScene()
{
    // Complex wave-based animation for all instances
    for (size_t i = 0; i < m_SceneObjects.size(); ++i) {
        float phase = i * 0.05f;
        float time_offset = i * 0.1f;
        
        // Extract base position (stored in transform matrix column 3)
        glm::vec3 basePos = glm::vec3(m_SceneObjects[i].transform[3]);
        
        // Multi-layered wave motion
        glm::vec3 waveOffset(
            sin(m_Time * 0.8f + phase) * 1.2f + cos(m_Time * 1.3f + time_offset) * 0.6f,
            cos(m_Time * 1.2f + phase) * 0.8f + sin(m_Time * 0.9f + time_offset) * 0.4f,
            sin(m_Time * 0.6f + phase) * 1.0f + cos(m_Time * 1.1f + time_offset) * 0.5f
        );
        
        // Complex rotation combining multiple axes
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), m_Time * 0.3f + phase, 
            glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), m_Time * 0.5f + time_offset, 
            glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), m_Time * 0.2f + phase, 
            glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::mat4 rotation = rotationY * rotationX * rotationZ;
        
        // Dynamic scaling based on distance from center
        float distFromCenter = glm::length(basePos);
        float scaleWave = 0.8f + 0.4f * sin(m_Time * 2.0f + distFromCenter * 0.1f);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleWave));
        
        // Combine transformations
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), basePos + waveOffset);
        m_SceneObjects[i].transform = translation * rotation * scale;
    }
    
    // Animate lights for dramatic effect
    if (m_SceneLights.size() >= 6) {
        // Animate point light positions in orbiting patterns
        float radius1 = 15.0f + 8.0f * sin(m_Time * 0.3f);
        float radius2 = 12.0f + 6.0f * cos(m_Time * 0.4f);
        
        m_SceneLights[1].position = glm::vec3(
            cos(m_Time * 0.6f) * radius1,
            8.0f + 4.0f * sin(m_Time * 0.8f),
            sin(m_Time * 0.6f) * radius1
        );
        
        m_SceneLights[2].position = glm::vec3(
            cos(m_Time * 0.7f + 3.14f) * radius2,
            6.0f + 3.0f * cos(m_Time * 1.2f),
            sin(m_Time * 0.7f + 3.14f) * radius2
        );
        
        // Animate light colors
        m_SceneLights[1].color = glm::vec3(
            0.8f + 0.3f * sin(m_Time),
            0.2f + 0.2f * cos(m_Time * 1.3f),
            0.1f + 0.1f * sin(m_Time * 0.7f)
        );
        
        m_SceneLights[2].color = glm::vec3(
            0.1f + 0.1f * sin(m_Time * 0.9f),
            0.2f + 0.2f * cos(m_Time * 1.1f),
            0.8f + 0.3f * sin(m_Time * 1.4f)
        );
        
        m_SceneLights[3].color = glm::vec3(
            0.2f + 0.3f * cos(m_Time * 0.6f),
            0.9f + 0.2f * sin(m_Time * 0.8f),
            0.4f + 0.4f * sin(m_Time * 1.0f)
        );
        
        // Animate spot light directions
        m_SceneLights[4].direction = glm::normalize(glm::vec3(
            sin(m_Time * 0.4f),
            -0.6f,
            cos(m_Time * 0.3f)
        ));
        
        m_SceneLights[5].direction = glm::normalize(glm::vec3(
            -sin(m_Time * 0.5f),
            -0.8f,
            -cos(m_Time * 0.4f)
        ));
    }
    
    // Animate directional light
    if (!m_SceneLights.empty()) {
        m_SceneLights[0].direction = glm::normalize(glm::vec3(
            0.3f * sin(m_Time * 0.2f),
            -0.8f,
            0.5f * cos(m_Time * 0.15f)
        ));
        
        // Subtle color animation for day/night cycle effect
        float dayNightCycle = 0.8f + 0.2f * sin(m_Time * 0.1f);
        m_SceneLights[0].color = glm::vec3(
            dayNightCycle * 1.0f,
            dayNightCycle * 0.95f,
            dayNightCycle * 0.85f
        );
    }
}
RenderableData GraphicsTestDriver::CreateRenderable(const std::string& modelName, const std::string& materialName, 
                                                  const glm::vec3& position, const glm::vec3& scale)
{
    RenderableData renderable;
    
    auto model = m_ResourceManager->GetModel(modelName);
    if (model && !model->meshes.empty()) {
        renderable.mesh = std::make_shared<Mesh>(model->meshes[0]);
    }
    
    // Try to use bindless shader first, fall back to basic shader
    auto shader = m_ResourceManager->GetShader("instanced_bindless");
    if (!shader) {
        shader = m_ResourceManager->GetShader("basic");
    }
    
    if (shader) {
        renderable.material = std::make_shared<Material>(shader, materialName);
        
        // Set material properties based on name
        if (materialName == "RedMaterial") {
            renderable.material->SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.2f, 0.2f));
            renderable.material->SetFloat("u_Metallic", 0.1f);
            renderable.material->SetFloat("u_Roughness", 0.8f);
        }
        else if (materialName == "GreenMaterial") {
            renderable.material->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.8f, 0.2f));
            renderable.material->SetFloat("u_Metallic", 0.3f);
            renderable.material->SetFloat("u_Roughness", 0.6f);
        }
        else if (materialName == "BlueMaterial") {
            renderable.material->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.2f, 0.8f));
            renderable.material->SetFloat("u_Metallic", 0.5f);
            renderable.material->SetFloat("u_Roughness", 0.4f);
        }
        else if (materialName == "GoldMaterial") {
            renderable.material->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.8f, 0.2f));
            renderable.material->SetFloat("u_Metallic", 0.9f);
            renderable.material->SetFloat("u_Roughness", 0.1f);
        }
        else if (materialName == "WhiteMaterial") {
            renderable.material->SetVec3("u_AlbedoColor", glm::vec3(0.9f, 0.9f, 0.9f));
            renderable.material->SetFloat("u_Metallic", 0.0f);
            renderable.material->SetFloat("u_Roughness", 0.9f);
        }
    }
    
    renderable.transform = glm::scale(glm::translate(glm::mat4(1.0f), position), scale);
    renderable.visible = true;
    
    return renderable;
}

SubmittedLightData GraphicsTestDriver::CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    SubmittedLightData light;
    light.type = Light::Type::Directional;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.intensity = intensity;
    light.enabled = true;
    return light;
}

SubmittedLightData GraphicsTestDriver::CreatePointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float range)
{
    SubmittedLightData light;
    light.type = Light::Type::Point;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.range = range;
    light.enabled = true;
    return light;
}

SubmittedLightData GraphicsTestDriver::CreateSpotLight(const glm::vec3& position, const glm::vec3& direction,
                                                     const glm::vec3& color, float intensity, float range,
                                                     float innerCone, float outerCone)
{
    SubmittedLightData light;
    light.type = Light::Type::Spot;
    light.position = position;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.intensity = intensity;
    light.range = range;
    light.innerCone = innerCone;
    light.outerCone = outerCone;
    light.enabled = true;
    return light;
}

void GraphicsTestDriver::ProcessInput()
{
    if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_Window->GetNativeWindow(), true);
    }
    
    // No scene switching - single advanced demo
    
    // Camera movement
    if (m_CameraEnabled && m_Camera) {
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::FORWARD, m_DeltaTime);
        }
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_S) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::BACKWARD, m_DeltaTime);
        }
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_A) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::LEFT, m_DeltaTime);
        }
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_D) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::RIGHT, m_DeltaTime);
        }
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::UP, m_DeltaTime);
        }
        if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            m_Camera->ProcessKeyboard(CameraMovement::DOWN, m_DeltaTime);
        }
    }
}

void GraphicsTestDriver::UpdateCamera()
{
    // Camera update is handled in ProcessInput
}

void GraphicsTestDriver::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (!s_Instance) return;
    
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_F1:
                s_Instance->m_CameraEnabled = !s_Instance->m_CameraEnabled;
                if (s_Instance->m_CameraEnabled) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    s_Instance->m_FirstMouse = true;
                    std::cout << "Camera controls enabled\n";
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    std::cout << "Camera controls disabled\n";
                }
                break;
                
            case GLFW_KEY_F2:
                s_Instance->PrintSceneInfo();
                break;
        }
    }
}

void GraphicsTestDriver::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_Instance || !s_Instance->m_CameraEnabled) return;
    
    if (s_Instance->m_FirstMouse) {
        s_Instance->m_LastX = static_cast<float>(xpos);
        s_Instance->m_LastY = static_cast<float>(ypos);
        s_Instance->m_FirstMouse = false;
    }
    
    float xoffset = static_cast<float>(xpos) - s_Instance->m_LastX;
    float yoffset = s_Instance->m_LastY - static_cast<float>(ypos); // Reversed since y-coordinates go from bottom to top
    
    s_Instance->m_LastX = static_cast<float>(xpos);
    s_Instance->m_LastY = static_cast<float>(ypos);
    
    if (s_Instance->m_Camera) {
        s_Instance->m_Camera->ProcessMouseMovement(xoffset, yoffset);
    }
}

void GraphicsTestDriver::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!s_Instance || !s_Instance->m_Camera) return;
    
    s_Instance->m_Camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void GraphicsTestDriver::PrintSystemInfo()
{
    std::cout << "=== Graphics System Information ===\n";
    std::cout << "Window Size: " << m_Window->GetWidth() << "x" << m_Window->GetHeight() << "\n";
    std::cout << "Resource Manager Status: " << (m_ResourceManager ? "Active" : "Inactive") << "\n";
    if (m_ResourceManager) {
        std::cout << "Loaded Shaders: " << m_ResourceManager->GetShaderCount() << "\n";
        std::cout << "Loaded Models: " << m_ResourceManager->GetModelCount() << "\n";
    }
    std::cout << "=====================================\n\n";
}

void GraphicsTestDriver::PrintSceneInfo()
{
    std::cout << "\n=== Current Scene Information ===\n";
    std::cout << "Objects: " << m_SceneObjects.size() << "\n";
    std::cout << "Lights: " << m_SceneLights.size() << "\n";
    
    if (m_Camera) {
        auto pos = m_Camera->GetPosition();
        auto rot = m_Camera->GetRotation();
        std::cout << "Camera Position: (" << std::fixed << std::setprecision(2) 
                  << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
        std::cout << "Camera Rotation: (" << std::fixed << std::setprecision(1)
                  << rot.x << "°, " << rot.y << "°, " << rot.z << "°)\n";
    }
    
    std::cout << "Frame Time: " << std::fixed << std::setprecision(3) << m_DeltaTime * 1000.0f << "ms"
              << " (FPS: " << std::fixed << std::setprecision(1) << 1.0f / m_DeltaTime << ")\n";
    std::cout << "===============================\n\n";
}

// Main entry point
int main()
{
    GraphicsTestDriver driver;
    
    if (!driver.Initialize()) {
        std::cerr << "Failed to initialize graphics test driver!\n";
        return -1;
    }
    
    driver.Run();
    driver.Shutdown();
    
    return 0;
}