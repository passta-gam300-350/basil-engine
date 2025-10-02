#include "main.h"

#include <Resources/Material.h>
#include <Resources/Mesh.h>
#include <Resources/Model.h>
#include <Resources/PrimitiveGenerator.h>
#include <Utility/Light.h>
#include <Utility/AABB.h>
#include <Pipeline/DebugRenderPass.h>

#include <spdlog/spdlog.h>
#include <random>
#include <glm/gtx/matrix_decompose.hpp>
#include <map>

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
    , m_SceneRenderer(nullptr)
    , m_ResourceManager(nullptr)
    , m_Camera(nullptr)
    , m_Time(0.0f)
    , m_DeltaTime(0.0f)
    , m_CameraEnabled(false)
    , m_FirstMouse(true)
    , m_LastX(640.0f)
    , m_LastY(360.0f)
    , m_RotationEnabled(true)
{
    s_Instance = this;
}

GraphicsTestDriver::~GraphicsTestDriver()
{
    s_Instance = nullptr;
}

bool GraphicsTestDriver::Initialize()
{
    spdlog::info("=== Graphics Library Test Driver ===");
    spdlog::info("Initializing graphics systems...");

    // Create window
    m_Window = std::make_unique<Window>("Graphics Library Test Driver", 1280, 720);
    if (!m_Window->GetNativeWindow()) {
        spdlog::error("Failed to create window!");
        return false;
    }

    // Create scene renderer (owns all graphics systems now)
    m_SceneRenderer = std::make_unique<SceneRenderer>();

    // Get references to systems owned by SceneRenderer
    m_ResourceManager = m_SceneRenderer->GetResourceManager();

    // Setup camera
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
    m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));
    
    // We'll manually update the scene renderer's frame data with camera matrices

    // Setup input callbacks
    glfwSetKeyCallback(m_Window->GetNativeWindow(), KeyCallback);
    glfwSetCursorPosCallback(m_Window->GetNativeWindow(), MouseCallback);
    glfwSetMouseButtonCallback(m_Window->GetNativeWindow(), MouseButtonCallback);
    glfwSetScrollCallback(m_Window->GetNativeWindow(), ScrollCallback);
    glfwSetInputMode(m_Window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Load resources
    if (!LoadTestResources()) {
        spdlog::error("Failed to load test resources!");
        return false;
    }

    // Setup the advanced demo scene
    SetupAdvancedScene();
    
    

    // Print system info
    PrintSystemInfo();

    m_LastFrameTime = std::chrono::steady_clock::now();

    spdlog::info("Graphics system initialized successfully!");
    return true;
}

void GraphicsTestDriver::Run()
{
    spdlog::info("Starting render loop...");
    spdlog::info("Advanced Graphics Demo - Instanced + Traditional Textures + PBR + AABB Debug");
    spdlog::info("Note: First tinbox model (all meshes) will rotate to demonstrate AABB transformation");
    spdlog::info("Controls:");
    spdlog::info("  - WASD: Move camera");
    spdlog::info("  - Mouse: Look around");
    spdlog::info("  - ESC: Exit");
    spdlog::info("  - F1: Toggle camera controls");
    spdlog::info("  - F2: Print scene info");
    spdlog::info("  - F3: Print render pass status");
    spdlog::info("  - 1: Toggle shadow pass");
    spdlog::info("  - 2: Toggle main pass");
    spdlog::info("  - 3: Toggle post-process pass");
    spdlog::info("  - 4: Toggle AABB wireframes");
    spdlog::info("  - 5: Toggle object rotation");
    spdlog::info("  - 6: Toggle skybox");
    spdlog::info("  - 7: Toggle point shadows");  // NEW

    while (!m_Window->ShouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        m_DeltaTime = std::chrono::duration<float>(currentTime - m_LastFrameTime).count();
        m_LastFrameTime = currentTime;
        m_Time += m_DeltaTime;

        // Process input
        ProcessInput();

        // Begin frame - no direct renderer access needed
        m_SceneRenderer->ClearFrame();

        // Submit static data once during initialization
        for (const auto& light : m_SceneLights) {
            m_SceneRenderer->SubmitLight(light);
        }
        for (const auto& renderable : m_SceneObjects) {
            m_SceneRenderer->SubmitRenderable(renderable);
        }

        // Update camera data manually
        if (m_Camera) {
            auto& frameData = m_SceneRenderer->GetFrameData();
            frameData.viewMatrix = m_Camera->GetViewMatrix();
            frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
            frameData.cameraPosition = m_Camera->GetPosition();
        }

        // Update transformations (rotate one instance for AABB testing)
        UpdateInstanceTransforms();

        // Calculate and submit debug AABBs for visualization
        CalculateAndSubmitAABBs();

        // Render scene (handles begin/end frame internally)
        m_SceneRenderer->Render();

		// Swap buffers
		m_Window->SwapBuffers();

        // Poll events
        m_Window->PollEvents();
    }

    spdlog::info("Render loop ended.");
}

