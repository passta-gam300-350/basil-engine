#include "Rendering/HUDRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include "Core/RenderCommandBuffer.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <algorithm>

HUDRenderer::HUDRenderer()
    : m_TotalElements(0)
{
    CreateQuadMesh();
    spdlog::info("HUDRenderer initialized");
}

HUDRenderer::~HUDRenderer()
{
    // Cleanup OpenGL resources
    if (m_QuadVAO != 0) {
        glDeleteVertexArrays(1, &m_QuadVAO);
    }
    if (m_QuadVBO != 0) {
        glDeleteBuffers(1, &m_QuadVBO);
    }
    spdlog::info("HUDRenderer destroyed");
}

void HUDRenderer::BeginFrame()
{
    // Clear all batches for new frame
    for (auto& [textureID, batch] : m_TextureBatches) {
        batch.instances.clear();
        batch.dirty = true;
    }
    m_TotalElements = 0;
    /*spdlog::debug("HUDRenderer::BeginFrame() - cleared batches");*/
}

void HUDRenderer::SubmitElement(const HUDElementData& element)
{
    if (!element.visible) {
        spdlog::warn("HUDRenderer::SubmitElement() - element not visible");
        return;
    }

    /*spdlog::info("HUDRenderer::SubmitElement() - textureID={}, pos=({},{}), size=({},{})",
        element.textureID, element.position.x, element.position.y, element.size.x, element.size.y);*/

    // Convert to GPU instance data
    HUDInstanceData instanceData;
    instanceData.position = element.position;
    instanceData.size = element.size;
    instanceData.color = element.color;
    instanceData.rotation = glm::radians(element.rotation);  // Convert to radians
    instanceData.anchor = static_cast<uint32_t>(element.anchor);
    instanceData.padding[0] = 0.0f;
    instanceData.padding[1] = 0.0f;

    // Add to appropriate texture batch
    auto& batch = m_TextureBatches[element.textureID];
    batch.textureID = element.textureID;
    batch.instances.push_back(instanceData);
    batch.dirty = true;

    m_TotalElements++;
    //spdlog::info("HUDRenderer::SubmitElement() - total elements now: {}", m_TotalElements);
}

void HUDRenderer::EndFrame()
{
    /*spdlog::info("HUDRenderer::EndFrame() - total elements: {}, batches: {}",
        m_TotalElements, m_TextureBatches.size());*/

    // Update SSBOs for all dirty batches
    for (auto& [textureID, batch] : m_TextureBatches) {
        if (batch.dirty && !batch.instances.empty()) {
            /*spdlog::info("HUDRenderer::EndFrame() - updating batch for textureID={}, instances={}",
                textureID, batch.instances.size());*/
            UpdateBatchSSBO(batch);
        }
    }
}

void HUDRenderer::RenderToPass(RenderPass& renderPass, const FrameData& frameData)
{
    /*spdlog::info("HUDRenderer::RenderToPass() - batches: {}, elements: {}",
        m_TextureBatches.size(), m_TotalElements);*/

    if (m_TextureBatches.empty() || m_TotalElements == 0) {
        spdlog::warn("HUDRenderer::RenderToPass() - no batches or elements to render");
        return;
    }

    if (!m_HUDShader) {
        spdlog::error("HUDRenderer::RenderToPass() - No shader set!");
        return;
    }

    //spdlog::info("HUDRenderer::RenderToPass() - shader OK, rendering {} batches", m_TextureBatches.size());

    // Sort batches by layer (stored in first instance of each batch)
    // For simplicity, we'll render in texture ID order
    // TODO: Implement proper layer-based sorting across batches

    for (const auto& [textureID, batch] : m_TextureBatches) {
        if (!batch.instances.empty()) {
            RenderBatch(renderPass, batch, frameData);
        }
    }
}

