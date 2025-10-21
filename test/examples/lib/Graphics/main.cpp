/*
 * ===============================================
 * GRAPHICS TEST DRIVER - CONFIGURATION GUIDE
 * ===============================================
 *
 * DEMO SELECTION:
 * ---------------
 * In Initialize(), choose which demo to run by uncommenting ONE:
 *
 *   SetupSponzaDemo();   // Sponza cathedral - LIGHTING & HDR TEST
 *                        // - Animated point light (intensity 5.0)
 *                        // - HDR tone mapping
 *                        // - Camera positioned inside cathedral
 *                        // - NO OUTLINES (lighting test only)
 *
 *   SetupTinboxDemo();   // Tinbox grid - OUTLINE & PBR TEST
 *                        // - Directional + point lights
 *                        // - CLICK-BASED OUTLINES (left-click to select)
 *                        // - Camera positioned to view grid
 *
 * Each demo function sets up:
 *   - Scene objects
 *   - Lighting
 *   - Camera position/orientation
 *   - Outline mode (if applicable)
 *
 * OUTLINE SELECTION:
 * ------------------
 * - Click-based: Left-click on any object to outline the entire model
 * - Click on empty space to clear outlines
 * - All meshes of the same model instance will be outlined together
 *
 * ===============================================
 */

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
#include <algorithm>
#include <cfloat>

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
    , m_ActiveDemo(DemoType::Tinbox)  // Default to Tinbox
    , m_Time(0.0f)
    , m_DeltaTime(0.0f)
    , m_CameraEnabled(false)
    , m_FirstMouse(true)
    , m_LastX(640.0f)
    , m_LastY(360.0f)
    , m_RotationEnabled(false)
    , m_HDREnabled(false)  // HDR starts disabled
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

    // Enable VSync to prevent screen tearing and black flashes
    m_Window->SetVSync(true);
    spdlog::info("VSync enabled");

    // Create scene renderer (owns all graphics systems now)
    m_SceneRenderer = std::make_unique<SceneRenderer>();

    // Get references to systems owned by SceneRenderer
    m_ResourceManager = m_SceneRenderer->GetResourceManager();

    // Enable HDR tone mapping pipeline (matches ogldev tutorial 63)
    auto* pipeline = m_SceneRenderer->GetPipeline();
    if (pipeline) {
        pipeline->EnablePass("HDRResolvePass", true);
        pipeline->EnablePass("HDRLuminancePass", true);
        pipeline->EnablePass("ToneMapPass", true);
        m_HDREnabled = true;  // Update state variable
        spdlog::info("HDR tone mapping pipeline enabled (matching ogldev tutorial 63)");
    }

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

    // ===== DEMO SELECTION =====
    // Uncomment ONE demo to run:

    //SetupSponzaDemo();     // Sponza cathedral - lighting/HDR test
    SetupTinboxDemo();       // Tinbox grid - outline/PBR test
    
    

    // Print system info
    PrintSystemInfo();

    m_LastFrameTime = std::chrono::steady_clock::now();

    spdlog::info("Graphics system initialized successfully!");
    return true;
}

