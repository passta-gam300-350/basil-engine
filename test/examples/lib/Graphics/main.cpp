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

#ifdef _WIN32
// NVIDIA Optimus - force discrete GPU
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}

// AMD PowerXpress - force discrete GPU  
extern "C" {
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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
    
    // Submit static data once during initialization
    for (const auto& light : m_SceneLights) {
        m_SceneRenderer->SubmitLight(light);
    }
    for (const auto& renderable : m_SceneObjects) {
        m_SceneRenderer->SubmitRenderable(renderable);
    }

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

        // Static data submitted once during initialization - no need to resubmit

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
    
    // Create a 10x10 grid of objects for instanced rendering
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial", "GoldMaterial", "WhiteMaterial"};
    
    const int gridSize = 10;
    const float spacing = 3.0f;
    const float startOffset = -(gridSize - 1) * spacing * 0.5f; // Center the grid
    
    // Create 10x10 grid (100 objects total) using both meshes
    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            glm::vec3 position(
                startOffset + x * spacing,
                0.0f,
                startOffset + z * spacing
            );
            
            // Use uniform scale
            glm::vec3 scaleVec(1.0f);
            
            // Cycle through materials based on position
            int materialIndex = (x + z) % materials.size();
            std::string material = materials[materialIndex];
            
            CreateModelInstance("tinbox", "DefaultMaterial", position, scaleVec);
        }
    }
    
    // Complex PBR lighting setup
    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(0.2f, -0.8f, 0.3f),
        glm::vec3(1.0f, 0.95f, 0.85f),
        1.0f
    ));
    
    // Point lights at the four corners of the 10x10 grid
    // Grid spans from -13.5 to +13.5 (with 3.0f spacing)
    float cornerOffset = 13.5f; // Half of (gridSize-1) * spacing
    float lightHeight = 8.0f;
    
    // Top-left corner (red light)
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(1.0f, 0.2f, 0.2f),
        4.0f,
        25.0f
    ));
    
    // Top-right corner (green light)
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(0.2f, 1.0f, 0.2f),
        4.0f,
        25.0f
    ));
    
    // Bottom-left corner (blue light)
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, cornerOffset),
        glm::vec3(0.2f, 0.2f, 1.0f),
        4.0f,
        25.0f
    ));
    
    // Bottom-right corner (yellow light)
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, cornerOffset),
        glm::vec3(1.0f, 1.0f, 0.2f),
        4.0f,
        25.0f
    ));
    
    // Single spotlight in center above the grid
    m_SceneLights.push_back(CreateSpotLight(
        glm::vec3(0.0f, 20.0f, 0.0f),        // Position: centered above grid
        glm::vec3(0.0f, -1.0f, 0.0f),        // Direction: pointing straight down
        glm::vec3(1.0f, 1.0f, 0.8f),         // Color: warm white light
        6.0f,                                // Intensity
        40.0f,                               // Range
        15.0f,                               // Inner cone angle
        30.0f                                // Outer cone angle
    ));
    
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.05f, 0.08f, 0.12f));
    
    std::cout << "Advanced scene created: " << m_SceneObjects.size() << " instances, " 
              << m_SceneLights.size() << " dynamic lights\n";
              
    // Debug: verify all objects were created
    std::cout << "Debug: Created " << m_SceneObjects.size() << " scene objects" << std::endl;
}

