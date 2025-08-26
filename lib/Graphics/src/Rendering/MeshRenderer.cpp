#include "Rendering/MeshRenderer.h"
#include "../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/ComponentInterfaces.h"
#include "Core/RenderCommandBuffer.h"
#include "Core/Renderer.h"
#include <iostream>
#include <entt/entt.hpp>

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
    auto* componentAccessor = ComponentUtils::GetComponentAccessor();
    
    if (!componentAccessor) {
        std::cout << "Warning: No component accessor set for MeshRenderer" << std::endl;
        return;
    }
    
    // Get view and projection matrices
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix();
    
    // Query for renderable entities
    auto view = registry.view<MeshComponent, MaterialComponent>();
    
    view.each([&](auto entity, auto& mesh, auto& material) {
        // Check if entity has required components using interface
        if (!componentAccessor->HasTransform(registry, entity) || 
            !componentAccessor->HasVisibility(registry, entity)) {
            return;
        }
        
        // Get components via interface
        auto* visibility = componentAccessor->GetVisibility(registry, entity);
        if (!visibility || !visibility->IsVisible()) {
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
    // Get graphics components
    auto& mesh = registry.get<MeshComponent>(entity);
    auto& material = registry.get<MaterialComponent>(entity);
    
    // Get engine components via interface
    auto* componentAccessor = ComponentUtils::GetComponentAccessor();
    if (!componentAccessor) return;
    
    auto* transform = componentAccessor->GetTransform(registry, entity);
    if (!transform) return;

    // Get the first material and its shader
    auto& mat = material.Materials[0];
    auto shader = mat->GetShader();
    
    if (!shader) {
        return;
    }

    // Create value-based commands for efficient sorting - this is graphics work
    auto& meshData = *mesh.mesh;
    glm::vec3 cameraPosition = camera.GetPosition();
    
    // Generate sort key for efficient batching
    RenderCommands::CommandSortKey sortKey;
    sortKey.pass = 0;  // Main opaque pass
    sortKey.material = reinterpret_cast<uintptr_t>(mat.get()) & 0xFFFFFF; // Material ID
    sortKey.mesh = reinterpret_cast<uintptr_t>(mesh.mesh.get()) & 0xFFFF;  // Mesh ID
    sortKey.instance = static_cast<uint32_t>(entity) & 0xFFFF;  // Instance ID
    
    // 1. Bind shader (can be sorted and deduplicated)
    RenderCommands::BindShaderData bindShaderCmd{shader};
    Renderer::Get().Submit(bindShaderCmd, sortKey);
    
    // 2. Set uniforms (per-object, cannot be deduplicated)
    RenderCommands::SetUniformsData uniformsCmd{
        shader,
        transform->GetTransform(), // Model matrix via interface
        viewMatrix,                // View matrix  
        projectionMatrix,          // Projection matrix
        cameraPosition             // Camera position
    };
    Renderer::Get().Submit(uniformsCmd, sortKey);
    
    // 3. Bind textures (can be sorted and deduplicated by material)
    RenderCommands::BindTexturesData texturesCmd{meshData.textures, shader};
    Renderer::Get().Submit(texturesCmd, sortKey);
    
    // 4. Draw geometry (pure drawing, no state setup)
    RenderCommands::DrawElementsData drawCmd{
        mesh.mesh->GetVertexArray()->GetVAOHandle(), 
        mesh.mesh->GetIndexCount()
    };
    Renderer::Get().Submit(drawCmd, sortKey);
}