void GraphicsTestDriver::Run()
{
    spdlog::info("Starting render loop...");
    spdlog::info("=== GRAPHICS DEMO ===");
    spdlog::info("Controls:");
    spdlog::info("  - WASD: Move camera");
    spdlog::info("  - Mouse: Look around");
    spdlog::info("  - ESC: Exit");
    spdlog::info("  - F1: Toggle camera controls");
    spdlog::info("  - F2: Print scene info");
    spdlog::info("  - F3: Print render pass status");
    spdlog::info("  - 1: Toggle shadow pass (directional)");
    spdlog::info("  - 2: Toggle outline pass");
    spdlog::info("  - 3: Toggle debug pass (light visualization)");
    spdlog::info("  - 4: Toggle AABB wireframes");
    spdlog::info("  - 5: Toggle object rotation");
    spdlog::info("  - 6: Toggle skybox");
    spdlog::info("  - 7: Toggle point shadow pass");
    spdlog::info("  - 8: Print point shadow info");
    spdlog::info("  - 9: Print HDR statistics (exposure, luminance)");
    spdlog::info("  - B: Toggle spot shadow pass");
    spdlog::info("  - O: Toggle HDR pipeline (all HDR passes on/off)");
    spdlog::info("  - H: Toggle HDR auto-exposure only");
    spdlog::info("  - T: Toggle tone mapping only");
    spdlog::info("  - R: Toggle HDR resolve only");
    spdlog::info("  - M: Cycle tone mapping method (None/Reinhard/ACES/Exposure)");

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

        // Animate point light position (only in Sponza demo)
        if (m_ActiveDemo == DemoType::Sponza &&
            !m_SceneLights.empty() && m_SceneLights[0].type == Light::Type::Point) {
            // Animate X position: oscillates between -60 and +70 (sponza corridor)
            float animatedX = (cosf(m_Time * 0.3f) + 1.0f) * 65.0f - 60.0f;
            m_SceneLights[0].position.x = animatedX;
        }

        // Submit lights and objects each frame
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

        // ===== OUTLINE UPDATE =====
        // Outline selection is now CLICK-BASED via HandleObjectPicking()
        // Left-click on an object to outline the entire model instance
        // Click on empty space to clear outlines
        // No need for per-frame camera-based updates anymore!

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

        // Load INSTANCED shadow mapping depth-only shader (SSBO-based)
        auto shadowShader = m_ResourceManager->LoadShader("shadow_depth_instanced",
            "assets/shaders/shadow_depth_instanced.vert",
            "assets/shaders/shadow_depth_instanced.frag");

        if (shadowShader) {
            spdlog::info("Instanced shadow mapping shader loaded successfully!");
            // Configure the shadow mapping pass with the instanced shader
            m_SceneRenderer->SetShadowDepthShader(shadowShader);
            // Configure spot shadow mapping pass (reuses same instanced shader)
            m_SceneRenderer->SetSpotShadowShader(shadowShader);
            spdlog::info("Shadow passes configured with GPU instancing (supports multiple spot lights)");
        } else {
            spdlog::warn("Could not load instanced shadow mapping shader");
        }

        // Load INSTANCED point shadow mapping shader (geometry shader + SSBO)
        auto pointShadowShader = m_ResourceManager->LoadShaderWithGeometry("point_shadow_instanced",
            "assets/shaders/point_shadow_instanced.vert",
            "assets/shaders/point_shadow_instanced.frag",
            "assets/shaders/point_shadow_instanced.geom");

        if (pointShadowShader) {
            spdlog::info("Instanced point shadow mapping shader loaded successfully!");
            // Configure the point shadow mapping pass with the loaded shader
            m_SceneRenderer->SetPointShadowShader(pointShadowShader);
        } else {
            spdlog::warn("Could not load instanced point shadow mapping shader");
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

        // Load outline shader for stencil-based outlines
        auto outlineShader = m_ResourceManager->LoadShader("outline",
            "assets/shaders/outline.vert",
            "assets/shaders/outline.frag");

        if (outlineShader) {
            spdlog::info("Outline shader loaded successfully!");
            // Configure the outline pass with the loaded shader
            m_SceneRenderer->SetOutlineShader(outlineShader);
        } else {
            spdlog::warn("Could not load outline shader");
        }

        // ===== Load HDR Compute Shader =====
        spdlog::info("Loading HDR compute shader...");
        auto hdrComputeShader = m_ResourceManager->LoadComputeShader(
            "hdr_luminance",
            "assets/shaders/hdr_luminance.comp"
        );

        if (hdrComputeShader) {
            spdlog::info("HDR compute shader loaded successfully!");
            m_SceneRenderer->SetHDRComputeShader(hdrComputeShader);
        } else {
            spdlog::warn("Could not load HDR compute shader");
        }

        // ===== Load Tone Mapping Shader =====
        spdlog::info("Loading tone mapping shader...");
        auto toneMappingShader = m_ResourceManager->LoadShader(
            "tonemap",
            "assets/shaders/tonemap.vert",
            "assets/shaders/tonemap.frag"
        );

        if (toneMappingShader) {
            spdlog::info("Tone mapping shader loaded successfully!");
            m_SceneRenderer->SetToneMappingShader(toneMappingShader);
        } else {
            spdlog::warn("Could not load tone mapping shader");
        }

        // Load models
        auto tinBoxModel = m_ResourceManager->LoadModel("tinbox",
            "assets/models/tinbox/tin_box.obj");

        if (!tinBoxModel) {
            spdlog::error("Failed to load tin box model!");
            return false;
        }

        // Load Crytek Sponza model
        auto sponzaModel = m_ResourceManager->LoadModel("sponza",
            "assets/models/crytek_sponza/sponza.obj");

        if (!sponzaModel) {
            spdlog::error("Failed to load Sponza model!");
            return false;
        } else {
            spdlog::info("Sponza model loaded successfully with {} meshes", sponzaModel->meshes.size());
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

// =============================================================================================
// DEMO SETUP FUNCTIONS
// =============================================================================================
// Each demo function is a complete, self-contained setup that configures:
//   1. Scene objects
//   2. Lighting
//   3. Camera position and orientation
//   4. Outline mode (if applicable)
//
// DEMO 1 - Sponza:  Lighting/HDR test - NO outlines
// DEMO 2 - Tinbox:  Outline/PBR test - Camera-based outlines
//
// To switch demos, just uncomment one function call in Initialize()
// =============================================================================================

// ===== DEMO 1: SPONZA CATHEDRAL - LIGHTING & HDR TEST =====
void GraphicsTestDriver::SetupSponzaDemo()
{
    m_ActiveDemo = DemoType::Sponza;
    spdlog::info("=== SETTING UP SPONZA DEMO (Lighting & HDR Test) ===");

    // 1. CREATE CAMERA
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(59.0f, 9.0f, -1.6f));  // Inside cathedral
    glm::vec3 direction = glm::normalize(glm::vec3(-1.0f, 0.05f, 0.07f));
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));
    m_Camera->SetRotation(glm::vec3(pitch, yaw, 0.0f));
    spdlog::info("Camera positioned inside Sponza cathedral");

    // 2. CREATE SCENE OBJECTS
    CreateModelInstance("sponza", "WhiteMaterial", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.05f));
    spdlog::info("Sponza model loaded: {} meshes", m_SceneObjects.size());

    // 3. CREATE LIGHTS
    // Animated point light (ogldev tutorial 63 setup)
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(0.0f, 0.5f, -1.6f),        // Initial position (will be animated)
        glm::vec3(1.0f, 1.0f, 1.0f),         // Color: white
        5.0f,                                 // DiffuseIntensity: 5.0 (high for HDR)
        0.2f,                                 // AmbientIntensity: 0.2
        100.0f                                // Range: large for Sponza
    ));
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.07f, 0.07f, 0.07f));
    spdlog::info("Animated point light created (intensity 5.0 for HDR demo)");

    spdlog::info("Sponza demo setup complete: {} objects, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: This demo focuses on LIGHTING/HDR - NO OUTLINES");
    spdlog::info("      In Run() loop, COMMENT OUT UpdateCameraBasedOutline()");
}