void GraphicsTestDriver::Shutdown()
{
    spdlog::info("Shutting down graphics systems...");

    // Clear scene data first
    m_SceneObjects.clear();
    m_SceneLights.clear();

    // Reset in proper order - SceneRenderer owns Renderer and ResourceManager now
    m_SceneRenderer.reset();  // This will properly clean up all graphics systems
    m_Camera.reset();
    m_Window.reset();

    // Clear pointers (they're just references now, not owners)
    m_ResourceManager = nullptr;

    spdlog::info("Shutdown complete.");
}

bool GraphicsTestDriver::LoadTestResources()
{
    spdlog::info("Loading test resources...");

    try {
        // Load primitive shader for simple geometry rendering (light cubes, etc.)
        auto primitiveShader = m_ResourceManager->LoadShader("primitive",
            "assets/shaders/primitive.vert",
            "assets/shaders/primitive.frag");

        if (!primitiveShader) {
            spdlog::error("Failed to load primitive shader!");
            return false;
        }

        // Configure the debug render pass with the loaded shader
        m_SceneRenderer->SetDebugPrimitiveShader(primitiveShader);

        // Create debug visualization meshes
        auto lightCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
        auto lightRay = std::make_shared<Mesh>(PrimitiveGenerator::CreateDirectionalRay(3.0f));
        auto wireframeCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateWireframeCube(1.0f));

        // Configure debug meshes
        m_SceneRenderer->SetDebugLightCubeMesh(lightCube);
        m_SceneRenderer->SetDebugDirectionalRayMesh(lightRay);
        m_SceneRenderer->SetDebugAABBWireframeMesh(wireframeCube);

        // Load advanced traditional texture binding shaders for maximum visual quality
        auto instancedShader = m_ResourceManager->LoadShader("main_pbr",
            "assets/shaders/main_pbr.vert",
            "assets/shaders/main_pbr.frag");

        if (instancedShader) {
            spdlog::info("Advanced traditional texture shaders loaded successfully!");
        } else {
            spdlog::warn("Could not load traditional texture shaders, will use basic shaders for fallback");
        }

        // Load shadow mapping depth-only shader
        auto shadowShader = m_ResourceManager->LoadShader("shadow_depth",
            "assets/shaders/shadow_depth.vert",
            "assets/shaders/shadow_depth.frag");

        if (shadowShader) {
            spdlog::info("Shadow mapping shader loaded successfully!");
            // Configure the shadow mapping pass with the loaded shader
            m_SceneRenderer->SetShadowDepthShader(shadowShader);
        } else {
            spdlog::warn("Could not load shadow mapping shader");
        }

        // NEW: Load point shadow shader WITH geometry shader
        auto pointShadowShader = m_ResourceManager->LoadShader("point_shadow_depth",
            "assets/shaders/point_shadow_depth.vert",
            "assets/shaders/point_shadow_depth.geom",  // Geometry shader
            "assets/shaders/point_shadow_depth.frag");

        if (pointShadowShader)
        {
            spdlog::info("Point shadow shader (with geometry shader) loaded successfully!");
            m_SceneRenderer->SetPointShadowDepthShader(pointShadowShader);
        }
        else
        {
            spdlog::warn("Could not load point shadow shader");
        }

        // Load skybox shader
        auto skyboxShader = m_ResourceManager->LoadShader("skybox",
            "assets/shaders/skybox.vert",
            "assets/shaders/skybox.frag");

        if (skyboxShader)
        {
            spdlog::info("Skybox shader loaded successfully!");
            m_SceneRenderer->SetSkyboxShader(skyboxShader);
        }
        else
        {
            spdlog::warn("Could not load skybox shader");
        }

        // Load skybox cubemap
        std::array<std::string, 6> skyboxFaces = {
            "right.jpg",    // +X
            "left.jpg",     // -X
            "top.jpg",      // +Y
            "bottom.jpg",   // -Y
            "front.jpg",    // +Z
            "back.jpg"      // -Z
        };

        unsigned int skyboxCubemap = TextureLoader::CubemapFromFiles(
            skyboxFaces, "assets/skybox");

        if (skyboxCubemap != 0)
        {
            spdlog::info("Skybox cubemap loaded successfully!");
            m_SceneRenderer->SetSkyboxCubemap(skyboxCubemap);
            m_SceneRenderer->EnableSkybox(true);
        }
        else
        {
            spdlog::warn("Could not load skybox cubemap");
        }

        // Load picking shader for object selection
        auto pickingShader = m_ResourceManager->LoadShader("picking",
            "assets/shaders/picking.vert",
            "assets/shaders/picking.frag");

        if (pickingShader) {
            spdlog::info("Picking shader loaded successfully!");
            // Configure the picking pass with the loaded shader
            m_SceneRenderer->SetPickingShader(pickingShader);
        } else {
            spdlog::warn("Could not load picking shader");
        }

        // Load models
        auto tinBoxModel = m_ResourceManager->LoadModel("tinbox",
            "assets/models/tinbox/tin_box.obj");

        if (!tinBoxModel) {
            spdlog::error("Failed to load tin box model!");
            return false;
        }

        // Create test materials
        CreateTestMaterials();

        spdlog::info("Resources loaded successfully!");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Exception while loading resources: {}", e.what());
        return false;
    }
}

