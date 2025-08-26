#include "Core/RenderCommandBuffer.h"
#include "Resources/TextureBindingSystem.h"
#include <glad/gl.h>
#include <algorithm>
#include <iostream>

RenderCommandBuffer::RenderCommandBuffer() {
    // Default to traditional texture binding
    m_TextureBindingSystem = TextureBindingFactory::Create(TextureBindingFactory::BindingType::Traditional);
}

void RenderCommandBuffer::Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey)
{
    m_Commands.emplace_back(SortableCommand{command, sortKey});
}

void RenderCommandBuffer::SetTextureBindingSystem(std::unique_ptr<ITextureBindingSystem> bindingSystem)
{
    m_TextureBindingSystem = std::move(bindingSystem);
}

void RenderCommandBuffer::Clear()
{
    m_Commands.clear();
}

void RenderCommandBuffer::Sort()
{
    // Sort commands by sort key for optimal rendering performance
    std::sort(m_Commands.begin(), m_Commands.end());
}

void RenderCommandBuffer::Execute()
{
    // Begin batch operations for texture binding
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->BeginBatch();
    }
    
    // Execute commands in order (after sorting)
    for (const auto& sortableCmd : m_Commands) {
        // Use std::visit for efficient type-based dispatch
        std::visit([this](const auto& cmd) {
            this->ExecuteCommand(cmd);
        }, sortableCmd.command);
    }
    
    // End batch operations
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->EndBatch();
    }
}

size_t RenderCommandBuffer::GetMemoryUsage() const
{
    return m_Commands.size() * sizeof(SortableCommand);
}

// Command execution implementations
void RenderCommandBuffer::ExecuteCommand(const RenderCommands::ClearData& cmd)
{
    if (cmd.clearColor) {
        glClearColor(cmd.r, cmd.g, cmd.b, cmd.a);
    }
    
    GLbitfield mask = 0;
    if (cmd.clearColor) mask |= GL_COLOR_BUFFER_BIT;
    if (cmd.clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    
    if (mask != 0) {
        glClear(mask);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindShaderData& cmd)
{
    if (cmd.shader) {
        cmd.shader->use();
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformsData& cmd)
{
    if (!cmd.shader) return;
    
    // Set transform matrices
    cmd.shader->setMat4("u_Model", cmd.modelMatrix);
    cmd.shader->setMat4("u_View", cmd.viewMatrix);
    cmd.shader->setMat4("u_Projection", cmd.projectionMatrix);
    
    // Set camera position for lighting
    cmd.shader->setVec3("u_ViewPos", cmd.cameraPosition);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindTexturesData& cmd)
{
    if (!cmd.shader || !m_TextureBindingSystem) return;
    
    // Use abstracted texture binding system
    m_TextureBindingSystem->BindTextures(cmd.textures, cmd.shader);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawElementsData& cmd)
{
    // Pure drawing - assumes state is already set up
    glBindVertexArray(cmd.vao);
    glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    // Reset texture state using abstraction
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->UnbindAll();
    }
}