void GraphicsTestDriver::UpdateAdvancedScene()
{
    // Completely static scene - no animations for instances or lights
    // Objects and lights remain in their original positions with initial properties
}
void GraphicsTestDriver::CreateModelInstance(const std::string& modelName, const std::string& materialName,
                                            const glm::vec3& position, const glm::vec3& scale)
{
    auto model = m_ResourceManager->GetModel(modelName);
    if (!model || model->meshes.empty()) {
        return;
    }
    
    // Create a renderable for each mesh in the model
    for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
        // Share the same mesh objects instead of creating new ones
        static std::unordered_map<std::string, std::shared_ptr<Mesh>> s_MeshCache;
        
        std::string meshKey = modelName + "_mesh" + std::to_string(meshIndex);
        auto it = s_MeshCache.find(meshKey);
        
        std::shared_ptr<Mesh> mesh;
        if (it != s_MeshCache.end()) {
            // Reuse existing mesh
            mesh = it->second;
        } else {
            // Create new mesh and cache it
            mesh = std::make_shared<Mesh>(model->meshes[meshIndex]);
            s_MeshCache[meshKey] = mesh;
        }
        
        // Create renderable for this mesh
        RenderableData renderable;
        renderable.mesh = mesh;
        renderable.transform = glm::scale(glm::translate(glm::mat4(1.0f), position), scale);
        renderable.visible = true;
        
        // Use the mesh's original material if it exists, otherwise create a simple one
        auto shader = m_ResourceManager->GetShader("instanced_bindless");
        if (!shader) {
            shader = m_ResourceManager->GetShader("basic");
        }
        
        if (shader) {
            // Create a simple material wrapper - the mesh may already have texture/material data
            renderable.material = std::make_shared<Material>(shader, "MeshMaterial_" + std::to_string(meshIndex));
        }
        
        m_SceneObjects.push_back(renderable);
    }
}


RenderableData GraphicsTestDriver::CreateRenderable(const std::string& modelName, const std::string& materialName, 
                                                  const glm::vec3& position, const glm::vec3& scale, int meshIndex)
{
    // This method is deprecated - use CreateModelInstance instead
    RenderableData renderable;
    
    auto model = m_ResourceManager->GetModel(modelName);
    if (model && !model->meshes.empty()) {
        // Share the same mesh object instead of creating new ones  
        static std::unordered_map<std::string, std::shared_ptr<Mesh>> s_MeshCache;
        
        auto it = s_MeshCache.find(modelName);
        if (it != s_MeshCache.end()) {
            // Reuse existing mesh
            renderable.mesh = it->second;
        } else {
            // Create new mesh and cache it using first mesh only
            auto newMesh = std::make_shared<Mesh>(model->meshes[0]);
            s_MeshCache[modelName] = newMesh;
            renderable.mesh = newMesh;
        }
    }
    
    // Try to use bindless shader first, fall back to basic shader
    auto shader = m_ResourceManager->GetShader("instanced_bindless");
    if (!shader) {
        shader = m_ResourceManager->GetShader("basic");
    }
    
    if (shader) {
        // Reuse existing materials instead of creating new ones
        // This ensures all renderables with the same material name share the same material pointer
        static std::unordered_map<std::string, std::shared_ptr<Material>> s_MaterialCache;
        
        auto it = s_MaterialCache.find(materialName);
        if (it != s_MaterialCache.end()) {
            // Reuse existing material
            renderable.material = it->second;
        } else {
            // Create new material and cache it
            auto newMaterial = std::make_shared<Material>(shader, materialName);
            
            // Set material properties based on name
            if (materialName == "RedMaterial") {
                newMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.2f, 0.2f));
                newMaterial->SetFloat("u_Metallic", 0.1f);
                newMaterial->SetFloat("u_Roughness", 0.8f);
            }
            else if (materialName == "GreenMaterial") {
                newMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.8f, 0.2f));
                newMaterial->SetFloat("u_Metallic", 0.3f);
                newMaterial->SetFloat("u_Roughness", 0.6f);
            }
            else if (materialName == "BlueMaterial") {
                newMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.2f, 0.2f, 0.8f));
                newMaterial->SetFloat("u_Metallic", 0.5f);
                newMaterial->SetFloat("u_Roughness", 0.4f);
            }
            else if (materialName == "GoldMaterial") {
                newMaterial->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.8f, 0.2f));
                newMaterial->SetFloat("u_Metallic", 0.9f);
                newMaterial->SetFloat("u_Roughness", 0.1f);
            }
            else if (materialName == "WhiteMaterial") {
                newMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.9f, 0.9f, 0.9f));
                newMaterial->SetFloat("u_Metallic", 0.0f);
                newMaterial->SetFloat("u_Roughness", 0.9f);
            }
            
            s_MaterialCache[materialName] = newMaterial;
            renderable.material = newMaterial;
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