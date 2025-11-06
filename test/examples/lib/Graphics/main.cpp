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
 *                        // - PER-NODE SELECTION (click individual objects like pillars/arches)
 *
 *   SetupTinboxDemo();   // Tinbox grid - OUTLINE & PBR TEST
 *                        // - Directional + point lights
 *                        // - WHOLE-MODEL SELECTION (click selects entire tinbox, not just top/bottom)
 *                        // - Camera positioned to view grid
 *
 *   SetupEditorDemo();   // 3x3 cube grid - EDITOR COMPARISON TEST
 *                        // - 9 cubes in 3x3 grid (matching editor scene)
 *                        // - Directional light (intensity 2.5)
 *                        // - Strong ambient lighting (0.7, 0.7, 0.7)
 *                        // - Camera at (0, 5, 10) looking at origin
 *                        // - Use this to compare with editor output
 *
 *   SetupTransparencyDemo();  // Transparency test - LEARNOPENGL STYLE
 *                             // - Multiple transparent windows at different depths
 *                             // - Tests batch-level sorting for transparency
 *                             // - Opaque cubes for reference
 *                             // - Move camera to test sorting from different angles
 *
 * Each demo function sets up:
 *   - Scene objects
 *   - Lighting
 *   - Camera position/orientation
 *   - Outline mode (if applicable)
 *
 * OUTLINE SELECTION:
 * ------------------
 * - Click-based: Left-click on any object to outline it
 * - Click on empty space to clear outlines
 * - Sponza: Per-node selection (individual objects like pillars/arches)
 * - Tinbox: Whole-model selection (all meshes outlined together)
 *
 * ===============================================
 */

#include "main.h"

