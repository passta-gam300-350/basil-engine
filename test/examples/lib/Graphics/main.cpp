// test/examples/lib/graphics/main.cpp
#include "Application.h"
#include <Scene/Entity.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightComponent.h>

class GraphicsTestApp : public Application
{
public:
    GraphicsTestApp() : Application("Graphics Engine Test", 1280, 720)
    {
    }

    void LoadResources() override
    {
        auto &rm = GetResourceManager();

        // Load shaders from assets folder
        std::cout << "Loading shaders..." << std::endl;
        // Load separate vertex and fragment shaders
        m_BasicShader = rm.LoadShader("basic", 
            "assets/shaders/basic.vert",  // vertex shader
            "assets/shaders/basic.frag"   // fragment shader
        );
        if (m_BasicShader) {
            std::cout << "✓ Shader 'basic' loaded successfully" << std::endl;
        } else {
            std::cerr << "✗ Failed to load shader 'basic'" << std::endl;
        }

        // Load tin box model from assets
        std::cout << "Loading models..." << std::endl;
        m_TinBoxModel = rm.LoadModel("tinbox", "assets/models/tinbox/tin_box.obj", "basic");
        std::cout << "✓ Model loaded: " << m_TinBoxModel.meshes.size() << " meshes, " 
                  << m_TinBoxModel.materials.size() << " materials" << std::endl;

        // Temporarily disable texture loading to test basic rendering
        std::cout << "Skipping texture loading (testing basic rendering without textures)" << std::endl;
        
        // TODO: Fix Texture constructor and re-enable
        /*
        // Load tin box textures with error handling
        std::cout << "Loading textures..." << std::endl;
        try {
            m_BaseColorTexture = rm.LoadTexture("tinbox_basecolor", "assets/models/tinbox/tin_can_lambert1_BaseColor.png");
            m_NormalTexture = rm.LoadTexture("tinbox_normal", "assets/models/tinbox/tin_can_lambert1_Normal.png");
            m_MetallicTexture = rm.LoadTexture("tinbox_metallic", "assets/models/tinbox/tin_can_lambert1_Metallic.png");
            m_RoughnessTexture = rm.LoadTexture("tinbox_roughness", "assets/models/tinbox/tin_can_lambert1_Roughness.png");
            m_HeightTexture = rm.LoadTexture("tinbox_height", "assets/models/tinbox/tin_can_lambert1_Height.png");
        } catch (const std::exception& e) {
            std::cerr << "Exception during texture loading: " << e.what() << std::endl;
        }
        */

        // Texture verification (currently disabled)
        std::cout << "Total textures loaded: 0/5 (disabled for testing)" << std::endl;

        // Setup scene
        SetupScene();
    }

