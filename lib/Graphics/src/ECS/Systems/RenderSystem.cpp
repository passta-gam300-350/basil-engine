#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include <glm/gtc/type_ptr.hpp>

void RenderSystem::OnUpdate(Scene* scene)
{
	if (!scene)
	{
		return;
	}

	// Get the main camera
	Camera* mainCamera = nullptr;
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);

	// Find the main camera
    auto cameraView = scene->GetRegistry().view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto [camera, transform] = cameraView.get<CameraComponent, TransformComponent>(entity);

        if (camera.Primary)
        {
            mainCamera = camera.camera.get();

            // Update camera view matrix based on transform
            glm::mat4 cameraTransform = transform.GetTransform();
            mainCamera->SetViewMatrix(glm::inverse(cameraTransform));

            viewMatrix = mainCamera->GetViewMatrix();
            projectionMatrix = mainCamera->GetProjectionMatrix();
            break;
        }
    }

    if (!mainCamera)
    {
        return; // No camera to render from
    }

    // Get all renderable entities
    auto renderableView = scene->GetRegistry().view<MeshComponent, MaterialComponent, TransformComponent>();

    // First pass: Depth prepass (optional)
    // ...

    // Main pass: Render geometry
    for (auto entity : renderableView)
    {
        DrawEntity(scene->GetRegistry(), entity);
    }

    // Additional passes (e.g., transparency, post-processing)
    // ...
}

void RenderSystem::DrawEntity(entt::registry& registry, entt::entity entity)
{
    // Get components
    auto& mesh = registry.get<MeshComponent>(entity);
    auto& material = registry.get<MaterialComponent>(entity);
    auto& transform = registry.get<TransformComponent>(entity);

    // Skip if no mesh or material
    if (!mesh.Mesh || material.Materials.empty())
    {
        return;
    }

    // Get the first material
    auto& mat = material.Materials[0];

    // Bind the shader
    mat->Bind();

    // Set transform matrix
    mat->Set("u_Model", transform.GetTransform());

    // The camera view/projection would have been set globally

    // Draw the mesh
    mesh.Mesh->Bind();

    // Create a draw command
    DrawCommand drawCmd(mesh.Mesh->GetVertexArray()->GetRendererID(), mesh.Mesh->GetIndexCount());
    Renderer::Get().Submit(drawCmd);
}