#include <Rendering/InstancedRenderer.h>
#include <Resources/Material.h>
#include <Resources/Mesh.h>
#include <Resources/Model.h>
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Texture.h>
#include <Utility/Light.h>
#include <Utility/AABB.h>
#include <Utility/HUDData.h>
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
    , m_AABBsCached(false)
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

    // Setup resize callback to render during window resize (prevents black flashes)
    m_Window->SetResizeCallback([this]() {
        // Simple render during resize - reuse last frame's data
        if (m_SceneRenderer && m_Camera) {
            m_SceneRenderer->Render();
            m_Window->SwapBuffers();
        }
    });
    spdlog::info("Window resize callback registered for smooth resizing");

    // Get references to systems owned by SceneRenderer
    m_ResourceManager = m_SceneRenderer->GetResourceManager();

    // Enable HDR tone mapping pipeline (matches ogldev tutorial 63)
    m_SceneRenderer->ToggleHDRPipeline(true);
    m_HDREnabled = true;  // Update state variable
    spdlog::info("HDR tone mapping pipeline enabled (matching ogldev tutorial 63)");

    // Configure background color (visible when skybox is disabled)
    // Options: White (1,1,1), Black (0,0,0), Gray (0.7,0.7,0.7), Sky Blue (0.53,0.81,0.92)
    m_SceneRenderer->SetBackgroundColor(glm::vec4(0.f, 0.f, 0.f, 1.0f));  // Default gray
    spdlog::info("Background clear color set to gray (0.7, 0.7, 0.7)");

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
    //SetupTinboxDemo();     // Tinbox grid - outline/PBR test
    //SetupEditorDemo();       // 3x3 cube grid - matches editor scene
    SetupTransparencyDemo();  // Transparency test - like LearnOpenGL

    // Load HUD test textures (once during initialization)
    m_PauseMenuTexture = TextureLoader::TextureFromFile("PauseMenu.png", "assets/hud/Pause", false);
    m_ResumeButtonTexture = TextureLoader::TextureFromFile("RESUME_NEW.png", "assets/hud/Pause", false);
    spdlog::info("HUD test textures loaded: pause={}, resume={}", m_PauseMenuTexture, m_ResumeButtonTexture);

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

        // Animate directional light (daylight cycle - sun rotating across the sky)
        if (m_ActiveDemo == DemoType::Sponza &&
            !m_SceneLights.empty() && m_SceneLights[0].type == Light::Type::Directional) {
            // Rotate sun around the scene (full cycle in ~60 seconds)
            float cycleSpeed = 0.1f;  // Radians per second (0.1 = slow, 1.0 = fast)
            float angle = m_Time * cycleSpeed;

            // Sun rotates in XY plane (from east to west across the sky)
            // When angle = 0°: sun at horizon (east)
            // When angle = 90°: sun at zenith (noon, directly above)
            // When angle = 180°: sun at horizon (west)
            float sunX = cosf(angle) * 0.5f;   // Horizontal movement (east-west)
            float sunY = -sinf(angle);         // Vertical movement (below horizon to zenith)
            float sunZ = -0.2f;                // Slight north-south bias

            m_SceneLights[0].direction = glm::normalize(glm::vec3(sunX, sunY, sunZ));
        }

        // Submit lights and objects each frame
        for (const auto& light : m_SceneLights) {
            m_SceneRenderer->SubmitLight(light);
        }
        for (const auto& renderable : m_SceneObjects) {
            m_SceneRenderer->SubmitRenderable(renderable);
        }

        // Submit HUD test elements (using pre-loaded textures)
        // Test with pause menu background
        HUDElementData pauseBg;
        pauseBg.textureID = m_PauseMenuTexture;
        pauseBg.position = glm::vec2(1280.0f / 2.0f, 720.0f / 2.0f);  // Center of screen
        pauseBg.size = glm::vec2(512.0f, 384.0f);
        pauseBg.anchor = HUDAnchor::Center;
        pauseBg.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);  // Slightly transparent
        pauseBg.layer = 0;
        m_SceneRenderer->SubmitHUDElement(pauseBg);

        // Add resume button
        HUDElementData resumeBtn;
        resumeBtn.textureID = m_ResumeButtonTexture;
        resumeBtn.position = glm::vec2(1280.0f / 2.0f, 300.0f);
        resumeBtn.size = glm::vec2(256.0f, 64.0f);
        resumeBtn.anchor = HUDAnchor::Center;
        resumeBtn.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        resumeBtn.layer = 1;
        m_SceneRenderer->SubmitHUDElement(resumeBtn);

        // Update camera data manually
        if (m_Camera) {
            m_SceneRenderer->SetCameraData(
                m_Camera->GetViewMatrix(),
                m_Camera->GetProjectionMatrix(),
                m_Camera->GetPosition()
            );
        }

        // Update transformations (rotate one instance for AABB testing)
        UpdateInstanceTransforms();

        // Clear instance cache when transforms change (enables animation)
        if (m_RotationEnabled) {
            m_SceneRenderer->ClearInstanceCache();
            m_AABBsCached = false;  // Also invalidate AABB cache for rotating objects
        }

        // Calculate and submit debug AABBs for visualization (cached)
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

        // Load HUD shader for screen-space UI rendering
        auto hudShader = m_ResourceManager->LoadShader("hud",
            "assets/shaders/hud.vert",
            "assets/shaders/hud.frag");

        if (hudShader) {
            spdlog::info("HUD shader loaded successfully!");
            // Configure the HUD pass with the loaded shader
            m_SceneRenderer->SetHUDShader(hudShader);
            // Enable HUD rendering
            m_SceneRenderer->EnablePass("HUDPass", true);
        } else {
            spdlog::warn("Could not load HUD shader");
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
        auto tableModel = m_ResourceManager->LoadModel("table",
            "assets/models/table/Table.obj");

        if (!tableModel) {
            spdlog::error("Failed to load table model!");
            return false;
        }

        // Load Crytek Sponza model
        /*auto sponzaModel = m_ResourceManager->LoadModel("sponza",
            "assets/models/crytek_sponza/sponza.obj");

        if (!sponzaModel) {
            spdlog::error("Failed to load Sponza model!");
            return false;
        } else {
            spdlog::info("Sponza model loaded successfully with {} meshes", sponzaModel->meshes.size());
        }*/

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
    spdlog::info("=== SETTING UP SPONZA DEMO (Lighting & HDR Test with Per-Node Selection) ===");

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
    CreateModelInstance("sponza", "WhiteMaterial", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.05f), true);  // true = per-node selection
    spdlog::info("Sponza model loaded: {} meshes", m_SceneObjects.size());

    // 3. CREATE LIGHTS
    // Directional light (sunlight from above at steep angle, matching CryEngine)
    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(-0.3f, -0.8f, -0.2f),      // Direction: steep angle from above (like CryEngine Sponza)
        glm::vec3(1.0f, 0.95f, 0.9f),        // Color: warm sunlight
        2.5f,                                 // DiffuseIntensity: bright sunlight (increased for steeper angle)
        0.0f                                  // AmbientIntensity: no ambient (pure directional)
    ));
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.03f));  // Very low ambient for strong shadow contrast (like CryEngine)
    spdlog::info("Directional sunlight created (intensity 2.5) with DAYLIGHT CYCLE animation");

    spdlog::info("Sponza demo setup complete: {} objects, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: This demo uses PER-NODE SELECTION - click individual objects (pillars, arches, etc.)");
    spdlog::info("      Left-click on any object to outline it independently");
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

    // Table grid
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial",
                                          "GoldMaterial", "WhiteMaterial"};
    const int gridSize = 3;
    const float spacing = 3.0f;
    const float startOffset = -(gridSize - 1) * spacing * 0.5f;

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            glm::vec3 position(startOffset + x * spacing - 8.0f, 0.0f, startOffset + z * spacing);
            int materialIndex = (x + z) % materials.size();
            CreateModelInstance("table", materials[materialIndex], position, glm::vec3(0.01f));
        }
    }
    spdlog::info("Table grid created: {} objects", m_SceneObjects.size());

    // 3. CREATE LIGHTS
    // Directional light
    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(-0.3f, -0.8f, -0.2f),      // Direction: steep angle from above (like CryEngine Sponza)
        glm::vec3(1.0f, 0.95f, 0.9f),        // Color: warm sunlight
        2.5f,                                 // DiffuseIntensity: bright sunlight (increased for steeper angle)
        0.0f                                  // AmbientIntensity: no ambient (pure directional)
    ));
    // Point light - positioned close to chair for visible lighting
    //m_SceneLights.push_back(CreatePointLight(
    //    glm::vec3(-8.0f, 8.0f, 0.0f),          // Right above chair at (-8, 0, 0) Position
    //    glm::vec3(1.0f, 0.9f, 0.7f),           // Color
    //    5.0f, 0.0f, 50.0f                      // Diffuse, Ambient, range
    //));
    // Spot light - positioned close to chair for visible shadows
    //m_SceneLights.push_back(CreateSpotLight(
    //    glm::vec3(-8.0f, 3.0f, 0.0f),         // Lowered from 8.0 to 3.0 for stronger effect
    //    glm::vec3(0.0f, -1.0f, 0.0f),         // Direction: pointing straight down
    //    glm::vec3(1.0f, 0.8f, 0.6f),          // Color: warm white/yellow
    //    8.0f,                                  // Increased intensity to 8.0
    //    0.0f,                                  // AmbientIntensity: low ambient
    //    30.0f,                                 // Range: covers chair area
    //    15.0f,                                 // InnerCone: 15 degrees (sharp falloff)
    //    25.0f                                  // OuterCone: 25 degrees (cone angle)
    //));
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.03f)); // Reduced to let other lights show
    //spdlog::info("Lights created: 1 directional, 1 point, 1 spot");

    //m_SceneRenderer->EnableSkybox(false);

    // 4. SETUP OUTLINE MODE - Click-based selection
    m_SceneRenderer->ClearOutlinedObjects();
    spdlog::info("Click-based outline mode enabled (left-click to select objects)");

    spdlog::info("Tinbox demo setup complete: {} objects, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: This demo uses WHOLE-MODEL SELECTION - clicking tinbox outlines all meshes together (top + bottom)");
    spdlog::info("NOTE: Press 'B' to toggle spot light shadows (spotlight centered above grid at [-8, 8, 0])");
}

