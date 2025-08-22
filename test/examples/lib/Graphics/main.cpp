// test/examples/lib/graphics/main.cpp
#include "Application.h"
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

        // Load your actual assets
        std::cout << "Loading shaders..." << std::endl;
        m_BasicShader = rm.LoadShader("basic", "assets/shaders/basic.glsl");

        std::cout << "Loading models..." << std::endl;
        auto modelData = rm.LoadModel("testModel", "assets/models/your_model.obj", "basic");

        std::cout << "Loading textures..." << std::endl;
        m_TestTexture = rm.LoadTexture("test", "assets/textures/your_texture.png");

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

        // Create model entity
        auto modelData = GetResourceManager().GetModel("testModel");
        for (size_t i = 0; i < modelData.meshes.size(); ++i)
        {
            Entity modelEntity = scene.CreateEntity("ModelPart_" + std::to_string(i));
            modelEntity.AddComponent<TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));
            modelEntity.AddComponent<MeshComponent>(modelData.meshes[i]);

            if (i < modelData.materials.size())
            {
                modelEntity.AddComponent<MaterialComponent>(modelData.materials[i]);
            }
        }
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
    std::shared_ptr<Shader> m_BasicShader;
    std::shared_ptr<Texture> m_TestTexture;
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