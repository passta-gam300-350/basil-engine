#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "Core/RenderCommand.h"
#include "Core/Renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <iostream>

void RenderSystem::OnUpdate(Scene* scene)
{
	if (!scene)
	{
		std::cerr << "RenderSystem: No scene provided" << std::endl;
		return;
	}
	
	std::cout << "RenderSystem: OnUpdate called (legacy ECS camera version)" << std::endl;

	// Get the main camera from ECS components
	Camera* mainCamera = nullptr;
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);

	// Find the main camera
    auto cameraView = scene->GetRegistry().view<CameraComponent, TransformComponent>();
    cameraView.each([&](auto entity, auto& camera, auto& transform) {
        if (camera.Primary)
        {
            mainCamera = camera.camera.get();

            // Update camera position and rotation based on transform
            mainCamera->SetPosition(transform.Translation);
            mainCamera->SetRotation(transform.Rotation);

            viewMatrix = mainCamera->GetViewMatrix();
            projectionMatrix = mainCamera->GetProjectionMatrix();
        }
    });

    if (!mainCamera)
    {
        std::cout << "RenderSystem: No main camera found, returning early" << std::endl;
        return; // No camera to render from
    }
    
    std::cout << "RenderSystem: Found main camera, proceeding with rendering" << std::endl;

    // Debug: Check what entities exist (only print first time)
    static bool firstTime = true;
    if (firstTime) {
        auto meshView = scene->GetRegistry().view<MeshComponent>();
        auto materialView = scene->GetRegistry().view<MaterialComponent>();
        auto transformView = scene->GetRegistry().view<TransformComponent>();
        
        std::cout << "RenderSystem: Entities with MeshComponent: " << std::distance(meshView.begin(), meshView.end()) << std::endl;
        std::cout << "RenderSystem: Entities with MaterialComponent: " << std::distance(materialView.begin(), materialView.end()) << std::endl;
        std::cout << "RenderSystem: Entities with TransformComponent: " << std::distance(transformView.begin(), transformView.end()) << std::endl;
        firstTime = false;
    }
    
    // Get all renderable entities
    auto renderableView = scene->GetRegistry().view<MeshComponent, MaterialComponent, TransformComponent>();
    auto entityCount = std::distance(renderableView.begin(), renderableView.end());
    std::cout << "RenderSystem: Found " << entityCount << " renderable entities with all three components" << std::endl;

    // First pass: Depth prepass (optional)
    // ...

    // Main pass: Render geometry
    renderableView.each([&](auto entity, auto& mesh, auto& material, auto& transform) {
        std::cout << "RenderSystem: Processing entity for rendering" << std::endl;
        DrawEntity(scene->GetRegistry(), entity, mainCamera, viewMatrix, projectionMatrix);
    });

    // Additional passes (e.g., transparency, post-processing)
    // ...
}

void RenderSystem::OnUpdate(Scene* scene, Camera& camera)
{
	if (!scene)
	{
		std::cerr << "RenderSystem: No scene provided" << std::endl;
		return;
	}
	
	std::cout << "RenderSystem: OnUpdate called with direct camera" << std::endl;

	// Use the provided camera directly
	Camera* mainCamera = &camera;
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	glm::mat4 projectionMatrix = camera.GetProjectionMatrix();
    
    std::cout << "RenderSystem: Using provided camera, proceeding with rendering" << std::endl;

    // Debug: Check what entities exist (only print first time)
    static bool firstTime = true;
    if (firstTime) {
        auto meshView = scene->GetRegistry().view<MeshComponent>();
        auto materialView = scene->GetRegistry().view<MaterialComponent>();
        auto transformView = scene->GetRegistry().view<TransformComponent>();
        
        std::cout << "RenderSystem: Entities with MeshComponent: " << std::distance(meshView.begin(), meshView.end()) << std::endl;
        std::cout << "RenderSystem: Entities with MaterialComponent: " << std::distance(materialView.begin(), materialView.end()) << std::endl;
        std::cout << "RenderSystem: Entities with TransformComponent: " << std::distance(transformView.begin(), transformView.end()) << std::endl;
        firstTime = false;
    }
    
    // Get all renderable entities
    auto renderableView = scene->GetRegistry().view<MeshComponent, MaterialComponent, TransformComponent>();
    auto entityCount = std::distance(renderableView.begin(), renderableView.end());
    std::cout << "RenderSystem: Found " << entityCount << " renderable entities with all three components" << std::endl;

    // First pass: Depth prepass (optional)
    // ...

    // Main pass: Render geometry
    renderableView.each([&](auto entity, auto& mesh, auto& material, auto& transform) {
        std::cout << "RenderSystem: Processing entity for rendering" << std::endl;
        DrawEntity(scene->GetRegistry(), entity, mainCamera, viewMatrix, projectionMatrix);
    });

    // Additional passes (e.g., transparency, post-processing)
    // ...
}

void RenderSystem::DrawEntity(entt::registry& registry, entt::entity entity, Camera* camera, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    // Get components
    auto& mesh = registry.get<MeshComponent>(entity);
    auto& material = registry.get<MaterialComponent>(entity);
    auto& transform = registry.get<TransformComponent>(entity);

    // Skip if no mesh or material
    if (!mesh.mesh || material.Materials.empty())
    {
        return;
    }

    // Get the first material and its shader
    auto& mat = material.Materials[0];
    auto shader = mat->GetShader();
    
    if (!shader)
    {
        return;
    }

    // Use the shader
    shader->use();

    // Set transform matrix
    shader->setMat4("u_Model", transform.GetTransform());
    
    // Set view and projection matrices
    shader->setMat4("u_View", viewMatrix);
    shader->setMat4("u_Projection", projectionMatrix);
    
    // Set camera position for lighting
    if (camera) {
        shader->setVec3("u_ViewPos", camera->GetPosition());
    }

    // Set texture availability flags for the shader
    auto& meshData = *mesh.mesh;
    bool hasDiffuse = false, hasNormal = false, hasMetallic = false, hasRoughness = false, hasAO = false, hasEmissive = false;
    
    // Check what texture types are available
    for (const auto& texture : meshData.textures) {
        if (texture.type == "texture_diffuse") hasDiffuse = true;
        else if (texture.type == "texture_normal") hasNormal = true;
        else if (texture.type == "texture_metallic") hasMetallic = true;
        else if (texture.type == "texture_roughness") hasRoughness = true;
        else if (texture.type == "texture_ao") hasAO = true;
        else if (texture.type == "texture_emissive") hasEmissive = true;
    }
    
    // Set texture availability flags in shader
    shader->setBool("u_HasDiffuseMap", hasDiffuse);
    shader->setBool("u_HasNormalMap", hasNormal);
    shader->setBool("u_HasMetallicMap", hasMetallic);
    shader->setBool("u_HasRoughnessMap", hasRoughness);
    shader->setBool("u_HasAOMap", hasAO);
    shader->setBool("u_HasEmissiveMap", hasEmissive);

    // Create enhanced draw command that handles textures within the pipeline
    std::cout << "RenderSystem: Submitting draw command with VAO " << mesh.mesh->GetVertexArray()->GetVAOHandle() 
              << ", " << mesh.mesh->GetIndexCount() << " indices, and " << meshData.textures.size() << " textures" << std::endl;
    
    DrawWithTexturesCommand drawCmd(mesh.mesh->GetVertexArray()->GetVAOHandle(), 
                                   mesh.mesh->GetIndexCount(), 
                                   meshData.textures, 
                                   shader);
    Renderer::Get().Submit(drawCmd);
}