// ===== DEMO 3: EDITOR DEMO - 3X3 CUBE GRID MATCHING EDITOR SETUP =====
void GraphicsTestDriver::SetupEditorDemo()
{
    m_ActiveDemo = DemoType::Tinbox;  // Reuse Tinbox enum for now
    spdlog::info("=== SETTING UP EDITOR DEMO (3x3 Cube Grid) ===");

    // 1. CREATE CAMERA - Match editor camera setup
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 5.0f, 10.0f));  // Editor camera position
    // Set rotation to look towards origin from (0,5,10): pitch down, yaw to face -Z direction
    m_Camera->SetRotation(glm::vec3(-25.0f, -90.0f, 0.0f));
    spdlog::info("Camera positioned at (0, 5, 10) looking at origin");

    // 2. CREATE 3x3 CUBE GRID
    // Same colors as editor (Red, Green, Blue, Gold, White)
    std::vector<std::string> materials = {"RedMaterial", "GreenMaterial", "BlueMaterial",
                                          "GoldMaterial", "WhiteMaterial"};
    const int gridSize = 3;
    const float spacing = 150.0f;
    const float startOffset = -(gridSize - 1) * spacing * 0.5f;

    // Create cube mesh primitive
    auto cubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));

    // Use unique ID ranges for editor demo to avoid conflicts with other demos
    uint32_t objectID = 2000;
    uint32_t modelInstanceID = 3000;

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            glm::vec3 position(
                startOffset + x * spacing,
                0.0f,  // Ground level (matching editor)
                startOffset + z * spacing
            );

            // Cycle through materials based on position (matching editor logic)
            int materialIndex = (x + z) % materials.size();

            // Create renderable for this cube
            RenderableData cube;
            cube.mesh = cubeMesh;

            // Get material and create property block for color variation
            auto baseMaterial = m_ResourceManager->GetMaterial(materials[materialIndex]);
            cube.material = baseMaterial;

            // Create property block to override albedo (brighten color like editor does)
            auto propertyBlock = std::make_shared<MaterialPropertyBlock>();
            glm::vec3 baseColor = baseMaterial->GetAlbedoColor();
            propertyBlock->SetVec3("u_AlbedoColor", baseColor * 1.2f);  // Brighten like editor
            propertyBlock->SetFloat("u_MetallicValue", 0.0f);  // Non-metallic
            propertyBlock->SetFloat("u_RoughnessValue", 0.6f);  // Medium roughness
            cube.propertyBlock = propertyBlock;

            cube.transform = glm::translate(glm::mat4(1.0f), position);
            cube.visible = true;

            // Assign unique IDs (each cube is its own model instance)
            cube.objectID = objectID++;
            cube.modelInstanceID = modelInstanceID++;

            m_SceneObjects.push_back(cube);
        }
    }
    spdlog::info("Created 3x3 cube grid: {} cubes", gridSize * gridSize);

    // 2.5. CREATE GROUND PLANE TO CATCH SHADOWS
    auto planeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreatePlane(20.0f, 20.0f, 1, 1));
    RenderableData groundPlane;
    groundPlane.mesh = planeMesh;
    groundPlane.material = m_ResourceManager->GetMaterial("RedMaterial");

    // Create property block for the ground plane with a neutral gray color
    //auto groundPropertyBlock = std::make_shared<MaterialPropertyBlock>();
    //groundPropertyBlock->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 1.0f, 1.0f));  // Neutral gray
    //groundPropertyBlock->SetFloat("u_MetallicValue", 0.0f);  // Non-metallic
    //groundPropertyBlock->SetFloat("u_RoughnessValue", 0.8f);  // Fairly rough
    //groundPlane.propertyBlock = groundPropertyBlock;

    // Position plane below cubes (cubes are at y=0, so plane at y=-0.6 is below them)
    groundPlane.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    groundPlane.visible = true;
    groundPlane.objectID = objectID++;
    groundPlane.modelInstanceID = modelInstanceID++;

    m_SceneObjects.push_back(groundPlane);
    spdlog::info("Added ground plane at y=-0.6 to catch shadows");

    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(-0.3f, -0.8f, -0.2f),      // Direction: steep angle from above (like CryEngine Sponza)
        glm::vec3(1.0f, 0.95f, 0.9f),        // Color: warm sunlight
        2.5f,                                 // DiffuseIntensity: bright sunlight (increased for steeper angle)
        0.0f                                  // AmbientIntensity: no ambient (pure directional)
    ));

    // 3. CREATE SPOTLIGHT - Positioned above scene to cast shadows
    //m_SceneLights.push_back(CreateSpotLight(
    //    glm::vec3(0.0f, 6.0f, 0.0f),        // Position: centered above the grid
    //    glm::vec3(0.0f, -1.0f, 0.0f),        // Direction: pointing straight down
    //    glm::vec3(1.0f, 0.95f, 0.85f),       // Color: warm sunlight
    //    3.0f,                                 // DiffuseIntensity: bright spotlight
    //    0.0f,                                 // AmbientIntensity: no per-light ambient
    //    20.0f,                                // Range: covers entire scene
    //    25.0f,                                // InnerCone: 25 degrees
    //    35.0f                                 // OuterCone: 35 degrees
    //));

    // Set ambient light
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.01f));
    spdlog::info("Spotlight created at (0, 10, 0) pointing down with intensity 3.0");
    spdlog::info("Ambient light set to (0.03, 0.03, 0.03)");

    // 4. DISABLE SKYBOX - Editor doesn't use skybox
    m_SceneRenderer->EnableSkybox(false);
    //spdlog::info("Skybox disabled (editor has no skybox)");

    // 5. SETUP OUTLINE MODE
    m_SceneRenderer->ClearOutlinedObjects();

    spdlog::info("Editor demo setup complete: {} cubes, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: This demo matches the editor's default scene (CreateDemoScene)");
}