// ===== DEMO 2: TINBOX GRID - OUTLINE & PBR TEST =====
void GraphicsTestDriver::SetupTinboxDemo()
{
    m_ActiveDemo = DemoType::Tinbox;
    spdlog::info("=== SETTING UP TINBOX DEMO ===");

    // 1. CREATE CAMERA
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(-15.0f, 3.0f, 5.0f));  // View grid from angle
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, -0.2f, -0.3f));
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));
    m_Camera->SetRotation(glm::vec3(pitch, yaw, 0.0f));
    spdlog::info("Camera positioned to view tinbox grid");

    // 2. CREATE SCENE OBJECTS
    // Ground plane
    auto planeMesh = std::make_shared<Mesh>(
        PrimitiveGenerator::CreatePlane(30.0f, 30.0f, 10, 10)
    );
    RenderableData ground;
    ground.mesh = planeMesh;
    ground.material = m_ResourceManager->GetMaterial("WhiteMaterial");
    ground.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    ground.visible = true;
    ground.objectID = 1;
    m_SceneObjects.push_back(ground);

    // Tinbox grid
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial",
                                          "GoldMaterial", "WhiteMaterial"};
    const int gridSize = 3;
    const float spacing = 3.0f;
    const float startOffset = -(gridSize - 1) * spacing * 0.5f;

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            glm::vec3 position(startOffset + x * spacing - 8.0f, 0.0f, startOffset + z * spacing);
            int materialIndex = (x + z) % materials.size();
            CreateModelInstance("tinbox", materials[materialIndex], position, glm::vec3(1.0f));
        }
    }
    spdlog::info("Tinbox grid created: {} objects", m_SceneObjects.size());

    // 3. CREATE LIGHTS
    // Directional light
    /*m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(0.2f, -1.0f, 0.3f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        0.8f, 0.2f
    ));*/
    // Point light
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(1.0f, 0.9f, 0.7f),
        2.0f, 0.1f, 50.0f
    ));
    // Spot light - positioned directly above tinbox grid center
    //m_SceneLights.push_back(CreateSpotLight(
    //    glm::vec3(-8.0f, 8.0f, 0.0f),         // Position: centered above grid at (-8, 8, 0)
    //    glm::vec3(0.0f, -1.0f, 0.0f),         // Direction: pointing straight down
    //    glm::vec3(1.0f, 0.8f, 0.6f),          // Color: warm white/yellow
    //    2.5f,                                  // DiffuseIntensity: bright for visible shadows
    //    0.1f,                                  // AmbientIntensity: low ambient
    //    30.0f,                                 // Range: covers tinbox grid
    //    15.0f,                                 // InnerCone: 15 degrees (sharp falloff)
    //    25.0f                                  // OuterCone: 25 degrees (cone angle)
    //));
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    spdlog::info("Lights created: 1 directional, 1 point, 1 spot");

    // 4. SETUP OUTLINE MODE - Click-based selection
    m_SceneRenderer->ClearOutlinedObjects();
    spdlog::info("Click-based outline mode enabled (left-click to select objects)");

    spdlog::info("Tinbox demo setup complete: {} objects, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: Left-click on any tinbox to outline the entire model (all meshes)");
    spdlog::info("NOTE: Press 'B' to toggle spot light shadows (spotlight centered above grid at [-8, 8, 0])");
}

