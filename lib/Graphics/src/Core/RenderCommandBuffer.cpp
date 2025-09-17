#include "Core/RenderCommandBuffer.h"
#include "Resources/TextureBindingSystem.h"
#include <glad/gl.h>
#include <algorithm>
#include <spdlog/spdlog.h>

RenderCommandBuffer::RenderCommandBuffer() {
    // Texture binding system will be initialized explicitly via Initialize()
    m_TextureBindingSystem = nullptr;
}

void RenderCommandBuffer::Submit(const VariantRenderCommand& command)
{
    m_Commands.emplace_back(command);
}

void RenderCommandBuffer::Initialize()
{
    if (!m_TextureBindingSystem) {
        spdlog::info("[RenderCommandBuffer] Initializing texture binding system...");
        m_TextureBindingSystem = TextureBindingFactory::Create(TextureBindingFactory::BindingType::Bindless);
        if (!m_TextureBindingSystem) {
            spdlog::warn("[RenderCommandBuffer] Failed to create bindless system, using traditional");
            m_TextureBindingSystem = TextureBindingFactory::Create(TextureBindingFactory::BindingType::Traditional);
        }
    }
}

void RenderCommandBuffer::SetTextureBindingSystem(std::unique_ptr<ITextureBindingSystem> bindingSystem)
{
    m_TextureBindingSystem = std::move(bindingSystem);
}

void RenderCommandBuffer::Clear()
{
    m_Commands.clear();
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
        }, sortableCmd);
    }
    
    // End batch operations
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->EndBatch();
    }

    // Final GPU state cleanup
    CleanupGPUState();
}

size_t RenderCommandBuffer::GetMemoryUsage() const
{
    return m_Commands.size() * sizeof(VariantRenderCommand);
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
    if (cmd.shader)
    {
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
    if (!cmd.shader) return;
    
    // Ensure texture binding system is initialized
    if (!m_TextureBindingSystem) {
        spdlog::error("[RenderCommandBuffer] Texture binding system not initialized! Call Initialize() first.");
        return;
    }
    
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

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindSSBOData& cmd)
{
    // Bind SSBO to specified binding point for shader access
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cmd.bindingPoint, cmd.ssboHandle);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawElementsInstancedData& cmd)
{
    // Instanced drawing - assumes SSBO and state are already set up
    glBindVertexArray(cmd.vao);
    glDrawElementsInstanced(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, nullptr,
                           cmd.instanceCount);
    glBindVertexArray(0);

    // Reset texture state using abstraction
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->UnbindAll();
    }

    // Unbind SSBO binding points to prevent state leakage
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);  // Unbind instance data SSBO
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetShadowUniformsData& cmd)
{
    if (!cmd.shader) {
        return;
    }

    // Ensure shader is active
    cmd.shader->use();

    // Set light space matrix uniform
    cmd.shader->setMat4("u_LightSpaceMatrix", cmd.lightSpaceMatrix);

    // Bind shadow map texture to specified unit
    glActiveTexture(GL_TEXTURE0 + cmd.shadowMapUnit);
    glBindTexture(GL_TEXTURE_2D, cmd.shadowMapTexture);
    cmd.shader->setInt("u_ShadowMap", cmd.shadowMapUnit);

    // Note: Shadow map texture will remain bound until explicitly unbound
    // This is intentional as it needs to stay bound for the entire rendering pass
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BlitFramebufferData& cmd)
{

    // Bind source and destination framebuffers
    glBindFramebuffer(GL_READ_FRAMEBUFFER, cmd.srcFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cmd.dstFBO);

    // Perform the blit operation
    glBlitFramebuffer(
        cmd.srcX0, cmd.srcY0, cmd.srcX1, cmd.srcY1,  // Source rectangle
        cmd.dstX0, cmd.dstY0, cmd.dstX1, cmd.dstY1,  // Destination rectangle
        cmd.mask,    // Buffer mask (GL_COLOR_BUFFER_BIT, etc.)
        cmd.filter   // Filter (GL_NEAREST, GL_LINEAR)
    );

    // Restore default framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderCommandBuffer::CleanupGPUState()
{
    // Reset shadow map texture units that may have been bound during shadow mapping
    // This prevents texture unit state leakage between frames
    glActiveTexture(GL_TEXTURE15);  // Shadow map typically uses unit 15
    glBindTexture(GL_TEXTURE_2D, 0);

    // Reset to default texture unit
    glActiveTexture(GL_TEXTURE0);

    // Ensure all SSBO binding points are unbound
    // This prevents SSBO state leakage between frames
    for (int i = 0; i < 4; ++i) {  // Common SSBO binding points 0-3
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);
    }

    // Reset VAO binding to prevent state leakage
    glBindVertexArray(0);

    // Reset shader program to prevent state leakage
    glUseProgram(0);
}