    void SetupScene()
    {
        auto &scene = GetScene();

        // Test basic ECS functionality first
        std::cout << "Testing basic ECS functionality..." << std::endl;
        try {
            Entity testEntity = scene.CreateEntity("TestEntity");
            std::cout << "Test entity created successfully" << std::endl;
            
            // First try a different component type to isolate the issue
            std::cout << "Trying to add CameraComponent..." << std::endl;
            auto testCamera = std::make_shared<Camera>(CameraType::Perspective);
            testEntity.AddComponent<CameraComponent>(testCamera);
            std::cout << "CameraComponent added successfully" << std::endl;
            
            // Skip TransformComponent for now due to EnTT compatibility issues
            std::cout << "Skipping TransformComponent (EnTT compatibility issue)" << std::endl;
            // testEntity.AddComponent<TransformComponent>();
            std::cout << "ECS test completed successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "ECS test failed: " << e.what() << std::endl;
            return;
        }

        // Create camera entity with debug output
        std::cout << "Creating camera entity..." << std::endl;
        Entity cameraEntity = scene.CreateEntity("MainCamera");
        std::cout << "Camera entity created" << std::endl;
        
        auto camera = std::make_shared<Camera>(CameraType::Perspective);
        camera->SetPerspective(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        camera->SetPosition({ 0.0f, 2.0f, 5.0f });
        camera->SetRotation({ -15.0f, 0.0f, 0.0f });
        std::cout << "Camera object configured" << std::endl;

        std::cout << "Adding CameraComponent..." << std::endl;
        cameraEntity.AddComponent<CameraComponent>(camera);
        std::cout << "Skipping TransformComponent for camera (EnTT issue)" << std::endl;
        // TODO: Fix TransformComponent EnTT compatibility
        // TransformComponent cameraTransform;
        // cameraTransform.Translation = glm::vec3(0.0f, 2.0f, 5.0f);
        // cameraEntity.AddComponent<TransformComponent>(cameraTransform);
        std::cout << "Camera components added successfully" << std::endl;

        SetActiveCamera(camera);
        std::cout << "Active camera set" << std::endl;

        // Create light entity
        std::cout << "Creating light entity..." << std::endl;
        Entity lightEntity = scene.CreateEntity("MainLight");
        std::cout << "Adding light components..." << std::endl;
        // Skip TransformComponent for light as well
        // lightEntity.AddComponent<TransformComponent>(glm::vec3(5.0f, 5.0f, 5.0f));
        lightEntity.AddComponent<LightComponent>(LightType::Directional,
            glm::vec3(1.0f, 0.95f, 0.8f), 1.2f);
        std::cout << "Light entity created successfully" << std::endl;

        // Create tin box model entity
        std::cout << "Setting up scene entities..." << std::endl;
        auto modelData = GetResourceManager().GetModel("tinbox");
        if (modelData.meshes.size() > 0)
        {
            std::cout << "Creating " << modelData.meshes.size() << " mesh entities..." << std::endl;
            for (size_t i = 0; i < modelData.meshes.size(); ++i)
            {
                std::cout << "  Creating model entity " << i << "..." << std::endl;
                Entity modelEntity = scene.CreateEntity("TinBox_" + std::to_string(i));
                std::cout << "  Entity " << i << " created" << std::endl;
                
                std::cout << "  Skipping TransformComponent for model..." << std::endl;
                // Skip TransformComponent due to EnTT compatibility issue
                // auto& transform = modelEntity.AddComponent<TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));
                // transform.Scale = glm::vec3(1.0f);
                std::cout << "  TransformComponent skipped" << std::endl;
                
                std::cout << "  Adding MeshComponent..." << std::endl;
                // Add mesh component
                modelEntity.AddComponent<MeshComponent>(modelData.meshes[i]);
                std::cout << "    Mesh component added (vertices: " << modelData.meshes[i]->GetVertexCount() << ")" << std::endl;

                // Create material with loaded textures
                if (i < modelData.materials.size())
                {
                    auto& material = modelEntity.AddComponent<MaterialComponent>(modelData.materials[i]);
                    std::cout << "    Material component added" << std::endl;
                    
                    // Texture assignment disabled for testing
                    std::cout << "    Material created without textures (testing basic rendering)" << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "✗ Warning: No meshes found in tin box model" << std::endl;
        }
        
        std::cout << "Scene setup complete!" << std::endl;
    }

    void Update(float deltaTime) override
    {
        // Add any custom update logic here
        // Rotate model, move camera, etc.
    }

    void Render() override
    {
        // Any custom rendering outside the pipeline
        // Usually empty if using the scene renderer
    }

private:
    // Shader resources
    std::shared_ptr<Shader> m_BasicShader;
    
    // Model resources
    ModelData m_TinBoxModel;
    
    // Texture resources
    std::shared_ptr<Texture> m_BaseColorTexture;
    std::shared_ptr<Texture> m_NormalTexture;
    std::shared_ptr<Texture> m_MetallicTexture;
    std::shared_ptr<Texture> m_RoughnessTexture;
    std::shared_ptr<Texture> m_HeightTexture;
};

int main()
{
    try
    {
        GraphicsTestApp app;
		app.Initialize();
		app.LoadResources();
        app.Run();
        app.Shutdown();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}