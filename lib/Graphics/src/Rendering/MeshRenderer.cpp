#include "Rendering/MeshRenderer.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include "Core/RenderCommandBuffer.h"
#include "Core/Renderer.h"
#include <iostream>

MeshRenderer::MeshRenderer(Renderer* renderer) : m_Renderer(renderer)
{
}

void MeshRenderer::Render(const std::vector<RenderableData>& renderables, const FrameData& frameData)
{
    if (renderables.empty())
        return;
    
    // Get view and projection matrices from frame data
    glm::mat4 viewMatrix = frameData.viewMatrix;
    glm::mat4 projectionMatrix = frameData.projectionMatrix;
    glm::vec3 cameraPosition = frameData.cameraPosition;
    
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
    m_Renderer->Submit(bindShaderCmd, sortKey);
    
    // 2. Set uniforms (per-object, cannot be deduplicated)
    RenderCommands::SetUniformsData uniformsCmd{
        shader,
        renderable.transform,      // Model matrix
        viewMatrix,                // View matrix  
        projectionMatrix,          // Projection matrix
        cameraPosition             // Camera position
    };
    m_Renderer->Submit(uniformsCmd, sortKey);
    
    // 3. Bind textures (can be sorted and deduplicated by material)
    RenderCommands::BindTexturesData texturesCmd{renderable.mesh->textures, shader};
    m_Renderer->Submit(texturesCmd, sortKey);
    
    // 4. Draw geometry (pure drawing, no state setup)
    RenderCommands::DrawElementsData drawCmd{
        renderable.mesh->GetVertexArray()->GetVAOHandle(), 
        renderable.mesh->GetIndexCount()
    };
    m_Renderer->Submit(drawCmd, sortKey);
}