#include "Rendering/MeshRenderer.h"
#include "Utility/RenderData.h"
#include "Core/RenderCommandBuffer.h"
#include "Core/Renderer.h"
#include <iostream>

void MeshRenderer::Render(const std::vector<RenderableData>& renderables, Camera& camera)
{
    if (renderables.empty())
        return;
    
    // Get view and projection matrices
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix();
    glm::vec3 cameraPosition = camera.GetPosition();
    
    // Generate render commands for each renderable
    for (const auto& renderable : renderables)
    {
        // Skip if not visible or no mesh/material
        if (!renderable.visible || !renderable.mesh || !renderable.material) {
            continue;
        }
        
        // Generate render command - this is pure graphics work
        GenerateDrawCommand(renderable, viewMatrix, projectionMatrix, cameraPosition);
    }
}

void MeshRenderer::GenerateDrawCommand(const RenderableData& renderable, 
                                     const glm::mat4& viewMatrix, 
                                     const glm::mat4& projectionMatrix,
                                     const glm::vec3& cameraPosition)
{
    auto shader = renderable.material->GetShader();
    
    if (!shader) {
        return;
    }

    // Generate sort key for efficient batching
    RenderCommands::CommandSortKey sortKey;
    sortKey.pass = renderable.renderLayer;  // Use render layer as pass
    sortKey.material = reinterpret_cast<uintptr_t>(renderable.material.get()) & 0xFFFFFF;
    sortKey.mesh = reinterpret_cast<uintptr_t>(renderable.mesh.get()) & 0xFFFF;
    sortKey.instance = 0;  // Could use entity ID if available
    
    // 1. Bind shader (can be sorted and deduplicated)
    RenderCommands::BindShaderData bindShaderCmd{shader};
    Renderer::Get().Submit(bindShaderCmd, sortKey);
    
    // 2. Set uniforms (per-object, cannot be deduplicated)
    RenderCommands::SetUniformsData uniformsCmd{
        shader,
        renderable.transform,      // Model matrix
        viewMatrix,                // View matrix  
        projectionMatrix,          // Projection matrix
        cameraPosition             // Camera position
    };
    Renderer::Get().Submit(uniformsCmd, sortKey);
    
    // 3. Bind textures (can be sorted and deduplicated by material)
    RenderCommands::BindTexturesData texturesCmd{renderable.mesh->textures, shader};
    Renderer::Get().Submit(texturesCmd, sortKey);
    
    // 4. Draw geometry (pure drawing, no state setup)
    RenderCommands::DrawElementsData drawCmd{
        renderable.mesh->GetVertexArray()->GetVAOHandle(), 
        renderable.mesh->GetIndexCount()
    };
    Renderer::Get().Submit(drawCmd, sortKey);
}