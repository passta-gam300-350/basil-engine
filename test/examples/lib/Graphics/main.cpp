// test/examples/lib/graphics/main.cpp

// Force discrete GPU usage on systems with both integrated and discrete GPUs
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

#include "Application.h"
#include <Scene/SceneRenderer.h>
#include <Rendering/InstancedRenderer.h>
#include <Resources/Material.h>
#include <Resources/Mesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

class GraphicsTestApp : public Application
{
public:
    GraphicsTestApp() : Application("Graphics Engine Test", 1280, 720)
    {
    }

    void LoadResources() override
    {
        std::cout << "Loading resources..." << std::endl;

        // Load regular shader
        LoadShader("basic", 
            "assets/shaders/basic.vert", 
            "assets/shaders/basic.frag");

        // Load instanced shader
        LoadShader("instanced", 
            "assets/shaders/instanced.vert", 
            "assets/shaders/instanced.frag");

        // Load model using clean graphics lib interface  
        LoadModel("tinbox", "assets/models/tinbox/tin_box.obj");

        // Set up camera position for better view of instances
        SetCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));
        SetCameraRotation(glm::vec3(-15.0f, 0.0f, 0.0f));
        
        // Set up instanced rendering
        SetupInstancing();
        
        std::cout << "Resources loaded successfully!" << std::endl;
    }
    
private:
    void SetupInstancing()
    {
        std::cout << "Setting up instanced rendering..." << std::endl;
        
        // Get the instanced renderer
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        if (!instancedRenderer) {
            std::cerr << "Error: InstancedRenderer not available!" << std::endl;
            return;
        }
        
        // Set up mesh and material for instancing
        auto model = m_ResourceManager->GetModel("tinbox");
        auto shader = m_ResourceManager->GetShader("instanced");
        
        if (!model) {
            std::cerr << "Error: Model 'tinbox' not found!" << std::endl;
            return;
        }
        
        if (!shader) {
            std::cerr << "Error: Shader 'instanced' not found!" << std::endl;
            return;
        }
        
        if (model->meshes.empty()) {
            std::cerr << "Error: Model 'tinbox' has no meshes!" << std::endl;
            return;
        }
        
        std::cout << "✓ Model loaded with " << model->meshes.size() << " meshes" << std::endl;
        std::cout << "✓ Shader loaded successfully" << std::endl;
        
        // Create material for instanced rendering
        auto instancedMaterial = std::make_shared<Material>(shader, "InstancedMaterial");
        std::cout << "✓ Created instanced material" << std::endl;
        
        // Set up all meshes from the model for instanced rendering
        for (size_t i = 0; i < model->meshes.size(); ++i) {
            auto meshPtr = std::make_shared<Mesh>(model->meshes[i].vertices, 
                                                 model->meshes[i].indices, 
                                                 model->meshes[i].textures);
            std::cout << "✓ Created mesh " << i << " with " << meshPtr->GetVertexCount() << " vertices, " 
                      << meshPtr->GetIndexCount() << " indices" << std::endl;
            
            // Set mesh data for each part (body, lid, etc.)
            std::string meshId = "tinbox_instanced_" + std::to_string(i);
            instancedRenderer->SetMeshData(meshId, meshPtr, instancedMaterial);
            std::cout << "✓ Set mesh data for '" << meshId << "'" << std::endl;
        }
        
        // Generate instances in a grid pattern
        GenerateInstances();
        
        std::cout << "Instanced rendering setup complete!" << std::endl;
    }
    
    void GenerateInstances()
    {
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        if (!instancedRenderer) return;
        
        auto model = m_ResourceManager->GetModel("tinbox");
        if (!model) return;
        
        std::cout << "Generating " << (m_GridSize * m_GridSize) << " instances for " << model->meshes.size() << " meshes..." << std::endl;
        
        instancedRenderer->BeginInstanceBatch();
        
        float spacing = 2.5f;
        float offset = (m_GridSize - 1) * spacing * 0.5f;
        
        // Use consistent properties for all instances
        glm::vec4 uniformColor(0.8f, 0.6f, 0.4f, 1.0f);  // Nice golden color
        float uniformMetallic = 0.3f;
        float uniformRoughness = 0.4f;
        float uniformScale = 1.0f;
        
        for (int x = 0; x < m_GridSize; ++x) {
            for (int z = 0; z < m_GridSize; ++z) {
                // Only vary position
                glm::vec3 position(x * spacing - offset, 0.0f, z * spacing - offset);
                
                // Create transform matrix with uniform scale
                glm::mat4 transform = glm::mat4(1.0f);
                transform = glm::translate(transform, position);
                transform = glm::scale(transform, glm::vec3(uniformScale));
                
                // Create instance data with only position varying
                InstancedRenderer::InstanceData instance;
                instance.modelMatrix = transform;
                instance.color = uniformColor;
                instance.materialId = 0;
                instance.flags = 0;
                instance.metallic = uniformMetallic;
                instance.roughness = uniformRoughness;
                
                // Add instance for each mesh part (body, lid, etc.)
                for (size_t i = 0; i < model->meshes.size(); ++i) {
                    std::string meshId = "tinbox_instanced_" + std::to_string(i);
                    instancedRenderer->AddInstance(meshId, instance);
                }
            }
        }
        
        instancedRenderer->EndInstanceBatch();
        
        std::cout << "Generated " << (m_GridSize * m_GridSize) << " instances for " << model->meshes.size() << " mesh parts successfully!" << std::endl;
    }

    void Update(float deltaTime) override
    {
        // Update time for camera movement
        m_Time += deltaTime;
        
        // Optional: Move camera in a circle around the instances
        if (m_CircleCamera) {
            float radius = 20.0f;
            float x = sin(m_Time * m_CameraSpeed) * radius;
            float z = cos(m_Time * m_CameraSpeed) * radius;
            SetCameraPosition(glm::vec3(x, 5.0f, z));
        }
        
        // ANIMATION REMOVED: Previously caused system crashes
        // Animation functionality has been disabled for stability
    }
    