void GraphicsTestDriver::CreateTestMaterials()
{
    // Use the instanced traditional shader for materials (fallback to basic if needed)
    auto shader = m_ResourceManager->GetShader("main_pbr");
    if (!shader) {
        shader = m_ResourceManager->GetShader("basic");
    }
    if (!shader) return;

    // Red material
    auto redMaterial = std::make_shared<Material>(shader, "RedMaterial");
    redMaterial->SetAlbedoColor(glm::vec3(0.8f, 0.2f, 0.2f));
    redMaterial->SetMetallicValue(0.1f);
    redMaterial->SetRoughnessValue(0.8f);
    m_ResourceManager->AddMaterial("RedMaterial", redMaterial);

    // Green material
    auto greenMaterial = std::make_shared<Material>(shader, "GreenMaterial");
    greenMaterial->SetAlbedoColor(glm::vec3(0.2f, 0.8f, 0.2f));
    greenMaterial->SetMetallicValue(0.3f);
    greenMaterial->SetRoughnessValue(0.6f);
    m_ResourceManager->AddMaterial("GreenMaterial", greenMaterial);

    // Blue material
    auto blueMaterial = std::make_shared<Material>(shader, "BlueMaterial");
    blueMaterial->SetAlbedoColor(glm::vec3(0.2f, 0.2f, 0.8f));
    blueMaterial->SetMetallicValue(0.5f);
    blueMaterial->SetRoughnessValue(0.4f);
    m_ResourceManager->AddMaterial("BlueMaterial", blueMaterial);

    // Metallic gold material
    auto goldMaterial = std::make_shared<Material>(shader, "GoldMaterial");
    goldMaterial->SetAlbedoColor(glm::vec3(1.0f, 0.8f, 0.2f));
    goldMaterial->SetMetallicValue(0.9f);
    goldMaterial->SetRoughnessValue(0.1f);
    m_ResourceManager->AddMaterial("GoldMaterial", goldMaterial);

    // White material
    auto whiteMaterial = std::make_shared<Material>(shader, "WhiteMaterial");
    whiteMaterial->SetAlbedoColor(glm::vec3(1.0f, 1.0f, 1.0f));
    whiteMaterial->SetMetallicValue(0.0f);
    whiteMaterial->SetRoughnessValue(0.0f);
    m_ResourceManager->AddMaterial("WhiteMaterial", whiteMaterial);

    // Default material - simple metallic gray
    auto defaultMaterial = std::make_shared<Material>(shader, "DefaultMaterial");
    defaultMaterial->SetAlbedoColor(glm::vec3(0.7f, 0.7f, 0.7f));
    defaultMaterial->SetMetallicValue(0.8f);
    defaultMaterial->SetRoughnessValue(0.2f);
    m_ResourceManager->AddMaterial("DefaultMaterial", defaultMaterial);

    spdlog::info("Test materials created and registered.");
}

