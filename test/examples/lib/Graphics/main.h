#pragma once

#include <Core/Window.h>
#include <Scene/SceneRenderer.h>
#include <Resources/ResourceManager.h>
#include <Utility/Camera.h>
#include <Pipeline/ToneMapRenderPass.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

enum class DemoType
{
    Sponza,
    Tinbox
};

class GraphicsTestDriver
{
public:
    GraphicsTestDriver();
    ~GraphicsTestDriver();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    // Core systems
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    ResourceManager* m_ResourceManager;
    std::unique_ptr<Camera> m_Camera;

    // Advanced Graphics Demo: Instanced + Traditional Textures + PBR

    // Scene objects
    std::vector<RenderableData> m_SceneObjects;
    std::vector<SubmittedLightData> m_SceneLights;

    // Current demo type
    DemoType m_ActiveDemo;

    // Animation/Movement
    float m_Time;
    float m_DeltaTime;
    std::chrono::steady_clock::time_point m_LastFrameTime;

    // Camera controls
    bool m_CameraEnabled;
    bool m_FirstMouse;
    float m_LastX, m_LastY;

    // Animation controls
    bool m_RotationEnabled;

    // HDR state tracking
    bool m_HDREnabled;

    // Input handling
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    void ProcessInput();

    // Demo scene setups (each includes: objects, lights, camera, outline mode)
    void SetupSponzaDemo();      // Sponza cathedral - lighting test with HDR
    void SetupTinboxDemo();      // Tinbox grid - outline and PBR testing

    // Resource loading
    bool LoadTestResources();
    void CreateTestMaterials();
    
    // Utility functions
    void CreateModelInstance(const std::string& modelName, const std::string& materialName,
                           const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    SubmittedLightData CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                                              float diffuseIntensity = 1.0f, float ambientIntensity = 0.0f);
    SubmittedLightData CreatePointLight(const glm::vec3& position, const glm::vec3& color,
                                        float diffuseIntensity = 1.0f, float ambientIntensity = 0.0f, float range = 10.0f);
    SubmittedLightData CreateSpotLight(const glm::vec3& position, const glm::vec3& direction,
                                       const glm::vec3& color, float diffuseIntensity = 1.0f,
                                       float ambientIntensity = 0.0f, float range = 10.0f,
                                       float innerCone = 30.0f, float outerCone = 45.0f);

    // AABB calculation and debug rendering
    void CalculateAndSubmitAABBs();
    void UpdateInstanceTransforms();

    // Debug output
    void PrintSystemInfo() const;
    void PrintSceneInfo() const;
    void PrintRenderPassStatus() const;
    void PrintPointShadowInfo() const;
    void PrintHDRInfo() const;

    // Render pass controls
    void ToggleRenderPass(const std::string& passName);
    void ToggleHDRPipeline();
    void ToggleAABBVisualization();
    void ToggleSkybox();
    //void RenderUI(); // For ImGui if available

    // Object picking
    void HandleObjectPicking(double mouseX, double mouseY);

    // Outline modes
    void SetupStaticOutlines();           // Outline first 5 objects (static demo)
    void UpdateCameraBasedOutline();      // Outline object camera is looking at (dynamic)

    // Ray-AABB intersection helper
    bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                           const glm::vec3& aabbMin, const glm::vec3& aabbMax,
                           float& tMin, float& tMax) const;

    // Static pointer for callbacks
    static GraphicsTestDriver* s_Instance;
};