// ===== DEMO 4: TRANSPARENCY TEST - LIKE LEARNOPENGL =====
void GraphicsTestDriver::SetupTransparencyDemo()
{
    m_ActiveDemo = DemoType::Tinbox;  // Reuse Tinbox enum
    spdlog::info("=== SETTING UP TRANSPARENCY DEMO (Like LearnOpenGL) ===");

    // 1. CREATE CAMERA
    // Camera looks down -Z axis (into screen) in OpenGL right-handed coordinates
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 0.0f, 20.0f));  // Further back to see the 1000 window grid
    m_Camera->SetRotation(glm::vec3(0.0f, -90.0f, 0.0f));  // Yaw=-90° = looking down -Z axis
    spdlog::info("Camera at (0,0,20) looking at 1000-window grid");

    // 2. CREATE OPAQUE REFERENCE CUBES (for depth reference in the grid)
    auto cubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(2.0f));

    // Back cube - Red (far behind the grid)
    RenderableData backCube;
    backCube.mesh = cubeMesh;
    backCube.material = m_ResourceManager->GetMaterial("RedMaterial");
    backCube.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -30.0f));
    backCube.visible = true;
    backCube.objectID = 2;
    backCube.modelInstanceID = 2;
    m_SceneObjects.push_back(backCube);

    // Front cube - Blue (in front of the grid, closer to camera)
    RenderableData frontCube;
    frontCube.mesh = cubeMesh;
    frontCube.material = m_ResourceManager->GetMaterial("BlueMaterial");
    frontCube.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f));
    frontCube.visible = true;
    frontCube.objectID = 3;
    frontCube.modelInstanceID = 3;
    m_SceneObjects.push_back(frontCube);

    // 4. LOAD WINDOW TEXTURE
    unsigned int windowTexture = TextureLoader::TextureFromFile(
        "window.png",
        "assets/models/window",
        false
    );

    if (windowTexture == 0) {
        spdlog::error("Failed to load window.png texture!");
        return;
    }
    spdlog::info("Window texture loaded successfully (ID: {})", windowTexture);

    // 5. CREATE TRANSPARENT WINDOW MATERIAL
    auto shader = m_ResourceManager->GetShader("main_pbr");
    if (!shader) {
        spdlog::error("Failed to get main_pbr shader!");
        return;
    }

    auto transparentMaterial = std::make_shared<Material>(shader, "TransparentWindowMaterial");
    transparentMaterial->SetAlbedoColor(glm::vec3(1.0f, 1.0f, 1.0f));  // White (texture color)
    transparentMaterial->SetMetallicValue(0.0f);
    transparentMaterial->SetRoughnessValue(0.5f);
    transparentMaterial->SetBlendMode(BlendingMode::Transparent);
    m_ResourceManager->AddMaterial("TransparentWindowMaterial", transparentMaterial);
    spdlog::info("Transparent window material created");

    // 6. CREATE WINDOW QUAD MESHES WITH TEXTURE
    auto windowMesh = std::make_shared<Mesh>(
        PrimitiveGenerator::CreatePlane(1.0f, 1.0f, 1, 1)
    );

    // Add texture to mesh
    Texture windowTex;
    windowTex.id = windowTexture;
    windowTex.type = "texture_diffuse";
    windowTex.path = "assets/models/window/window.png";
    windowMesh->textures.push_back(windowTex);

    // 7. CREATE 1000 TRANSPARENT WINDOWS - Performance stress test
    // Planes are created in XZ plane (horizontal) with normal pointing +Y
    // We need to rotate them 90° around X axis to face the camera (-Z direction)
    // Windows distributed in a grid pattern across X, Y, and Z to test batch sorting
    std::vector<glm::vec3> windowPositions;
    windowPositions.reserve(1000);

    // Generate 1000 windows in a 10x10x10 grid
    const int gridSize = 10;
    const float spacing = 2.0f;  // 2 units between windows
    const float startOffset = -(gridSize - 1) * spacing * 0.5f;

    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                windowPositions.push_back(glm::vec3(
                    startOffset + x * spacing,
                    startOffset + y * spacing,
                    startOffset + z * spacing - 10.0f  // Shift back to be in front of camera
                ));
            }
        }
    }

    // Rotation to make plane face camera: 90° around X-axis
    // This rotates from XZ plane (horizontal) to XY plane (vertical, facing -Z)
    glm::mat4 faceCamera = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    uint32_t windowObjectID = 100;
    for (const auto& pos : windowPositions) {
        RenderableData window;
        window.mesh = windowMesh;
        window.material = transparentMaterial;

        // Transform = Translate * Rotate (rotate first, then translate)
        window.transform = glm::translate(glm::mat4(1.0f), pos) * faceCamera;

        window.visible = true;
        window.objectID = windowObjectID++;
        window.modelInstanceID = windowObjectID;  // Each window is its own instance
        m_SceneObjects.push_back(window);
    }
    spdlog::info("Created {} transparent windows in 10x10x10 grid (performance test)", windowPositions.size());

    // 8. CREATE LIGHTING
    m_SceneLights.push_back(CreateDirectionalLight(
        glm::vec3(-0.3f, -0.8f, -0.2f),
        glm::vec3(1.0f, 0.95f, 0.9f),
        2.5f,
        0.0f
    ));
    m_SceneRenderer->SetAmbientLight(glm::vec3(0.3f));  // Higher ambient for visibility
    spdlog::info("Directional light created");

    // 9. DISABLE SKYBOX
    m_SceneRenderer->EnableSkybox(false);

    spdlog::info("Transparency demo setup complete: {} objects, {} lights",
                 m_SceneObjects.size(), m_SceneLights.size());
    spdlog::info("NOTE: This demo tests batch-level sorting for transparent objects");
    spdlog::info("NOTE: Windows should blend correctly when viewed from different angles");
}

