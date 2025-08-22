#include <iostream>
#include <memory>

// Graphics library includes - these should handle all the dependencies internally
#include <Core/Window.h>
#include <Core/GraphicsContext.h>
#include <Core/Renderer.h>
#include <Resources/ResourceManager.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Utility/Camera.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightComponent.h>
#include <ECS/Systems/RenderSystem.h>

int main()
{
    try 
    {
        std::cout << "Initializing Graphics Engine..." << std::endl;

        // Create window and graphics context
        Window window("Graphics Engine Demo - Tinbox Model", 800, 600);
        auto nativeWindow = window.GetNativeWindow();
        if (!nativeWindow)
        {
            std::cerr << "Failed to create window!" << std::endl;
            return -1;
        }

        GraphicsContext context(nativeWindow);
        context.Initialize();
        if (!context.IsInitialized())
        {
            std::cerr << "Failed to initialize graphics context!" << std::endl;
            return -1;
        }

        std::cout << "Window and context initialized successfully!" << std::endl;

        // Initialize renderer and resource manager
        Renderer::Get().Initialize(nativeWindow);
        ResourceManager::Get().Initialize();

        std::cout << "Renderer and ResourceManager initialized!" << std::endl;

        // Load shaders from the assets directory
        auto basicShader = ResourceManager::Get().LoadShader("basic", "assets/shaders/basic");
        if (!basicShader) 
        {
            std::cerr << "Failed to load basic shader!" << std::endl;
            return -1;
        }

        std::cout << "Shaders loaded successfully!" << std::endl;

        // Load the tinbox model using ResourceManager
        ModelData tinboxModel = ResourceManager::Get().LoadModel("tinbox", "assets/models/tinbox/tin_box.obj", "basic");
        
        if (tinboxModel.meshes.empty()) 
        {
            std::cerr << "Failed to load tinbox model or model has no meshes!" << std::endl;
            return -1;
        }

        std::cout << "Tinbox model loaded successfully!" << std::endl;
        std::cout << "Model contains " << tinboxModel.meshes.size() << " meshes" << std::endl;
        std::cout << "Model contains " << tinboxModel.materials.size() << " materials" << std::endl;

        // Create scene and entities
        Scene scene("MainScene");

        // Create camera entity
        Entity cameraEntity = scene.CreateEntity("MainCamera");
        auto camera = std::make_shared<Camera>(CameraType::Perspective);
        camera->SetPerspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);
        auto& camComp = cameraEntity.AddComponent<CameraComponent>();
        camComp.camera = camera;
        camComp.Primary = true;

        // Create entities for each mesh in the model
        std::vector<Entity> modelEntities;
        for (size_t i = 0; i < tinboxModel.meshes.size(); ++i) 
        {
            Entity meshEntity = scene.CreateEntity("TinboxMesh_" + std::to_string(i));
            meshEntity.AddComponent<TransformComponent>();
            meshEntity.AddComponent<MeshComponent>(tinboxModel.meshes[i]);
            
            // Set up materials
            MaterialComponent matComp;
            if (i < tinboxModel.materials.size() && tinboxModel.materials[i]) 
            {
                matComp.Materials.push_back(tinboxModel.materials[i]);
            }
            else 
            {
                // Create a default material if none exists
                auto defaultMaterial = ResourceManager::Get().CreateMaterial("default_" + std::to_string(i), "basic");
                if (defaultMaterial) 
                {
                    // Set default material properties using available API
                    defaultMaterial->SetAlbedo({0.8f, 0.8f, 0.8f});
                    defaultMaterial->SetMetallic(0.0f);
                    defaultMaterial->SetRoughness(0.5f);
                    matComp.Materials.push_back(defaultMaterial);
                }
            }
            meshEntity.AddComponent<MaterialComponent>(matComp);
            
            modelEntities.push_back(meshEntity);
        }

        // Create light entity
        Entity lightEntity = scene.CreateEntity("DirectionalLight");
        auto& lightComp = lightEntity.AddComponent<LightComponent>();
        lightComp.Type = LightType::Directional;
        lightComp.Color = {1.0f, 1.0f, 1.0f};
        lightComp.Intensity = 1.0f;

        std::cout << "Scene set up successfully!" << std::endl;

        // Create render system
        RenderSystem renderSystem;
        
        // Set up renderer
        Renderer& renderer = Renderer::Get();
        renderer.GetContext()->SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        std::cout << "Starting render loop..." << std::endl;

        // Simple render loop - no input handling for now to avoid GLFW dependencies
        float rotationAngle = 0.0f;
        while (!window.ShouldClose())
        {
            // Simple time-based rotation
            rotationAngle += 0.01f; // Rotate slowly

            // Update transforms (rotate model)
            for (auto& entity : modelEntities) 
            {
                auto& transform = entity.GetComponent<TransformComponent>();
                transform.Rotation.y = rotationAngle;
            }

            // Begin frame
            renderer.BeginFrame();

            // Update and render scene
            scene.OnUpdate(0.016f); // Assume ~60fps
            renderSystem.OnUpdate(&scene);

            // End frame
            renderer.EndFrame();
            context.SwapBuffers();
            window.PollEvents();
        }

        std::cout << "Shutting down..." << std::endl;

        // Cleanup
        ResourceManager::Get().Shutdown();
        Renderer::Get().Shutdown();
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}