void GraphicsTestDriver::SetupAdvancedScene()
{
    spdlog::info("Setting up Advanced Graphics Demo...");

    // Create ground plane
    auto planeMesh = std::make_shared<Mesh>(
        PrimitiveGenerator::CreatePlane(30.0f, 30.0f, 10, 10)
    );

    // Add ground plane to scene using GreenMaterial
    RenderableData ground;
    ground.mesh = planeMesh;
    ground.material = m_ResourceManager->GetMaterial("WhiteMaterial");
    ground.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    ground.visible = true;
    ground.objectID = 1;  // Ground plane has object ID 1
    m_SceneObjects.push_back(ground);

    // Create grids of objects for instanced rendering
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial", "GoldMaterial", "WhiteMaterial"};

    const int gridSize = 3;
    const float spacing = 3.0f;
    const float startOffset = -(gridSize - 1) * spacing * 0.5f; // Center the grid

    // Create tinbox grid (left side)
    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            glm::vec3 position(
                startOffset + x * spacing - 8.0f, // Offset to the left
                0.0f,
                startOffset + z * spacing
            );

            // Use uniform scale
            glm::vec3 scaleVec(1.0f);

            // Cycle through materials based on position
            int materialIndex = (x + z) % materials.size();
            std::string material = materials[materialIndex];

            CreateModelInstance("tinbox", material, position, scaleVec);
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
    
    // Top-left corner (red light) - WITH SHADOWS
    auto redLight = CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(1.0f, 0.2f, 0.2f),
        4.0f,
        25.0f
    );
    redLight.castShadows = true;  // Enable shadows for this light
    m_SceneLights.push_back(redLight);

    // Top-right corner (green light) - WITH SHADOWS
    auto greenLight = CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(0.2f, 1.0f, 0.2f),
        4.0f,
        25.0f
    );
    greenLight.castShadows = true;  // Enable shadows for this light
    m_SceneLights.push_back(greenLight);

    // Bottom-left corner (blue light) - NO SHADOWS
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, cornerOffset),
        glm::vec3(0.2f, 0.2f, 1.0f),
        4.0f,
        25.0f
    ));

    // Bottom-right corner (yellow light) - NO SHADOWS
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, cornerOffset),
        glm::vec3(1.0f, 1.0f, 0.2f),
        4.0f,
        25.0f
    ));
    
    // Single spotlight in center above the grid
    m_SceneLights.push_back(CreateSpotLight(
        glm::vec3(0.0f, 10.0f, 0.0f),        // Position: centered above grid
        glm::vec3(0.0f, -1.0f, 0.0f),        // Direction: pointing straight down
        glm::vec3(0.8f, 0.0f, 0.8f),         // Color: purple light
        6.0f,                                // Intensity
        40.0f,                               // Range
        15.0f,                               // Inner cone angle
        30.0f                                // Outer cone angle
    ));
    
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.05f, 0.08f, 0.12f));

    spdlog::info("Advanced scene created: {} instances, {} dynamic lights", m_SceneObjects.size(), m_SceneLights.size());

    // Debug: verify all objects were created
    spdlog::debug("Created {} scene objects", m_SceneObjects.size());
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
        
        // Check if this mesh has textures (indicating we should preserve them)
        if (mesh->textures.size() > 0) {

            // Create a material that will work with the existing textures
            auto shader = m_ResourceManager->GetShader("main_pbr");
            if (!shader) {
                shader = m_ResourceManager->GetShader("basic");
            }

            if (shader) {
                renderable.material = std::make_shared<Material>(shader, "TexturedMaterial_" + std::to_string(meshIndex));
                // Use white/neutral colors so textures show through properly
                renderable.material->SetAlbedoColor(glm::vec3(1.0f, 1.0f, 1.0f));
                renderable.material->SetMetallicValue(0.0f);
                renderable.material->SetRoughnessValue(0.5f);
            }
        } else {
            // No textures - use our custom colored material
            spdlog::info("Mesh {} has no textures, using custom material: {}", meshIndex, materialName);
            renderable.material = m_ResourceManager->GetMaterial(materialName);
        }

        // Fallback: create a simple material if nothing worked
        if (!renderable.material) {
            auto shader = m_ResourceManager->GetShader("main_pbr");
            if (!shader) {
                shader = m_ResourceManager->GetShader("basic");
            }

            if (shader) {
                renderable.material = std::make_shared<Material>(shader, "FallbackMaterial_" + std::to_string(meshIndex));
                // Set default PBR properties
                renderable.material->SetAlbedoColor(glm::vec3(0.8f, 0.7f, 0.6f));
                renderable.material->SetMetallicValue(0.7f);
                renderable.material->SetRoughnessValue(0.3f);
            }
        }

        // Assign unique object ID (start from 100 for scene objects)
        static uint32_t nextObjectID = 100;
        renderable.objectID = nextObjectID++;
        spdlog::info("Created object with ID: {}", renderable.objectID);

        m_SceneObjects.push_back(renderable);
    }
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