void GraphicsTestDriver::CreateModelInstance(const std::string& modelName, const std::string& materialName,
                                            const glm::vec3& position, const glm::vec3& scale,
                                            bool perNodeSelection)
{
    auto model = m_ResourceManager->GetModel(modelName);
    if (!model || model->meshes.empty()) {
        return;
    }

    // Static counter for unique modelInstanceID values
    static uint32_t nextModelInstanceID = 1000;

    // Build a map of unique node names to modelInstanceID for THIS model instance
    std::map<std::string, uint32_t> nodeNameToInstanceID;

    if (perNodeSelection) {
        // Per-node selection: Each unique node name gets its own modelInstanceID
        // (e.g., individual pillars/arches in Sponza)
        for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
            const std::string& nodeName = model->meshNodeNames[meshIndex];

            if (nodeNameToInstanceID.find(nodeName) == nodeNameToInstanceID.end()) {
                // New node name, assign new modelInstanceID
                nodeNameToInstanceID[nodeName] = nextModelInstanceID++;
            }
        }
    } else {
        // Whole-model selection: All meshes share one modelInstanceID
        // (e.g., tinbox top and bottom should be outlined together)
        uint32_t sharedModelInstanceID = nextModelInstanceID++;
        for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
            const std::string& nodeName = model->meshNodeNames[meshIndex];
            nodeNameToInstanceID[nodeName] = sharedModelInstanceID;
        }
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

                // Detect glass materials by checking node name or texture paths
                const std::string& nodeName = model->meshNodeNames[meshIndex];
                bool isGlass = (nodeName.find("glass") != std::string::npos ||
                                nodeName.find("Glass") != std::string::npos);

                // Also check texture names for "glass" keyword
                if (!isGlass) {
                    for (const auto& texture : mesh->textures) {
                        if (texture.path.find("glass") != std::string::npos ||
                            texture.path.find("Glass") != std::string::npos) {
                            isGlass = true;
                            break;
                        }
                    }
                }

                if (isGlass) {
                    // Glass material - transparent with slight blue tint
                    renderable.material->SetAlbedoColor(glm::vec3(0.9f, 0.95f, 1.0f));
                    renderable.material->SetMetallicValue(0.0f);
                    renderable.material->SetRoughnessValue(0.05f);  // Very smooth
                    renderable.material->SetBlendMode(BlendingMode::Transparent);
                    spdlog::info("Detected glass material for mesh '{}' - enabling transparency", nodeName);
                } else {
                    // Regular textured material
                    renderable.material->SetAlbedoColor(glm::vec3(1.0f, 1.0f, 1.0f));
                    renderable.material->SetMetallicValue(0.0f);
                    renderable.material->SetRoughnessValue(0.5f);
                }
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

        // Assign the model instance ID based on node name (per-node selection)
        const std::string& nodeName = model->meshNodeNames[meshIndex];
        renderable.modelInstanceID = nodeNameToInstanceID[nodeName];

        //spdlog::info("Created object with ID: {}, ModelInstanceID: {}, NodeName: {}",
        //             renderable.objectID, renderable.modelInstanceID, nodeName);

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
    // Cache AABBs on first frame (avoids iterating through 2.6M vertices every frame)
    if (!m_AABBsCached) {
        m_CachedAABBs.clear();

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

        // Calculate AABB per model instance (EXPENSIVE - only done once!)
        for (auto it = modelInstances.begin(); it != modelInstances.end(); ++it) {
            const glm::vec3& instancePosition = it->first;
            const std::vector<const RenderableData*>& meshes = it->second;

            AABB modelAABB; // Start with invalid AABB
            glm::mat4 instanceTransform = meshes[0]->transform;

            // Combine all mesh AABBs to create model AABB
            for (const auto* renderable : meshes) {
                AABB meshAABB = AABB::CreateFromMesh(renderable->mesh);
                modelAABB.ExpandToInclude(meshAABB);
            }

            // Create debug color
            glm::vec3 debugColor = glm::vec3(1.0f, 0.0f, 0.0f);

            // Cache the AABB with local space bounds
            m_CachedAABBs.emplace_back(modelAABB, instanceTransform, debugColor);
        }

        m_AABBsCached = true;
    }

    // Submit cached AABBs to scene renderer (fast - just copies data)
    m_SceneRenderer->SetDebugAABBs(m_CachedAABBs);
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
                s_Instance->m_SceneRenderer->ToggleRenderPass("DirectionalShadowPass");
                break;

            case GLFW_KEY_2:
                s_Instance->m_SceneRenderer->ToggleRenderPass("OutlinePass");
                break;

            case GLFW_KEY_3:
                s_Instance->m_SceneRenderer->ToggleRenderPass("DebugPass");
                break;

            case GLFW_KEY_4:
                s_Instance->m_SceneRenderer->ToggleAABBVisualization();
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
                    bool isEnabled = s_Instance->m_SceneRenderer->IsPassEnabled("PickingPass");
                    spdlog::info("Picking mode {} (Left-click objects to test)",
                               isEnabled ? "already ENABLED" : "ENABLED");
                    if (!isEnabled) {
                        s_Instance->m_SceneRenderer->EnablePicking(true);
                    }
                }
                break;

            case GLFW_KEY_7:
                s_Instance->m_SceneRenderer->ToggleRenderPass("PointShadowPass");
                break;

            case GLFW_KEY_8:
                s_Instance->PrintPointShadowInfo();
                break;

            case GLFW_KEY_9:
                s_Instance->PrintHDRInfo();
                break;

            case GLFW_KEY_O:
                s_Instance->m_HDREnabled = !s_Instance->m_HDREnabled;
                s_Instance->m_SceneRenderer->ToggleHDRPipeline(s_Instance->m_HDREnabled);
                break;

            case GLFW_KEY_H:
                s_Instance->m_SceneRenderer->ToggleRenderPass("HDRLuminancePass");
                break;

            case GLFW_KEY_T:
                s_Instance->m_SceneRenderer->ToggleRenderPass("ToneMapPass");
                break;

            case GLFW_KEY_R:
                s_Instance->m_SceneRenderer->ToggleRenderPass("HDRResolvePass");
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
                s_Instance->m_SceneRenderer->ToggleRenderPass("SpotShadowPass");
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
        // Check status of all render passes
        std::vector<std::string> passes = {
            "DirectionalShadowPass",
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
            bool enabled = m_SceneRenderer->IsPassEnabled(passName);
            spdlog::info("  {}: {}", passName, enabled ? "ENABLED" : "DISABLED");
        }
    } else {
        spdlog::warn("Scene renderer not available!");
    }

    spdlog::info("==========================");
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
        bool passEnabled = m_SceneRenderer->IsPassEnabled("PointShadowPass");
        spdlog::info("  Point Shadow Pass: {}", passEnabled ? "ENABLED" : "DISABLED");

            // Count point lights
            int pointLightCount = 0;
            for (const auto& light : m_SceneLights) {
                if (light.enabled && light.type == Light::Type::Point) {
                    pointLightCount++;
                    spdlog::info("  Point Light #{}: Position({:.1f}, {:.1f}, {:.1f}), Intensity={:.1f}, Range={:.1f}",
                                pointLightCount,
                                light.position.x, light.position.y, light.position.z,
                                light.diffuseIntensity, light.range);
                }
            }

            spdlog::info("  Total Point Lights: {}", pointLightCount);

            const auto& frameData = m_SceneRenderer->GetFrameDataReadOnly();

            // Count point shadows in unified shadow array
            int pointShadowCount = 0;
            for (const auto& shadow : frameData.shadowDataArray) {
                if (shadow.shadowType == ShadowData::Point) {
                    pointShadowCount++;
                }
            }
        spdlog::info("  Point Shadows in SSBO: {}", pointShadowCount);
        spdlog::info("  Total Shadows in SSBO: {}", frameData.shadowDataArray.size());
    } else {
        spdlog::warn("Scene renderer not available!");
    }

    spdlog::info("================================");
}

void GraphicsTestDriver::PrintHDRInfo() const
{
    spdlog::info("=== HDR Information ===");

    if (m_SceneRenderer) {
        // Check HDR pass status
        bool hdrLumEnabled = m_SceneRenderer->IsPassEnabled("HDRLuminancePass");
        bool toneMapEnabled = m_SceneRenderer->IsPassEnabled("ToneMapPass");
        bool hdrResolveEnabled = m_SceneRenderer->IsPassEnabled("HDRResolvePass");

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
        /*spdlog::info("PICKED OBJECT: ID = {}, World Position = ({:.2f}, {:.2f}, {:.2f}), Depth = {:.3f}",
                    result.objectID,
                    result.worldPosition.x, result.worldPosition.y, result.worldPosition.z,
                    result.depth);*/

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
                    /*spdlog::info("  -> Outlining mesh with objectID: {} (modelInstanceID: {})",
                               renderable.objectID, renderable.modelInstanceID);*/
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