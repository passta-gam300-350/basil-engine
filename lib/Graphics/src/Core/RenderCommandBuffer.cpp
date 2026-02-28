/******************************************************************************/
/*!
\file   RenderCommandBuffer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Render command buffer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Core/RenderCommandBuffer.h"
#include "Resources/TextureSlotManager.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <cassert>

RenderCommandBuffer::RenderCommandBuffer() {
    // Texture binding system will be set via SetTextureSlotManager()
    m_TextureBindingSystem = nullptr;

    // Pre-allocate command buffer to avoid reallocations during rendering
    m_Commands.reserve(256);
}

void RenderCommandBuffer::Submit(const VariantRenderCommand& command)
{
    m_Commands.emplace_back(command);
}


void RenderCommandBuffer::Clear()
{
    m_Commands.clear();
    m_CurrentShaderID = 0;  // Reset shader tracking
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

    if (cmd.clearStencil) {
        glClearStencil(0);  // Clear stencil buffer to 0
    }

    GLbitfield mask = 0;
    if (cmd.clearColor) mask |= GL_COLOR_BUFFER_BIT;
    if (cmd.clearDepth) mask |= GL_DEPTH_BUFFER_BIT;
    if (cmd.clearStencil) mask |= GL_STENCIL_BUFFER_BIT;

    if (mask != 0) {
        glClear(mask);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindShaderData& cmd)
{
    assert(cmd.shader && "BindShaderData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Update current shader tracking
    cmd.shader->use();
    m_CurrentShaderID = cmd.shader->ID;
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

    // PERFORMANCE: Disabled texture unbinding after each draw call
    // Rebinding textures for the next draw is sufficient - no need to unbind
    // This saves hundreds of glBindTexture(slot, 0) calls per frame
    // Original code was unbinding 8 slots × 90 meshes = 720 calls per frame!
    /*if (m_TextureBindingSystem)
    {
        // Unbind material texture slots (0-7) but preserve shadow slot (8) and higher
        for (int i = 0; i < 8; ++i) {
            m_TextureBindingSystem->UnbindTexture(i);
        }
    }*/
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

    // PERFORMANCE: Disabled texture unbinding after each draw call
    // Rebinding textures and SSBOs for the next draw is sufficient
    // Original code: 90 meshes × (8 texture unbinds + 1 SSBO unbind) = 810 calls per frame!
    /*if (m_TextureBindingSystem)
    {
        // Unbind material texture slots (0-7) but preserve shadow slot (8) and higher
        for (int i = 0; i < 8; ++i) {
            m_TextureBindingSystem->UnbindTexture(i);
        }
    }

    // Unbind SSBO binding points to prevent state leakage
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);  // Unbind instance data SSBO*/
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

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformFloatData& cmd)
{
    assert(cmd.shader && "SetUniformFloatData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Set the Float uniform
    cmd.shader->setFloat(cmd.uniformName, cmd.value);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformIntData& cmd)
{
    assert(cmd.shader && "SetUniformIntData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Set the Int uniform
    cmd.shader->setInt(cmd.uniformName, cmd.value);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformBoolData& cmd)
{
    assert(cmd.shader && "SetUniformBoolData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Only bind shader if it's not already current (avoid redundant state changes)
    if (cmd.shader->ID != m_CurrentShaderID) {
        cmd.shader->use();
        m_CurrentShaderID = cmd.shader->ID;
    }

    // Set the Bool uniform
    cmd.shader->setBool(cmd.uniformName, cmd.value);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformMat4Data& cmd)
{
    assert(cmd.shader && "SetUniformMat4Data command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Set the mat4 uniform
    cmd.shader->setMat4(cmd.uniformName, cmd.value);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformMat4ArrayData& cmd)
{
    assert(cmd.shader && "SetUniformMat4ArrayData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(!cmd.uniformBaseName.empty() && "Uniform base name cannot be empty");
    assert(!cmd.matrices.empty() && "Matrix array cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Set each matrix in the array using indexed uniform names
    // e.g., "u_ShadowMatrices[0]", "u_ShadowMatrices[1]", etc.
    for (size_t i = 0; i < cmd.matrices.size(); ++i) {
        std::string uniformName = cmd.uniformBaseName + "[" + std::to_string(i) + "]";
        cmd.shader->setMat4(uniformName, cmd.matrices[i]);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetBlendingData& cmd)
{
    if (cmd.enable) {
        glEnable(GL_BLEND);
        glBlendFunc(cmd.srcFactor, cmd.dstFactor);
        glBlendEquation(cmd.blendEquation);
    } else {
        glDisable(GL_BLEND);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetViewportData& cmd)
{
    glViewport(cmd.x, cmd.y, cmd.width, cmd.height);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetUniformVec2Data& cmd)
{
    if (cmd.shader) {
        // Only bind shader if it's not already current (avoid redundant state changes)
        if (cmd.shader->ID != m_CurrentShaderID) {
            cmd.shader->use();
            m_CurrentShaderID = cmd.shader->ID;
        }
        cmd.shader->setVec2(cmd.uniformName, cmd.value);
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
    // Note: cubemapID can be 0 for unbinding
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS && "Texture unit must be within OpenGL limits");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind cubemap to specified texture unit (or unbind if ID is 0)
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmd.cubemapID);

    // Set uniform sampler to point to the texture unit
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindTextureIDData &cmd)
{
    assert(cmd.shader && "BindTextureIDData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    // Note: textureID can be 0 for unbinding
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS && "Texture unit must be within OpenGL limits");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind 2D texture to specified texture unit (or unbind if ID is 0)
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_2D, cmd.textureID);

    // Set uniform sampler to point to the texture unit
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindTexture3DData &cmd)
{
    assert(cmd.shader && "BindTexture3DData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS && "Texture unit exceeds OpenGL limits");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind 3D texture to specified texture unit
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_3D, cmd.textureID);

    // Set uniform sampler to point to the texture unit
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindTexture2DArrayData &cmd)
{
    assert(cmd.shader && "BindTexture2DArrayData command must have a valid shader");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS && "Texture unit exceeds OpenGL limits");
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind 2D texture array to specified texture unit
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, cmd.textureID);

    // Set uniform sampler to point to the texture unit
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawArraysData &cmd)
{
    assert(cmd.vao != 0 && "DrawArraysData command must have a valid VAO handle");
    assert(cmd.vertexCount > 0 && "DrawArraysData command must have a positive vertex count");
    assert(cmd.mode == GL_TRIANGLES || cmd.mode == GL_LINES || cmd.mode == GL_POINTS ||
           cmd.mode == GL_LINE_STRIP || cmd.mode == GL_TRIANGLE_STRIP && "DrawArraysData mode must be a valid OpenGL primitive type");

    // Bind VAO
    glBindVertexArray(cmd.vao);

    // Draw arrays (non-indexed rendering)
    glDrawArrays(cmd.mode, cmd.first, cmd.vertexCount);

    // Unbind VAO
    glBindVertexArray(0);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DrawArraysInstancedData& cmd)
{
    assert(cmd.vao != 0 && "DrawArraysInstancedData command must have a valid VAO handle");
    assert(cmd.vertexCount > 0 && "DrawArraysInstancedData command must have a positive vertex count");
    assert(cmd.instanceCount > 0 && "DrawArraysInstancedData command must have a positive instance count");
    assert(cmd.mode == GL_TRIANGLES || cmd.mode == GL_LINES || cmd.mode == GL_POINTS ||
           cmd.mode == GL_LINE_STRIP || cmd.mode == GL_TRIANGLE_STRIP && "DrawArraysInstancedData mode must be a valid OpenGL primitive type");

    // Bind VAO
    glBindVertexArray(cmd.vao);

    // Draw instanced arrays (non-indexed rendering with instancing)
    glDrawArraysInstanced(cmd.mode, cmd.first, cmd.vertexCount, cmd.instanceCount);

    // Unbind VAO
    glBindVertexArray(0);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::DispatchComputeData &cmd)
{
    assert(cmd.computeShader && "DispatchComputeData command must have a valid compute shader");
    assert(cmd.computeShader->ID != 0 && "Compute shader program must be compiled and linked");
    assert(cmd.numGroupsX > 0 && "Number of X work groups must be positive");
    assert(cmd.numGroupsY > 0 && "Number of Y work groups must be positive");
    assert(cmd.numGroupsZ > 0 && "Number of Z work groups must be positive");

    // Activate compute shader
    cmd.computeShader->use();

    // Dispatch compute shader work groups
    glDispatchCompute(cmd.numGroupsX, cmd.numGroupsY, cmd.numGroupsZ);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::MemoryBarrierData &cmd)
{
    assert(cmd.barriers != 0 && "MemoryBarrierData command must have valid barrier flags");

    // Insert memory barrier to ensure compute shader writes complete
    glMemoryBarrier(cmd.barriers);
}

// ===== LIGHTING COMMAND EXECUTION (Option A) =====

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetPointLightsData& cmd)
{
    assert(cmd.shader && "Shader cannot be null for point lights");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Activate shader
    cmd.shader->use();

    // Set number of point lights
    cmd.shader->setInt("u_NumPointLights", static_cast<int>(cmd.lights.size()));

    // Set up each point light's properties
    for (size_t i = 0; i < cmd.lights.size(); ++i) {
        const auto& light = cmd.lights[i];
        std::string prefix = "u_PointLights[" + std::to_string(i) + "].";

        cmd.shader->setVec3(prefix + "position", light.position);
        cmd.shader->setVec3(prefix + "color", light.color);
        cmd.shader->setFloat(prefix + "intensity", light.intensity);
        cmd.shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);
        cmd.shader->setFloat(prefix + "constant", light.constant);
        cmd.shader->setFloat(prefix + "linear", light.linear);
        cmd.shader->setFloat(prefix + "quadratic", light.quadratic);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetDirectionalLightsData& cmd)
{
    assert(cmd.shader && "Shader cannot be null for directional lights");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Activate shader
    cmd.shader->use();

    // Set number of directional lights
    cmd.shader->setInt("u_NumDirectionalLights", static_cast<int>(cmd.lights.size()));

    // Set up each directional light's properties
    for (size_t i = 0; i < cmd.lights.size(); ++i) {
        const auto& light = cmd.lights[i];
        std::string prefix = "u_DirectionalLights[" + std::to_string(i) + "].";

        cmd.shader->setVec3(prefix + "direction", light.direction);
        cmd.shader->setVec3(prefix + "color", light.color);
        cmd.shader->setFloat(prefix + "intensity", light.intensity);
        cmd.shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetSpotLightsData& cmd)
{
    assert(cmd.shader && "Shader cannot be null for spot lights");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Activate shader
    cmd.shader->use();

    // Set number of spot lights
    cmd.shader->setInt("u_NumSpotLights", static_cast<int>(cmd.lights.size()));

    // Set up each spot light's properties
    for (size_t i = 0; i < cmd.lights.size(); ++i) {
        const auto& light = cmd.lights[i];
        std::string prefix = "u_SpotLights[" + std::to_string(i) + "].";

        cmd.shader->setVec3(prefix + "position", light.position);
        cmd.shader->setVec3(prefix + "direction", light.direction);
        cmd.shader->setVec3(prefix + "color", light.color);
        cmd.shader->setFloat(prefix + "intensity", light.intensity);
        cmd.shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);
        cmd.shader->setFloat(prefix + "cutOff", light.cutOff);
        cmd.shader->setFloat(prefix + "outerCutOff", light.outerCutOff);
        cmd.shader->setFloat(prefix + "constant", light.constant);
        cmd.shader->setFloat(prefix + "linear", light.linear);
        cmd.shader->setFloat(prefix + "quadratic", light.quadratic);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetMaterialPBRData& cmd)
{
    assert(cmd.shader && "Shader cannot be null for material properties");
    assert(cmd.shader->ID != 0 && "Shader program must be compiled and linked");

    // Activate shader
    cmd.shader->use();

    // Set PBR material properties
    cmd.shader->setVec3("u_AlbedoColor", cmd.albedoColor);
    cmd.shader->setFloat("u_MetallicValue", cmd.metallicValue);
    cmd.shader->setFloat("u_RoughnessValue", cmd.roughnessValue);
}

// ===== STENCIL COMMAND EXECUTION =====
void RenderCommandBuffer::ExecuteCommand(const RenderCommands::EnableStencilTestData& cmd)
{
    if (cmd.enable) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetStencilFuncData& cmd)
{
    glStencilFunc(cmd.func, cmd.ref, cmd.mask);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetStencilOpData& cmd)
{
    glStencilOp(cmd.sfail, cmd.dpfail, cmd.dppass);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetStencilMaskData& cmd)
{
    glStencilMask(cmd.mask);
}

void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetColorMaskData& cmd)
{
    glColorMask(cmd.r, cmd.g, cmd.b, cmd.a);
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