void HUDRenderer::CreateQuadMesh()
{
    // Simple quad in local space (0,0 to 1,1)
    // Position will be calculated in vertex shader based on screen position
    float quadVertices[] = {
        // Positions   // TexCoords
        0.0f, 1.0f,    0.0f, 1.0f,  // Top-left
        0.0f, 0.0f,    0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,    1.0f, 0.0f,  // Bottom-right

        0.0f, 1.0f,    0.0f, 1.0f,  // Top-left
        1.0f, 0.0f,    1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,    1.0f, 1.0f   // Top-right
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    //spdlog::info("HUDRenderer: Quad mesh created");
}

void HUDRenderer::UpdateBatchSSBO(TextureBatch& batch)
{
    if (batch.instances.empty()) {
        return;
    }

    uint32_t instanceDataSize = static_cast<uint32_t>(batch.instances.size() * sizeof(HUDInstanceData));

    // Debug: Log instance data being uploaded
    for (size_t i = 0; i < batch.instances.size(); ++i) {
        const auto& inst = batch.instances[i];
        /*spdlog::info("HUDRenderer::UpdateBatchSSBO() - Instance {}: pos=({},{}), size=({},{}), color=({},{},{},{})",
            i, inst.position.x, inst.position.y, inst.size.x, inst.size.y,
            inst.color.r, inst.color.g, inst.color.b, inst.color.a);*/
    }

    if (!batch.ssbo) {
        // Create new SSBO
        batch.ssbo = std::make_unique<ShaderStorageBuffer>(
            batch.instances.data(),
            instanceDataSize,
            GL_DYNAMIC_DRAW
        );
    } else {
        // Update existing SSBO
        batch.ssbo->SetData(batch.instances.data(), instanceDataSize, 0);
    }

    batch.dirty = false;
}

void HUDRenderer::RenderBatch(RenderPass& renderPass, const TextureBatch& batch, const FrameData& frameData)
{
    if (batch.instances.empty() || !batch.ssbo) {
        spdlog::warn("HUDRenderer::RenderBatch() - empty instances or no SSBO");
        return;
    }

    /*spdlog::info("HUDRenderer::RenderBatch() - rendering textureID={}, instances={}, VAO={}",
        batch.textureID, batch.instances.size(), m_QuadVAO);*/

    // Bind shader
    RenderCommands::BindShaderData bindShaderCmd{ m_HUDShader };
    renderPass.Submit(bindShaderCmd);

    //spdlog::info("HUDRenderer::RenderBatch() - shader bound");

    // Set viewport uniforms (for screen-space calculations)
    RenderCommands::SetUniformVec2Data viewportCmd{
        m_HUDShader,
        "u_ViewportSize",
        glm::vec2(frameData.viewportWidth, frameData.viewportHeight)
    };
    renderPass.Submit(viewportCmd);

    // Bind instance data SSBO via command buffer (so it's bound at the right time during execution)
    uint32_t ssboHandle = batch.ssbo->GetSSBOHandle();
    /*spdlog::info("HUDRenderer::RenderBatch() - submitting SSBO bind command: handle={}, binding={}",
        ssboHandle, HUD_INSTANCE_SSBO_BINDING);*/

    RenderCommands::BindSSBOData bindSSBOCmd{
        ssboHandle,
        HUD_INSTANCE_SSBO_BINDING
    };
    renderPass.Submit(bindSSBOCmd);

    // Bind texture if available
    if (batch.textureID != 0) {
        RenderCommands::BindTextureIDData bindTextureCmd{
            batch.textureID,
            0,  // Texture unit 0
            m_HUDShader,
            "u_Texture"
        };
        renderPass.Submit(bindTextureCmd);

        // Enable texturing
        RenderCommands::SetUniformBoolData useTextureCmd{
            m_HUDShader,
            "u_UseTexture",
            true
        };
        renderPass.Submit(useTextureCmd);
    } else {
        // Disable texturing (solid color)
        RenderCommands::SetUniformBoolData useTextureCmd{
            m_HUDShader,
            "u_UseTexture",
            false
        };
        renderPass.Submit(useTextureCmd);
    }

    // Draw instanced
    uint32_t instanceCount = static_cast<uint32_t>(batch.instances.size());
    /*spdlog::info("HUDRenderer::RenderBatch() - submitting draw command: VAO={}, vertices=6, instances={}",
        m_QuadVAO, instanceCount);*/

    RenderCommands::DrawArraysInstancedData drawCmd{
        m_QuadVAO,
        6,              // 6 vertices per quad
        instanceCount,  // Number of instances
        GL_TRIANGLES,
        0               // Start at vertex 0
    };
    renderPass.Submit(drawCmd);

    //spdlog::info("HUDRenderer::RenderBatch() - draw command submitted");
}
