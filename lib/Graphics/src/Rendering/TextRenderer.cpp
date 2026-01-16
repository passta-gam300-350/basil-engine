/******************************************************************************/
/*!
\file   TextRenderer.cpp
\author Claude Code
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Implementation of SDF text rendering with GPU instancing

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Rendering/TextRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include "Core/RenderCommandBuffer.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <codecvt>
#include <locale>

TextRenderer::TextRenderer()
    : m_TotalGlyphs(0)
{
    CreateQuadMesh();
    spdlog::info("TextRenderer initialized");
}

TextRenderer::~TextRenderer()
{
    // Cleanup OpenGL resources
    if (m_QuadVAO != 0) {
        glDeleteVertexArrays(1, &m_QuadVAO);
    }
    if (m_QuadVBO != 0) {
        glDeleteBuffers(1, &m_QuadVBO);
    }
    spdlog::info("TextRenderer destroyed");
}

void TextRenderer::BeginFrame()
{
    // Clear all batches for new frame (use resize(0) to preserve capacity)
    for (auto& [atlasID, batch] : m_FontBatches) {
        batch.glyphInstances.resize(0);  // Clear without deallocating
        batch.dirty = true;
    }
    m_TotalGlyphs = 0;
}

void TextRenderer::SubmitText(const TextElementData& textElement)
{
    if (!textElement.visible || textElement.text.empty() || !textElement.fontAtlas) {
        return;
    }

    // Get font batch (or create new one)
    uint32_t atlasTextureID = textElement.fontAtlas->GetTextureID();
    auto [it, inserted] = m_FontBatches.try_emplace(atlasTextureID);
    auto& batch = it->second;

    if (inserted) {
        batch.atlasTextureID = atlasTextureID;
        batch.isMultiChannelSDF = textElement.fontAtlas->IsMultiChannelSDF();
        batch.glyphInstances.reserve(256);  // Pre-allocate for typical text length
    }

    // Layout text into glyphs
    LayoutText(textElement, batch);
}

void TextRenderer::EndFrame()
{
    // Update SSBOs for all dirty batches
    for (auto& [atlasID, batch] : m_FontBatches) {
        if (batch.dirty && !batch.glyphInstances.empty()) {
            UpdateBatchSSBO(batch);
        }
    }
}

void TextRenderer::RenderToPass(RenderPass& renderPass, const FrameData& frameData)
{
    if (m_FontBatches.empty() || m_TotalGlyphs == 0) {
        return;
    }

    if (!m_TextShader) {
        spdlog::error("TextRenderer::RenderToPass() - No shader set!");
        return;
    }

    // ========== SHARED STATE SETUP (ONCE FOR ALL BATCHES) ==========
    // Bind shader once for all batches
    RenderCommands::BindShaderData bindShaderCmd{ m_TextShader };
    renderPass.Submit(bindShaderCmd);

    // Set viewport uniforms once
    RenderCommands::SetUniformVec2Data viewportCmd{
        m_TextShader,
        "u_ViewportSize",
        glm::vec2(frameData.viewportWidth, frameData.viewportHeight)
    };
    renderPass.Submit(viewportCmd);

    // Set reference resolution uniform once
    RenderCommands::SetUniformVec2Data referenceResolutionCmd{
        m_TextShader,
        "u_ReferenceResolution",
        m_ReferenceResolution
    };
    renderPass.Submit(referenceResolutionCmd);

    // ========== PER-BATCH RENDERING ==========
    for (const auto& [atlasID, batch] : m_FontBatches) {
        if (!batch.glyphInstances.empty()) {
            RenderBatch(renderPass, batch, frameData);
        }
    }
}

void TextRenderer::CreateQuadMesh()
{
    // Simple quad in local space (0,0 to 1,1)
    // Position will be calculated in vertex shader based on glyph position
    float quadVertices[] = {
        // Positions   // TexCoords (will be replaced by glyph UVs in shader)
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
}

void TextRenderer::UpdateBatchSSBO(FontBatch& batch)
{
    if (batch.glyphInstances.empty()) {
        return;
    }

    uint32_t instanceDataSize = static_cast<uint32_t>(batch.glyphInstances.size() * sizeof(GlyphInstanceData));

    if (!batch.ssbo) {
        // Create new SSBO
        batch.ssbo = std::make_unique<ShaderStorageBuffer>(
            batch.glyphInstances.data(),
            instanceDataSize,
            GL_DYNAMIC_DRAW
        );
    } else {
        // Update existing SSBO
        batch.ssbo->SetData(batch.glyphInstances.data(), instanceDataSize, 0);
    }

    batch.dirty = false;
}

void TextRenderer::RenderBatch(RenderPass& renderPass, const FontBatch& batch, const FrameData& frameData)
{
    if (batch.glyphInstances.empty() || !batch.ssbo) {
        return;
    }

    // Bind instance data SSBO
    uint32_t ssboHandle = batch.ssbo->GetSSBOHandle();
    RenderCommands::BindSSBOData bindSSBOCmd{
        ssboHandle,
        GLYPH_INSTANCE_SSBO_BINDING
    };
    renderPass.Submit(bindSSBOCmd);

    // Bind font atlas texture
    RenderCommands::BindTextureIDData bindTextureCmd{
        batch.atlasTextureID,
        0,  // Texture unit 0
        m_TextShader,
        "u_SDFAtlas"
    };
    renderPass.Submit(bindTextureCmd);

    // Set multi-channel SDF flag
    RenderCommands::SetUniformBoolData multiChannelCmd{
        m_TextShader,
        "u_MultiChannelSDF",
        batch.isMultiChannelSDF
    };
    renderPass.Submit(multiChannelCmd);

    // Draw instanced
    uint32_t instanceCount = static_cast<uint32_t>(batch.glyphInstances.size());
    RenderCommands::DrawArraysInstancedData drawCmd{
        m_QuadVAO,
        6,              // 6 vertices per quad
        instanceCount,  // Number of glyph instances
        GL_TRIANGLES,
        0               // Start at vertex 0
    };
    renderPass.Submit(drawCmd);
}

void TextRenderer::LayoutText(const TextElementData& textElement, FontBatch& batch)
{
    const FontAtlas* fontAtlas = textElement.fontAtlas;
    if (!fontAtlas) return;

    // Calculate font scale from base size to desired size
    float fontScale = textElement.fontSize / static_cast<float>(fontAtlas->GetBaseFontSize());

    // Starting cursor position
    glm::vec2 cursor = textElement.position;

    // Decode UTF-8 and layout glyphs
    const char* str = textElement.text.c_str();
    char32_t prevCodepoint = 0;

    while (*str) {
        // Decode UTF-8 codepoint
        char32_t codepoint = DecodeUTF8(str);
        if (codepoint == 0) break;

        // Handle newline
        if (codepoint == '\n') {
            cursor.x = textElement.position.x;
            cursor.y += fontAtlas->GetLineHeight() * fontScale * textElement.lineSpacing;
            prevCodepoint = 0;
            continue;
        }

        // Get glyph data
        const GlyphData* glyph = fontAtlas->GetGlyph(codepoint);
        if (!glyph) {
            // Missing glyph, try to use space or skip
            glyph = fontAtlas->GetGlyph(' ');
            if (!glyph) {
                prevCodepoint = codepoint;
                continue;
            }
        }

        // Apply kerning
        if (prevCodepoint != 0) {
            float kerning = fontAtlas->GetKerning(prevCodepoint, codepoint);
            cursor.x += kerning * fontScale;
        }

        // Calculate glyph position
        glm::vec2 glyphPos = cursor + glyph->bearing * fontScale;
        glm::vec2 glyphSize = glyph->size * fontScale;

        // Create glyph instance
        GlyphInstanceData glyphInstance;
        glyphInstance.position = glyphPos;
        glyphInstance.size = glyphSize;
        glyphInstance.uvRect = glm::vec4(glyph->uv_min, glyph->uv_max);
        glyphInstance.color = textElement.color;
        glyphInstance.sdfThreshold = textElement.sdfThreshold;
        glyphInstance.smoothing = textElement.smoothing;
        glyphInstance.outlineWidth = textElement.outlineWidth;
        glyphInstance.glowStrength = textElement.glowStrength;
        glyphInstance.outlineColor = textElement.outlineColor;
        glyphInstance.glowColor = textElement.glowColor;
        glyphInstance.rotation = glm::radians(textElement.rotation);
        glyphInstance.anchor = static_cast<uint32_t>(textElement.anchor);
        glyphInstance.padding[0] = 0.0f;
        glyphInstance.padding[1] = 0.0f;

        batch.glyphInstances.push_back(glyphInstance);
        m_TotalGlyphs++;

        // Advance cursor
        cursor.x += (glyph->advance + textElement.letterSpacing) * fontScale;
        prevCodepoint = codepoint;
    }

    batch.dirty = true;
}

char32_t TextRenderer::DecodeUTF8(const char*& str)
{
    if (!str || !*str) return 0;

    unsigned char c = static_cast<unsigned char>(*str++);

    // 1-byte character (ASCII)
    if (c < 0x80) {
        return c;
    }

    // 2-byte character
    if ((c & 0xE0) == 0xC0) {
        char32_t codepoint = (c & 0x1F) << 6;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F);
        return codepoint;
    }

    // 3-byte character
    if ((c & 0xF0) == 0xE0) {
        char32_t codepoint = (c & 0x0F) << 12;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F) << 6;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F);
        return codepoint;
    }

    // 4-byte character
    if ((c & 0xF8) == 0xF0) {
        char32_t codepoint = (c & 0x07) << 18;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F) << 12;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F) << 6;
        c = static_cast<unsigned char>(*str++);
        codepoint |= (c & 0x3F);
        return codepoint;
    }

    // Invalid UTF-8 sequence
    return 0;
}

std::vector<std::string> TextRenderer::WrapText(const std::string& text, const FontAtlas* fontAtlas,
                                                  float fontSize, float maxWidth)
{
    std::vector<std::string> lines;
    if (maxWidth <= 0.0f || !fontAtlas) {
        lines.push_back(text);
        return lines;
    }

    // TODO: Implement word wrapping algorithm
    // For now, just return the text as a single line
    lines.push_back(text);
    return lines;
}