void GraphicsTestDriver::CalculateAndSubmitAABBs()
{
    // Clear previous frame's debug AABBs
    auto& frameData = m_SceneRenderer->GetFrameData();
    frameData.debugAABBs.clear();

    // Group meshes by model instance (based on position)
    struct Vec3Compare {
        bool operator()(const glm::vec3& a, const glm::vec3& b) const {
            const float epsilon = 0.01f;
            if (std::abs(a.x - b.x) > epsilon) return a.x < b.x;
            if (std::abs(a.y - b.y) > epsilon) return a.y < b.y;
            return a.z < b.z;
        }
    };

    std::map<glm::vec3, std::vector<const RenderableData*>, Vec3Compare> modelInstances;

    // Group all meshes by their position (model instance)
    for (const auto& renderable : m_SceneObjects) {
        if (!renderable.visible || !renderable.mesh) {
            continue;
        }

        // Extract position from transform matrix
        glm::vec3 position = glm::vec3(renderable.transform[3]);
        modelInstances[position].push_back(&renderable);
    }

    // Calculate AABB per model instance
    for (auto it = modelInstances.begin(); it != modelInstances.end(); ++it) {
        const glm::vec3& instancePosition = it->first;
        const std::vector<const RenderableData*>& meshes = it->second;

        AABB modelAABB; // Start with invalid AABB
        glm::mat4 instanceTransform = meshes[0]->transform; // All meshes should have same transform

        // Combine all mesh AABBs to create model AABB
        for (const auto* renderable : meshes) {
            AABB meshAABB = AABB::CreateFromMesh(renderable->mesh);
            modelAABB.ExpandToInclude(meshAABB);
        }

        // Create debug color based on instance position
        glm::vec3 debugColor = glm::vec3(1.0f, 0.0f, 0.0f);

        // Submit model instance AABB to frame data
        frameData.debugAABBs.emplace_back(modelAABB, instanceTransform, debugColor);
    }
}