void GraphicsTestDriver::CreateModelInstance(const std::string& modelName, const std::string& materialName,
                                            const glm::vec3& position, const glm::vec3& scale)
{
    auto model = m_ResourceManager->GetModel(modelName);
    if (!model || model->meshes.empty()) {
        return;
    }

    // Assign a unique model instance ID for this model instance (shared by all meshes)
    static uint32_t nextModelInstanceID = 1000;
    uint32_t thisModelInstanceID = nextModelInstanceID++;

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

        // Assign the model instance ID (shared by all meshes of this model)
        renderable.modelInstanceID = thisModelInstanceID;

        //spdlog::info("Created object with ID: {}, ModelInstanceID: {}", renderable.objectID, renderable.modelInstanceID);

        m_SceneObjects.push_back(renderable);
    }
}



SubmittedLightData GraphicsTestDriver::CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                                                              float diffuseIntensity, float ambientIntensity)
{
    SubmittedLightData light;
    light.type = Light::Type::Directional;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.diffuseIntensity = diffuseIntensity;    // Ogldev-style diffuse intensity
    light.ambientIntensity = ambientIntensity;     // Ogldev-style per-light ambient
    light.enabled = true;
    return light;
}

SubmittedLightData GraphicsTestDriver::CreatePointLight(const glm::vec3& position, const glm::vec3& color,
                                                         float diffuseIntensity, float ambientIntensity, float range)
{
    SubmittedLightData light;
    light.type = Light::Type::Point;
    light.position = position;
    light.color = color;
    light.diffuseIntensity = diffuseIntensity;     // Ogldev-style diffuse intensity
    light.ambientIntensity = ambientIntensity;     // Ogldev-style per-light ambient
    light.range = range;
    light.enabled = true;
    return light;
}

