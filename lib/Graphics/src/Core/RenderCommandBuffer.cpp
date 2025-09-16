#include "Core/RenderCommandBuffer.h"
#include "Resources/TextureBindingSystem.h"
#include <glad/gl.h>
#include <algorithm>
#include <spdlog/spdlog.h>

RenderCommandBuffer::RenderCommandBuffer() {
    // Texture binding system will be initialized explicitly via Initialize()
    m_TextureBindingSystem = nullptr;
}

void RenderCommandBuffer::Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey)
{
    m_Commands.emplace_back(SortableCommand{command, sortKey});
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
            using T = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<T, RenderCommands::ClearData>) {
                std::cout << "ExecuteCommand: ClearData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::BindShaderData>) {
                std::cout << "ExecuteCommand: BindShaderData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::SetUniformsData>) {
                std::cout << "ExecuteCommand: SetUniformsData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::BindTexturesData>) {
                std::cout << "ExecuteCommand: BindTexturesData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::DrawElementsData>) {
                std::cout << "ExecuteCommand: DrawElementsData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::BindSSBOData>) {
                std::cout << "ExecuteCommand: BindSSBOData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::DrawElementsInstancedData>) {
                std::cout << "ExecuteCommand: DrawElementsInstancedData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::SetShadowUniformsData>) {
                std::cout << "ExecuteCommand: SetShadowUniformsData" << std::endl;
            } else if constexpr (std::is_same_v<T, RenderCommands::BlitFramebufferData>) {
                std::cout << "ExecuteCommand: BlitFramebufferData" << std::endl;
            }
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
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetShadowUniformsData& cmd)
{
    if (!cmd.shader) {
        std::cout << "ExecuteCommand: SetShadowUniformsData - NULL shader!" << std::endl;
        return;
    }

    // Ensure shader is active
    cmd.shader->use();

    // Set light space matrix uniform
    cmd.shader->setMat4("u_LightSpaceMatrix", cmd.lightSpaceMatrix);

    // Bind shadow map texture
    glActiveTexture(GL_TEXTURE0 + cmd.shadowMapUnit);
    glBindTexture(GL_TEXTURE_2D, cmd.shadowMapTexture);
    cmd.shader->setInt("u_ShadowMap", cmd.shadowMapUnit);
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