private:
    // REMOVED: RegenerateAnimatedInstances() function
    // This function was causing system crashes due to continuous SSBO updates
    // Animation functionality has been disabled for system stability

    void OnKeyboard(int key, int action, float deltaTime) override
    {
        // Handle additional key inputs
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_R:
                    // Reset camera position
                    SetCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));
                    SetCameraRotation(glm::vec3(-15.0f, 0.0f, 0.0f));
                    std::cout << "Camera reset" << std::endl;
                    break;
                case GLFW_KEY_C:
                    // Toggle camera circle mode
                    m_CircleCamera = !m_CircleCamera;
                    std::cout << "Circle camera: " << (m_CircleCamera ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_SPACE:
                    // Instance animation has been disabled for stability
                    std::cout << "Instance animation: DISABLED (was causing system crashes)" << std::endl;
                    break;
                case GLFW_KEY_I:
                    // Toggle between instanced and regular rendering
                    m_UseInstancing = !m_UseInstancing;
                    std::cout << "Instanced rendering: " << (m_UseInstancing ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_G:
                    // Change grid size
                    m_GridSize = (m_GridSize == 10) ? 5 : ((m_GridSize == 5) ? 20 : 10);
                    std::cout << "Grid size changed to: " << m_GridSize << "x" << m_GridSize << std::endl;
                    GenerateInstances();
                    break;
                case GLFW_KEY_H:
                    // Show help
                    std::cout << "\n=== Controls ===\n";
                    std::cout << "R: Reset camera\n";
                    std::cout << "C: Toggle camera circle mode\n";
                    std::cout << "SPACE: Instance animation (DISABLED - was causing crashes)\n";
                    std::cout << "I: Toggle instanced vs regular rendering\n";
                    std::cout << "G: Change grid size (5x5, 10x10, 20x20)\n";
                    std::cout << "H: Show this help\n\n";
                    break;
            }
        }
    }

    void OnMouseScroll(double yoffset) override
    {
        // Additional scroll handling if needed
        // std::cout << "Scroll: " << yoffset << std::endl;
    }

    void Render() override
    {
        if (m_UseInstancing) {
            // Instanced data already set up in LoadResources()
            // Pipeline will automatically render instances via InstancedRenderer
        } else {
            // Create/update ECS entity - pipeline will render via MeshRenderer
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f));
            RenderModel("tinbox", "basic", model);
        }
    }

private:
    // Instance rendering settings
    int m_GridSize = 10;
    bool m_UseInstancing = true;
    bool m_CircleCamera = false;
    float m_CameraSpeed = 0.5f;
    float m_Time = 0.0f;
    
    // REMOVED: m_AnimateInstances - was causing system crashes
};

int main()
{
    try
    {
        std::cout << "=== Starting Graphics Engine Test ===" << std::endl;
        
        // Create and run application - graphics lib handles everything internally
        GraphicsTestApp app;
        app.Run();
        
        std::cout << "=== Graphics Engine Test Complete ===" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}