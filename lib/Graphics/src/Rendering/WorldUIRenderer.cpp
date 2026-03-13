/******************************************************************************/
/*!
\file   WorldUIRenderer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/15
\brief  World-space UI renderer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Rendering/WorldUIRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include "Core/RenderCommandBuffer.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

WorldUIRenderer::WorldUIRenderer()
    : m_TotalElements(0)
{
    CreateQuadMesh();
    spdlog::info("WorldUIRenderer initialized");
}

WorldUIRenderer::~WorldUIRenderer()
{
    if (m_QuadVAO != 0) {
        glDeleteVertexArrays(1, &m_QuadVAO);
    }
    if (m_QuadVBO != 0) {
        glDeleteBuffers(1, &m_QuadVBO);
    }
    spdlog::info("WorldUIRenderer destroyed");
}

void WorldUIRenderer::BeginFrame()
{
    for (auto& [textureID, batch] : m_TextureBatches) {
        batch.instances.resize(0);
        batch.dirty = true;
    }
    m_TotalElements = 0;
}

void WorldUIRenderer::SubmitElement(const WorldUIElementData& element)
{
    if (!element.visible) return;

    // Calculate billboard vectors
    glm::vec3 billboardRight, billboardUp;
    CalculateBillboardVectors(element, billboardRight, billboardUp);

    // Convert to GPU instance data
    WorldUIInstanceData instanceData;
    instanceData.worldPosition = element.worldPosition;
    instanceData._pad0 = 0.0f;
    instanceData.billboardRight = billboardRight;
    instanceData._pad1 = 0.0f;
    instanceData.billboardUp = billboardUp;
    instanceData._pad2 = 0.0f;
    instanceData.size = element.size;
    instanceData._pad3 = glm::vec2(0.0f);
    instanceData.uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Full UV rect
    instanceData.color = element.color;

    // Add to appropriate texture batch
    auto [it, inserted] = m_TextureBatches.try_emplace(element.textureID);
    auto& batch = it->second;
    if (inserted) {
        batch.textureID = element.textureID;
        batch.instances.reserve(64);
    }
    batch.instances.push_back(instanceData);
    batch.dirty = true;

    m_TotalElements++;
}

void WorldUIRenderer::EndFrame()
{
    for (auto& [textureID, batch] : m_TextureBatches) {
        if (batch.dirty && !batch.instances.empty()) {
            UpdateBatchSSBO(batch);
        }
    }
}

void WorldUIRenderer::RenderToPass(RenderPass& renderPass, const FrameData& frameData)
{
    if (m_TextureBatches.empty() || m_TotalElements == 0) return;

    if (!m_WorldUIShader) {
        spdlog::error("WorldUIRenderer::RenderToPass() - No shader set!");
        return;
    }

    // Bind shader once for all batches
    RenderCommands::BindShaderData bindShaderCmd{ m_WorldUIShader };
    renderPass.Submit(bindShaderCmd);

    // Set view/projection uniforms
    RenderCommands::SetUniformMat4Data viewCmd{
        m_WorldUIShader,
        "u_View",
        frameData.viewMatrix
    };
    renderPass.Submit(viewCmd);

    RenderCommands::SetUniformMat4Data projCmd{
        m_WorldUIShader,
        "u_Projection",
        frameData.projectionMatrix
    };
    renderPass.Submit(projCmd);

    // Render each batch
    for (const auto& [textureID, batch] : m_TextureBatches) {
        if (!batch.instances.empty()) {
            RenderBatch(renderPass, batch, frameData);
        }
    }
}

void WorldUIRenderer::CreateQuadMesh()
{
    // Simple quad (0,0 to 1,1) with texture coordinates
    float quadVertices[] = {
        // Positions   // TexCoords
        0.0f, 1.0f,    0.0f, 1.0f,  // Top-left
        0.0f, 0.0f,    0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,    1.0f, 0.0f,  // Bottom-right

        0.0f, 1.0f,    0.0f, 1.0f,  // Top-left
        1.0f, 0.0f,    1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,    1.0f, 1.0f   // Top-right
    };

    // DSA: Create VAO and VBO without binding
    glCreateVertexArrays(1, &m_QuadVAO);
    glCreateBuffers(1, &m_QuadVBO);

    // DSA: Upload vertex data to VBO
    glNamedBufferData(m_QuadVBO, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // DSA: Associate VBO with VAO at binding index 0
    GLsizei stride = 4 * sizeof(float);
    glVertexArrayVertexBuffer(m_QuadVAO, 0, m_QuadVBO, 0, stride);

    // DSA: Position attribute (location = 0)
    glEnableVertexArrayAttrib(m_QuadVAO, 0);
    glVertexArrayAttribFormat(m_QuadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_QuadVAO, 0, 0);  // Attribute 0 -> Binding 0

    // DSA: TexCoord attribute (location = 1)
    glEnableVertexArrayAttrib(m_QuadVAO, 1);
    glVertexArrayAttribFormat(m_QuadVAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(m_QuadVAO, 1, 0);  // Attribute 1 -> Binding 0
}

void WorldUIRenderer::UpdateBatchSSBO(TextureBatch& batch)
{
    if (batch.instances.empty()) return;

    uint32_t instanceDataSize = static_cast<uint32_t>(batch.instances.size() * sizeof(WorldUIInstanceData));

    if (!batch.ssbo) {
        batch.ssbo = std::make_unique<ShaderStorageBuffer>(
            batch.instances.data(),
            instanceDataSize,
            GL_DYNAMIC_DRAW
        );
    } else {
        if (instanceDataSize > batch.ssbo->GetSize()) {
            uint32_t newSize = instanceDataSize * 2;
            batch.ssbo->Resize(newSize);
		}
        batch.ssbo->SetData(batch.instances.data(), instanceDataSize, 0);
    }

    batch.dirty = false;
}

void WorldUIRenderer::RenderBatch(RenderPass& renderPass, const TextureBatch& batch, const FrameData& /*frameData*/)
{
    if (batch.instances.empty() || !batch.ssbo) return;

    // Bind instance data SSBO
    RenderCommands::BindSSBOData bindSSBOCmd{
        batch.ssbo->GetSSBOHandle(),
        WORLD_UI_INSTANCE_SSBO_BINDING
    };
    renderPass.Submit(bindSSBOCmd);

    // Bind texture if available
    if (batch.textureID != 0) {
        RenderCommands::BindTextureIDData bindTextureCmd{
            batch.textureID,
            0,  // Texture unit 0
            m_WorldUIShader,
            "u_Texture"
        };
        renderPass.Submit(bindTextureCmd);

        RenderCommands::SetUniformBoolData useTextureCmd{
            m_WorldUIShader,
            "u_HasTexture",
            true
        };
        renderPass.Submit(useTextureCmd);
    } else {
        RenderCommands::SetUniformBoolData useTextureCmd{
            m_WorldUIShader,
            "u_HasTexture",
            false
        };
        renderPass.Submit(useTextureCmd);
    }

    // Draw instanced
    uint32_t instanceCount = static_cast<uint32_t>(batch.instances.size());
    RenderCommands::DrawArraysInstancedData drawCmd{
        m_QuadVAO,
        6,
        instanceCount,
        GL_TRIANGLES,
        0
    };
    renderPass.Submit(drawCmd);
}

void WorldUIRenderer::CalculateBillboardVectors(
    const WorldUIElementData& element,
    glm::vec3& outRight,
    glm::vec3& outUp) const
{
    switch (element.billboardMode) {
        case WorldUIBillboardMode::Full: {
            // Full billboard - always faces camera
            glm::vec3 toCamera = glm::normalize(element.cameraPosition - element.worldPosition);
            outRight = glm::normalize(glm::cross(element.cameraUp, toCamera));
            outUp = -glm::normalize(glm::cross(outRight, toCamera));
            break;
        }

        case WorldUIBillboardMode::Cylindrical: {
            // Cylindrical billboard - rotate around Y-axis only
            glm::vec3 toCamera = element.cameraPosition - element.worldPosition;
            toCamera.y = 0.0f;
            if (glm::length(toCamera) < 0.001f) {
                toCamera = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            toCamera = glm::normalize(toCamera);
            outRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCamera));
            outUp = glm::vec3(0, 1, 0);
            break;
        }

        case WorldUIBillboardMode::None:
        default: {
            // No billboard - use custom rotation from transform
            outRight = glm::vec3(element.customRotation[0]);
            outUp = glm::vec3(element.customRotation[1]);
            break;
        }
    }
}
