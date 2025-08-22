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

protected:
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

        // Load tin box textures
        std::cout << "Loading textures..." << std::endl;
        m_BaseColorTexture = rm.LoadTexture("tinbox_basecolor", "assets/models/tinbox/tin can_lambert1_BaseColor.png");
        m_NormalTexture = rm.LoadTexture("tinbox_normal", "assets/models/tinbox/tin can_lambert1_Normal.png");
        m_MetallicTexture = rm.LoadTexture("tinbox_metallic", "assets/models/tinbox/tin can_lambert1_Metallic.png");
        m_RoughnessTexture = rm.LoadTexture("tinbox_roughness", "assets/models/tinbox/tin can_lambert1_Roughness.png");
        m_HeightTexture = rm.LoadTexture("tinbox_height", "assets/models/tinbox/tin can_lambert1_Height.png");

        // Verify texture loading
        int textureCount = 0;
        if (m_BaseColorTexture) { std::cout << "✓ Base Color texture loaded" << std::endl; textureCount++; }
        if (m_NormalTexture) { std::cout << "✓ Normal texture loaded" << std::endl; textureCount++; }
        if (m_MetallicTexture) { std::cout << "✓ Metallic texture loaded" << std::endl; textureCount++; }
        if (m_RoughnessTexture) { std::cout << "✓ Roughness texture loaded" << std::endl; textureCount++; }
        if (m_HeightTexture) { std::cout << "✓ Height texture loaded" << std::endl; textureCount++; }
        std::cout << "Total textures loaded: " << textureCount << "/5" << std::endl;

        // Setup scene
        SetupScene();
    }

    void SetupScene()
    {
        auto &scene = GetScene();

        // Create camera entity
        Entity cameraEntity = scene.CreateEntity("MainCamera");
        auto camera = std::make_shared<Camera>(CameraType::Perspective);
        camera->SetPerspective(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        camera->SetPosition({ 0.0f, 2.0f, 5.0f });
        camera->SetRotation({ -15.0f, 0.0f, 0.0f });

        cameraEntity.AddComponent<CameraComponent>(camera);
        cameraEntity.AddComponent<TransformComponent>(glm::vec3(0.0f, 2.0f, 5.0f));

        SetActiveCamera(camera);

        // Create light entity
        Entity lightEntity = scene.CreateEntity("MainLight");
        lightEntity.AddComponent<TransformComponent>(glm::vec3(5.0f, 5.0f, 5.0f));
        lightEntity.AddComponent<LightComponent>(LightType::Directional,
            glm::vec3(1.0f, 0.95f, 0.8f), 1.2f);

        // Create tin box model entity
        std::cout << "Setting up scene entities..." << std::endl;
        auto modelData = GetResourceManager().GetModel("tinbox");
        if (modelData.meshes.size() > 0)
        {
            std::cout << "Creating " << modelData.meshes.size() << " mesh entities..." << std::endl;
            for (size_t i = 0; i < modelData.meshes.size(); ++i)
            {
                Entity modelEntity = scene.CreateEntity("TinBox_" + std::to_string(i));
                std::cout << "  Entity " << i << " created" << std::endl;
                
                // Position the model at origin
                auto& transform = modelEntity.AddComponent<TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));
                transform.Scale = glm::vec3(1.0f); // Adjust scale if needed
                
                // Add mesh component
                modelEntity.AddComponent<MeshComponent>(modelData.meshes[i]);
                std::cout << "    Mesh component added (vertices: " << modelData.meshes[i]->GetVertexCount() << ")" << std::endl;

                // Create material with loaded textures
                if (i < modelData.materials.size())
                {
                    auto& material = modelEntity.AddComponent<MaterialComponent>(modelData.materials[i]);
                    std::cout << "    Material component added" << std::endl;
                    
                    // Assign PBR textures to material with correct uniform names
                    if (!material.Materials.empty() && material.Materials[0])
                    {
                        int texturesApplied = 0;
                        if (m_BaseColorTexture) {
                            material.Materials[0]->SetTexture("texture_diffuse1", m_BaseColorTexture);
                            texturesApplied++;
                        }
                        if (m_NormalTexture) {
                            material.Materials[0]->SetTexture("texture_normal1", m_NormalTexture);
                            texturesApplied++;
                        }
                        if (m_MetallicTexture) {
                            material.Materials[0]->SetTexture("texture_metallic1", m_MetallicTexture);
                            texturesApplied++;
                        }
                        if (m_RoughnessTexture) {
                            material.Materials[0]->SetTexture("texture_roughness1", m_RoughnessTexture);
                            texturesApplied++;
                        }
                        if (m_HeightTexture) {
                            material.Materials[0]->SetTexture("texture_height1", m_HeightTexture);
                            texturesApplied++;
                        }
                        std::cout << "    " << texturesApplied << " textures applied to material" << std::endl;
                    }
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
        app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}