SubmittedLightData GraphicsTestDriver::CreateSpotLight(const glm::vec3& position, const glm::vec3& direction,
                                                       const glm::vec3& color, float diffuseIntensity,
                                                       float ambientIntensity, float range,
                                                       float innerCone, float outerCone)
{
    SubmittedLightData light;
    light.type = Light::Type::Spot;
    light.position = position;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.diffuseIntensity = diffuseIntensity;     // Ogldev-style diffuse intensity
    light.ambientIntensity = ambientIntensity;     // Ogldev-style per-light ambient
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

            case GLFW_KEY_2:
                s_Instance->ToggleRenderPass("OutlinePass");
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

            case GLFW_KEY_7:
                s_Instance->ToggleRenderPass("PointShadowPass");
                break;

            case GLFW_KEY_8:
                s_Instance->PrintPointShadowInfo();
                break;

            case GLFW_KEY_9:
                s_Instance->PrintHDRInfo();
                break;

            case GLFW_KEY_O:
                s_Instance->ToggleHDRPipeline();
                break;

            case GLFW_KEY_H:
                s_Instance->ToggleRenderPass("HDRLuminancePass");
                break;

            case GLFW_KEY_T:
                s_Instance->ToggleRenderPass("ToneMapPass");
                break;

            case GLFW_KEY_R:
                s_Instance->ToggleRenderPass("HDRResolvePass");
                break;

            case GLFW_KEY_M:  // Cycle through tone mapping methods
                if (s_Instance->m_SceneRenderer) {
                    auto* pipeline = s_Instance->m_SceneRenderer->GetPipeline();
                    if (pipeline) {
                        auto toneMapPass = std::dynamic_pointer_cast<ToneMapRenderPass>(
                            pipeline->GetPass("ToneMapPass")
                        );
                        if (toneMapPass) {
                            int current = static_cast<int>(toneMapPass->GetMethod());
                            int next = (current + 1) % 4;  // Cycle 0-3
                            toneMapPass->SetMethod(static_cast<ToneMapRenderPass::Method>(next));

                            const char* names[] = {"None", "Reinhard", "ACES", "Exposure"};
                            spdlog::info("Tone mapping method: {} ({})", names[next], next);
                        }
                    }
                }
                break;

            case GLFW_KEY_B:
                s_Instance->ToggleRenderPass("SpotShadowPass");
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
            // Check status of all render passes
            std::vector<std::string> passes = {
                "ShadowPass",
                "PointShadowPass",
                "SpotShadowPass",
                "MainPass",
                "HDRResolvePass",
                "HDRLuminancePass",
                "ToneMapPass",
                "DebugPass",
                "EditorResolvePass",
                "PickingPass",
                "PresentPass"
            };

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

void GraphicsTestDriver::ToggleHDRPipeline()
{
    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            // Toggle the state
            m_HDREnabled = !m_HDREnabled;

            // Apply to all HDR-related passes
            pipeline->EnablePass("HDRResolvePass", m_HDREnabled);
            pipeline->EnablePass("HDRLuminancePass", m_HDREnabled);
            pipeline->EnablePass("ToneMapPass", m_HDREnabled);

            spdlog::info("HDR Pipeline {} (Resolve + Luminance + ToneMap)",
                        m_HDREnabled ? "ENABLED" : "DISABLED");
        } else {
            spdlog::warn("Pipeline not found - cannot toggle HDR pipeline");
        }
    } else {
        spdlog::warn("Scene renderer not available - cannot toggle HDR pipeline");
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

void GraphicsTestDriver::PrintPointShadowInfo() const
{
    spdlog::info("=== Point Shadow Information ===");

    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            bool passEnabled = pipeline->IsPassEnabled("PointShadowPass");
            spdlog::info("  Point Shadow Pass: {}", passEnabled ? "ENABLED" : "DISABLED");

            // Count point lights
            int pointLightCount = 0;
            for (const auto& light : m_SceneLights) {
                if (light.enabled && light.type == Light::Type::Point) {
                    pointLightCount++;
                    spdlog::info("  Point Light #{}: Position({:.1f}, {:.1f}, {:.1f}), Intensity={:.1f}, Range={:.1f}",
                                pointLightCount,
                                light.position.x, light.position.y, light.position.z,
                                light.intensity, light.range);
                }
            }

            spdlog::info("  Total Point Lights: {}", pointLightCount);

            auto& frameData = m_SceneRenderer->GetFrameData();
            spdlog::info("  Point Shadow Cubemaps Generated: {}", frameData.pointShadowCubemaps.size());

            for (size_t i = 0; i < frameData.pointShadowCubemaps.size(); ++i) {
                spdlog::info("    Cubemap {}: TextureID={}, FarPlane={:.1f}",
                            i, frameData.pointShadowCubemaps[i],
                            i < frameData.pointShadowFarPlanes.size() ? frameData.pointShadowFarPlanes[i] : 0.0f);
            }
        } else {
            spdlog::warn("Pipeline not found!");
        }
    } else {
        spdlog::warn("Scene renderer not available!");
    }

    spdlog::info("================================");
}