void GraphicsTestDriver::UpdateInstanceTransforms()
{
    // Only rotate if rotation is enabled
    if (!m_RotationEnabled || m_SceneObjects.empty()) {
        return;
    }

    // Find the first tinbox instance position to use as the rotation center
    glm::vec3 tinboxPosition;
    bool foundTinboxPosition = false;

    for (size_t i = 1; i < m_SceneObjects.size(); ++i) { // Start at 1 to skip ground plane
        if (m_SceneObjects[i].mesh) {
            // Extract position from the first tinbox mesh to use as rotation center
            glm::vec3 translation, scale, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            glm::decompose(m_SceneObjects[i].transform, scale, rotation, translation, skew, perspective);

            tinboxPosition = translation;
            foundTinboxPosition = true;
            break;
        }
    }

    if (!foundTinboxPosition) {
        return; // No suitable object found
    }

    // Create rotation based on time (combined rotations for interesting AABB changes)
    float rotationSpeed = 0.8f; // Rotation speed in radians per second
    glm::quat timeBasedRotation = glm::angleAxis(m_Time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f)) *
                                  glm::angleAxis(m_Time * rotationSpeed * 0.6f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                                  glm::angleAxis(m_Time * rotationSpeed * 0.4f, glm::vec3(0.0f, 0.0f, 1.0f));

    // Rotate ALL meshes of the first tinbox model together
    for (size_t i = 1; i < m_SceneObjects.size(); ++i) { // Start at 1 to skip ground plane
        if (m_SceneObjects[i].mesh) {
            auto& targetObject = m_SceneObjects[i];

            // Extract original position and scale from this mesh's transform
            glm::vec3 translation, scale, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            glm::decompose(targetObject.transform, scale, rotation, translation, skew, perspective);

            // Check if this mesh belongs to the same tinbox model (same position)
            float distanceToTinboxCenter = glm::length(translation - tinboxPosition);
            if (distanceToTinboxCenter < 0.1f) { // Meshes from the same model should be at the same position

                // Rebuild the transform matrix with the new rotation
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
                glm::mat4 rotationMatrix = glm::mat4_cast(timeBasedRotation);
                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

                // Apply transformations in order: Scale -> Rotate -> Translate
                targetObject.transform = translationMatrix * rotationMatrix * scaleMatrix;
            }
        }
    }
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
                    spdlog::info("Camera controls enabled");
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    spdlog::info("Camera controls disabled");
                }
                break;
                
            case GLFW_KEY_F2:
                s_Instance->PrintSceneInfo();
                break;

            case GLFW_KEY_F3:
                s_Instance->PrintRenderPassStatus();
                break;

            case GLFW_KEY_1:
                s_Instance->ToggleRenderPass("ShadowPass");
                break;

            case GLFW_KEY_3:
                s_Instance->ToggleRenderPass("DebugPass");
                break;

            case GLFW_KEY_4:
                s_Instance->ToggleAABBVisualization();
                break;

            case GLFW_KEY_5:
                s_Instance->m_RotationEnabled = !s_Instance->m_RotationEnabled;
                spdlog::info("Object rotation {}", s_Instance->m_RotationEnabled ? "ENABLED" : "DISABLED");
                break;

            case GLFW_KEY_6:
                s_Instance->ToggleSkybox();
                break;

                // NEW: Key 7 to toggle point shadow pass
            case GLFW_KEY_7:
                s_Instance->ToggleRenderPass("PointShadowPass");
                break;

            case GLFW_KEY_P:
                if (s_Instance->m_SceneRenderer) {
                    auto* pipeline = s_Instance->m_SceneRenderer->GetPipeline();
                    if (pipeline) {
                        bool isEnabled = pipeline->IsPassEnabled("PickingPass");
                        spdlog::info("Picking mode {} (Left-click objects to test)",
                                   isEnabled ? "already ENABLED" : "ENABLED");
                        if (!isEnabled) {
                            s_Instance->m_SceneRenderer->EnablePicking(true);
                        }
                    }
                }
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

void GraphicsTestDriver::PrintSystemInfo() const
{
    spdlog::info("=== Graphics System Information ===");
    spdlog::info("Window Size: {}x{}", m_Window->GetWidth(), m_Window->GetHeight());
    spdlog::info("Resource Manager Status: {}", m_ResourceManager ? "Active" : "Inactive");
    if (m_ResourceManager) {
        spdlog::info("Loaded Shaders: {}", m_ResourceManager->GetShaderCount());
        spdlog::info("Loaded Models: {}", m_ResourceManager->GetModelCount());
    }
    spdlog::info("=====================================");
}

void GraphicsTestDriver::PrintSceneInfo() const
{
    spdlog::info("=== Current Scene Information ===");
    spdlog::info("Objects: {}", m_SceneObjects.size());
    spdlog::info("Lights: {}", m_SceneLights.size());

    if (m_Camera) {
        auto pos = m_Camera->GetPosition();
        auto rot = m_Camera->GetRotation();
        spdlog::info("Camera Position: ({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
        spdlog::info("Camera Rotation: ({:.1f}°, {:.1f}°, {:.1f}°)", rot.x, rot.y, rot.z);
    }

    spdlog::info("Frame Time: {:.3f}ms (FPS: {:.1f})", m_DeltaTime * 1000.0f, 1.0f / m_DeltaTime);
    spdlog::info("===============================");
}

void GraphicsTestDriver::PrintRenderPassStatus() const
{
    spdlog::info("=== Render Pass Status ===");

    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            // Check status of common render passes
            std::vector<std::string> passes = {"ShadowPass", "MainPass"};

            for (const auto& passName : passes) {
                bool enabled = pipeline->IsPassEnabled(passName);
                spdlog::info("  {}: {}", passName, enabled ? "ENABLED" : "DISABLED");
            }
        } else {
            spdlog::warn("Pipeline not found!");
        }
    } else {
        spdlog::warn("Scene renderer not available!");
    }

    spdlog::info("==========================");
}

