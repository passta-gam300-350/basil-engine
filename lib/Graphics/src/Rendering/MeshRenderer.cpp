#include "Rendering/MeshRenderer.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "../../../test/examples/lib/Graphics/Engine/ECS/Components/TransformComponent.h"
#include "../../../test/examples/lib/Graphics/Engine/ECS/Components/VisibilityComponent.h"
#include "Core/RenderCommand.h"
#include "Core/Renderer.h"
#include <iostream>

void MeshRenderer::Render(Scene* scene, Camera& camera)
{
    if (!scene)
        return;

    std::cout << "MeshRenderer: Rendering visible meshes" << std::endl;
    
    // This is RENDERING COORDINATION - queries ECS and generates commands
    // No ECS system logic, just graphics work
    RenderVisibleMeshes(scene, camera);
}

void MeshRenderer::RenderVisibleMeshes(Scene* scene, Camera& camera)
{
    auto& registry = scene->GetRegistry();
    
    // Get view and projection matrices
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix();
    
    // Query for renderable entities that are visible
    auto view = registry.view<MeshComponent, MaterialComponent, TransformComponent, VisibilityComponent>();
    
    view.each([&](auto entity, auto& mesh, auto& material, auto& transform, auto& visibility) {
        // Skip if not visible (set by VisibilitySystem)
        if (!visibility.IsVisible) {
            return;
        }
        
        // Skip if no mesh or material data
        if (!mesh.mesh || material.Materials.empty()) {
            return;
        }
        
        // Generate render command - this is pure graphics work
        GenerateDrawCommand(registry, entity, viewMatrix, projectionMatrix, camera);
    });
}

void MeshRenderer::GenerateDrawCommand(entt::registry& registry, entt::entity entity, 
                                     const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                                     Camera& camera)
{
    // Get components
    auto& mesh = registry.get<MeshComponent>(entity);
    auto& material = registry.get<MaterialComponent>(entity);
    auto& transform = registry.get<TransformComponent>(entity);

    // Get the first material and its shader
    auto& mat = material.Materials[0];
    auto shader = mat->GetShader();
    
    if (!shader) {
        return;
    }

    // Create comprehensive draw command - this is graphics work
    auto& meshData = *mesh.mesh;
    glm::vec3 cameraPosition = camera.GetPosition();
    
    DrawMeshCommand drawCmd(mesh.mesh->GetVertexArray()->GetVAOHandle(), 
                           mesh.mesh->GetIndexCount(), 
                           meshData.textures, 
                           shader,
                           transform.GetTransform(),  // Model matrix
                           viewMatrix,                // View matrix  
                           projectionMatrix,          // Projection matrix
                           cameraPosition);           // Camera position
                           
    Renderer::Get().Submit(drawCmd);
}