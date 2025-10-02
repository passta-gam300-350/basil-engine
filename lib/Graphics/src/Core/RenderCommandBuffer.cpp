#include "Core/RenderCommandBuffer.h"
#include "Resources/TextureSlotManager.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <cassert>

RenderCommandBuffer::RenderCommandBuffer() {
    // Texture binding system will be set via SetTextureSlotManager()
    m_TextureBindingSystem = nullptr;
}

void RenderCommandBuffer::Submit(const VariantRenderCommand& command)
{
    m_Commands.emplace_back(command);
}


void RenderCommandBuffer::Clear()
{
    m_Commands.clear();
}

void RenderCommandBuffer::Execute()
{
    // Execute commands in order without batching to ensure proper per-object texture uniforms
    for (const auto& sortableCmd : m_Commands) {
        std::visit([this](const auto& cmd) {
            this->ExecuteCommand(cmd);
        }, sortableCmd);
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
    assert(cmd.shader && "BindShaderData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    cmd.shader->use();
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformsData& cmd)
{
    assert(cmd.shader && "SetUniformsData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Set transform matrices
    cmd.shader->setMat4("u_Model", cmd.modelMatrix);
    cmd.shader->setMat4("u_View", cmd.viewMatrix);
    cmd.shader->setMat4("u_Projection", cmd.projectionMatrix);

    // Set camera position for lighting
    cmd.shader->setVec3("u_ViewPos", cmd.cameraPosition);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindTexturesData& cmd)
{
    assert(cmd.shader && "BindTexturesData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(m_TextureBindingSystem && "Texture binding system must be set before binding textures! Call SetTextureSlotManager()");

    // Use slot-based texture system
    m_TextureBindingSystem->BindTextures(cmd.textures, cmd.shader);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawElementsData& cmd)
{
    assert(cmd.vao != 0 && "DrawElementsData command must have a valid VAO handle");
    assert(cmd.indexCount > 0 && "DrawElementsData command must have a positive index count");
    assert(cmd.mode == GL_TRIANGLES || cmd.mode == GL_LINES || cmd.mode == GL_POINTS ||
           cmd.mode == GL_LINE_STRIP || cmd.mode == GL_TRIANGLE_STRIP && "DrawElementsData mode must be a valid OpenGL primitive type");

    glBindVertexArray(cmd.vao);
    glDrawElements(cmd.mode, cmd.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset material texture state (but preserve shadow map binding)
    if (m_TextureBindingSystem)
    {
        // Unbind material texture slots (0-7) but preserve shadow slot (8) and higher
        for (int i = 0; i < 8; ++i) {
            m_TextureBindingSystem->UnbindTexture(i);
        }
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindSSBOData& cmd)
{
    assert(cmd.ssboHandle != 0 && "BindSSBOData command must have a valid SSBO handle");
    assert(cmd.bindingPoint < GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS && "SSBO binding point must be within OpenGL limits");

    // Bind SSBO to specified binding point for shader access
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cmd.bindingPoint, cmd.ssboHandle);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawElementsInstancedData& cmd)
{
    assert(cmd.vao != 0 && "DrawElementsInstancedData command must have a valid VAO handle");
    assert(cmd.indexCount > 0 && "DrawElementsInstancedData command must have a positive index count");
    assert(cmd.instanceCount > 0 && "DrawElementsInstancedData command must have a positive instance count");
    assert(cmd.mode == GL_TRIANGLES || cmd.mode == GL_LINES || cmd.mode == GL_POINTS ||
           cmd.mode == GL_LINE_STRIP || cmd.mode == GL_TRIANGLE_STRIP && "DrawElementsInstancedData mode must be a valid OpenGL primitive type");

    glBindVertexArray(cmd.vao);
    glDrawElementsInstanced(cmd.mode, cmd.indexCount, GL_UNSIGNED_INT, nullptr,
                           cmd.instanceCount);
    glBindVertexArray(0);

    // Reset material texture state (but preserve shadow map binding)
    if (m_TextureBindingSystem)
    {
        // Unbind material texture slots (0-7) but preserve shadow slot (8) and higher
        for (int i = 0; i < 8; ++i) {
            m_TextureBindingSystem->UnbindTexture(i);
        }
    }

    // Unbind SSBO binding points to prevent state leakage
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);  // Unbind instance data SSBO
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetShadowUniformsData& cmd)
{
    assert(cmd.shader && "SetShadowUniformsData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(cmd.shadowMapUnit >= 0 && cmd.shadowMapUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS &&
           "Shadow map texture unit must be within OpenGL limits");

    // Ensure shader is active
    cmd.shader->use();

    // Set light space matrix uniform
    cmd.shader->setMat4("u_LightSpaceMatrix", cmd.lightSpaceMatrix);

    // Bind shadow map texture to specified unit using TextureSlotManager
    if (m_TextureBindingSystem) {
        m_TextureBindingSystem->BindTextureID(cmd.shadowMapTexture, cmd.shadowMapUnit);
    } else {
        // Fallback to manual binding
        glActiveTexture(GL_TEXTURE0 + cmd.shadowMapUnit);
        glBindTexture(GL_TEXTURE_2D, cmd.shadowMapTexture);
    }

    // Always set the uniform to point to the correct texture unit
    cmd.shader->setInt("u_ShadowMap", cmd.shadowMapUnit);

    // Set the shadow enable flag
    cmd.shader->setBool("u_EnableShadows", cmd.enableShadows);

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

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformVec3Data& cmd)
{
    assert(cmd.shader && "SetUniformVec3Data command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Set the Vec3 uniform
    cmd.shader->setVec3(cmd.uniformName, cmd.value);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetBlendingData& cmd)
{
    if (cmd.enable) {
        glEnable(GL_BLEND);
        glBlendFunc(cmd.srcFactor, cmd.dstFactor);
    } else {
        glDisable(GL_BLEND);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetLineWidthData& cmd)
{
    glLineWidth(cmd.width);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetDepthTestData &cmd)
{
    if (cmd.enable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(cmd.depthFunc);
        glDepthMask(cmd.depthWrite ? GL_TRUE : GL_FALSE);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetFaceCullingData &cmd)
{
    if (cmd.enable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(cmd.cullFace);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetObjectIDData& cmd)
{
    assert(cmd.shader && "Shader cannot be null for object ID");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Ensure shader is active
    cmd.shader->use();

    // Convert object ID to normalized color (R channel for simplicity)
    // For 24-bit object IDs, we can use RGB channels if needed
    float normalizedID = static_cast<float>(cmd.objectID) / 16777215.0f; // 2^24 - 1
    cmd.shader->setFloat("u_ObjectID", normalizedID);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::ReadPixelData& cmd)
{
    assert(cmd.outValue && "Output pointer cannot be null");

    // Read a single pixel at the specified coordinates
    // We read as RGBA since that's the typical framebuffer format
    uint8_t pixel[4];
    glReadPixels(cmd.x, cmd.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // Convert back to object ID (assuming we stored it in R channel)
    *cmd.outValue = static_cast<uint32_t>(pixel[0]) |
                   (static_cast<uint32_t>(pixel[1]) << 8) |
                   (static_cast<uint32_t>(pixel[2]) << 16);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindCubemapData &cmd)
{
    assert(cmd.shader && "BindCubemapData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(cmd.cubemapID != 0 && "Cubemap ID must be valid");
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS && "Texture unit must be within OpenGL limits");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind cubemap to specified texture unit
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmd.cubemapID);

    // Set uniform sampler to point to the texture unit
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}

void RenderCommandBuffer::CleanupGPUState()
{
    // Note: We don't reset shadow map texture (slot 8) here since it should persist
    // for the entire rendering pass. Shadow map will be reset at frame end.

    // Reset to default texture unit
    glActiveTexture(GL_TEXTURE0);

    // Ensure SSBO binding points are unbound
    // This prevents SSBO state leakage between frames
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);  // Unbind instance data SSBO
    for (int i = 1; i < 4; ++i) {  // Unbind binding points 1-3
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);
    }

    // Reset VAO binding to prevent state leakage
    glBindVertexArray(0);

    // Reset shader program to prevent state leakage
    glUseProgram(0);
}