void GraphicsTestDriver::ToggleRenderPass(const std::string& passName)
{
    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            bool currentlyEnabled = pipeline->IsPassEnabled(passName);
            bool newState = !currentlyEnabled;

            pipeline->EnablePass(passName, newState);

            spdlog::info("Render pass '{}' {}", passName, newState ? "ENABLED" : "DISABLED");
        } else {
            spdlog::warn("Pipeline not found - cannot toggle pass '{}'", passName);
        }
    } else {
        spdlog::warn("Scene renderer not available - cannot toggle pass '{}'", passName);
    }
}

void GraphicsTestDriver::ToggleAABBVisualization()
{
    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(pipeline->GetPass("DebugPass"));
            if (debugPass) {
                bool currentlyEnabled = debugPass->GetShowAABBs();
                bool newState = !currentlyEnabled;

                debugPass->SetShowAABBs(newState);

                spdlog::info("AABB wireframe visualization {}", newState ? "ENABLED" : "DISABLED");
            } else {
                spdlog::warn("Debug pass not found - cannot toggle AABB visualization");
            }
        } else {
            spdlog::warn("Pipeline not found - cannot toggle AABB visualization");
        }
    } else {
        spdlog::warn("Scene renderer not available - cannot toggle AABB visualization");
    }
}

void GraphicsTestDriver::ToggleSkybox()
{
    if (m_SceneRenderer)
    {
        bool currentlyEnabled = m_SceneRenderer->IsSkyboxEnabled();
        bool newState = !currentlyEnabled;
        m_SceneRenderer->EnableSkybox(newState);
        spdlog::info("Skybox {}", newState ? "ENABLED" : "DISABLED");
    }
}

void GraphicsTestDriver::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (!s_Instance) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Get mouse position
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Handle object picking
        s_Instance->HandleObjectPicking(mouseX, mouseY);
    }
}

void GraphicsTestDriver::HandleObjectPicking(double mouseX, double mouseY)
{
    if (!m_SceneRenderer) return;

    // Get window size for viewport calculation
    int windowWidth, windowHeight;
    glfwGetWindowSize(m_Window->GetNativeWindow(), &windowWidth, &windowHeight);

    spdlog::info("Attempting to pick at screen position ({:.0f}, {:.0f})", mouseX, mouseY);

    // Enable picking temporarily
    m_SceneRenderer->EnablePicking(true);
    spdlog::info("Picking enabled, rendering picking frame...");

    // Render a picking frame (this will execute the picking pass)
    m_SceneRenderer->Render();
    spdlog::info("Picking frame rendered, querying result...");

    // Create picking query
    MousePickingQuery query;
    query.screenX = static_cast<int>(mouseX);
    query.screenY = static_cast<int>(mouseY);
    query.viewportWidth = windowWidth;
    query.viewportHeight = windowHeight;

    // Query the picking result
    PickingResult result = m_SceneRenderer->QueryObjectPicking(query);
    spdlog::info("Query completed, object ID: {}, hasHit: {}", result.objectID, result.hasHit);

    // Disable picking after use
    m_SceneRenderer->EnablePicking(false);

    // Handle the result
    if (result.hasHit) {
        spdlog::info("PICKED OBJECT: ID = {}, World Position = ({:.2f}, {:.2f}, {:.2f}), Depth = {:.3f}",
                    result.objectID,
                    result.worldPosition.x, result.worldPosition.y, result.worldPosition.z,
                    result.depth);

        // You could add additional logic here, such as:
        // - Highlighting the selected object
        // - Storing the selected object ID
        // - Triggering editor actions

    } else {
        spdlog::info("No object picked at screen position ({:.0f}, {:.0f})", mouseX, mouseY);
    }
}


// Main entry point
int main()
{
    GraphicsTestDriver driver;
    
    if (!driver.Initialize()) {
        spdlog::error("Failed to initialize graphics test driver!");
        return -1;
    }

    driver.Run();
    driver.Shutdown();
    
    return 0;
}