void GraphicsTestDriver::PrintHDRInfo() const
{
    spdlog::info("=== HDR Information ===");

    if (m_SceneRenderer) {
        auto* pipeline = m_SceneRenderer->GetPipeline();
        if (pipeline) {
            // Check HDR pass status
            bool hdrLumEnabled = pipeline->IsPassEnabled("HDRLuminancePass");
            bool toneMapEnabled = pipeline->IsPassEnabled("ToneMapPass");
            bool hdrResolveEnabled = pipeline->IsPassEnabled("HDRResolvePass");

            spdlog::info("  HDR Resolve Pass: {}", hdrResolveEnabled ? "ENABLED" : "DISABLED");
            spdlog::info("  HDR Luminance Pass (Auto-Exposure): {}", hdrLumEnabled ? "ENABLED" : "DISABLED");
            spdlog::info("  Tone Mapping Pass: {}", toneMapEnabled ? "ENABLED" : "DISABLED");

            // Note: HDR values (exposure, avgLuminance) are calculated during render
            // and are part of the temporary RenderContext. To display them here,
            // we'd need to store them in FrameData.
            spdlog::info("");
            spdlog::info("  Note: HDR exposure and luminance values are calculated");
            spdlog::info("  dynamically during rendering. Check console output during");
            spdlog::info("  frame rendering for real-time values.");

        } else {
            spdlog::warn("Pipeline not found!");
        }
    } else {
        spdlog::warn("Scene renderer not available!");
    }

    spdlog::info("=======================");
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

    //spdlog::info("Attempting to pick at screen position ({:.0f}, {:.0f})", mouseX, mouseY);

    // Enable picking temporarily
    m_SceneRenderer->EnablePicking(true);
    //spdlog::info("Picking enabled, rendering picking frame...");

    // Render a picking frame (this will execute the picking pass)
    m_SceneRenderer->Render();
    //spdlog::info("Picking frame rendered, querying result...");

    // Create picking query
    MousePickingQuery query;
    query.screenX = static_cast<int>(mouseX);
    query.screenY = static_cast<int>(mouseY);
    query.viewportWidth = windowWidth;
    query.viewportHeight = windowHeight;

    // Query the picking result
    PickingResult result = m_SceneRenderer->QueryObjectPicking(query);
    //spdlog::info("Query completed, object ID: {}, hasHit: {}", result.objectID, result.hasHit);

    // Disable picking after use
    m_SceneRenderer->EnablePicking(false);

    // Handle the result
    if (result.hasHit) {
        spdlog::info("PICKED OBJECT: ID = {}, World Position = ({:.2f}, {:.2f}, {:.2f}), Depth = {:.3f}",
                    result.objectID,
                    result.worldPosition.x, result.worldPosition.y, result.worldPosition.z,
                    result.depth);

        // Find the modelInstanceID of the clicked object
        uint32_t clickedModelInstanceID = 0;
        for (const auto& renderable : m_SceneObjects) {
            if (renderable.objectID == result.objectID) {
                clickedModelInstanceID = renderable.modelInstanceID;
                break;
            }
        }

        if (clickedModelInstanceID != 0) {
            // Clear previous outlines
            m_SceneRenderer->ClearOutlinedObjects();

            // Find and outline all meshes with the same modelInstanceID
            for (const auto& renderable : m_SceneObjects) {
                if (renderable.modelInstanceID == clickedModelInstanceID) {
                    m_SceneRenderer->AddOutlinedObject(renderable.objectID);
                    spdlog::info("  -> Outlining mesh with objectID: {} (modelInstanceID: {})",
                               renderable.objectID, renderable.modelInstanceID);
                }
            }
        }

    } else {
        spdlog::info("No object picked at screen position ({:.0f}, {:.0f})", mouseX, mouseY);
        // Clear outlines when clicking empty space
        m_SceneRenderer->ClearOutlinedObjects();
    }
}

bool GraphicsTestDriver::RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                           const glm::vec3& aabbMin, const glm::vec3& aabbMax,
                                           float& tMin, float& tMax) const
{
    // Ray-AABB intersection using the slab method
    tMin = 0.0f;
    tMax = FLT_MAX;

    for (int i = 0; i < 3; i++) {
        if (abs(rayDir[i]) < 0.0001f) {
            // Ray is parallel to slab, check if origin is within bounds
            if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i]) {
                return false;
            }
        } else {
            // Compute intersection distances to near and far plane
            float t1 = (aabbMin[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (aabbMax[i] - rayOrigin[i]) / rayDir[i];

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax) {
                return false;
            }
        }
    }

    return true;
}

// =============================================================================================
// OUTLINE MODE FUNCTIONS
// =============================================================================================
// Two outline modes are available:
//
// 1. STATIC OUTLINE (SetupStaticOutlines):
//    - Call once in demo setup
//    - Outlines first N objects permanently
//    - Usage: Manual utility function (not used in current demos)
//
// 2. CAMERA-BASED OUTLINE (UpdateCameraBasedOutline):
//    - Call every frame in render loop
//    - Outlines object camera is pointing at (raycast from camera)
//    - Used in: SetupTinboxDemo()
//
// NOTE: SetupSponzaDemo() does NOT use outlines (lighting test only)
// =============================================================================================

// ===== OUTLINE UTILITY: STATIC (First N objects) =====
void GraphicsTestDriver::SetupStaticOutlines()
{
    if (!m_SceneRenderer) return;

    // Clear any existing outlines
    m_SceneRenderer->ClearOutlinedObjects();

    // Add first 5 objects to outline list
    for (size_t i = 0; i < std::min(size_t(5), m_SceneObjects.size()); ++i) {
        m_SceneRenderer->AddOutlinedObject(m_SceneObjects[i].objectID);
    }

    spdlog::info("Static outline mode: {} objects outlined",
                 std::min(size_t(5), m_SceneObjects.size()));
}

// ===== OUTLINE MODE: CAMERA-BASED (Dynamic raycast) =====
void GraphicsTestDriver::UpdateCameraBasedOutline()
{
    if (!m_Camera || !m_SceneRenderer) return;

    // Get camera ray
    glm::vec3 rayOrigin = m_Camera->GetPosition();
    glm::vec3 rayDir = m_Camera->GetFront();  // Camera uses GetFront() not GetForward()

    // Track closest hit
    float closestDistance = FLT_MAX;
    uint32_t hitObjectID = 0;
    bool foundHit = false;

    // Test ray against all objects
    for (const auto& renderable : m_SceneObjects) {
        if (!renderable.visible || !renderable.mesh) {
            continue;
        }

        // Calculate mesh AABB in local space
        AABB localAABB = AABB::CreateFromMesh(renderable.mesh);

        // Transform AABB to world space
        glm::vec3 aabbMin = glm::vec3(renderable.transform * glm::vec4(localAABB.min, 1.0f));
        glm::vec3 aabbMax = glm::vec3(renderable.transform * glm::vec4(localAABB.max, 1.0f));

        // Ensure min/max are correct after transformation
        glm::vec3 worldMin = glm::min(aabbMin, aabbMax);
        glm::vec3 worldMax = glm::max(aabbMin, aabbMax);

        // Test ray-AABB intersection
        float tMin, tMax;
        if (RayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, tMin, tMax)) {
            // Check if this is the closest hit
            if (tMin >= 0.0f && tMin < closestDistance) {
                closestDistance = tMin;
                hitObjectID = renderable.objectID;
                foundHit = true;
            }
        }
    }

    // Update outline system - clear all first
    m_SceneRenderer->ClearOutlinedObjects();

    // Add only the hit object
    if (foundHit) {
        m_SceneRenderer->AddOutlinedObject(hitObjectID);
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