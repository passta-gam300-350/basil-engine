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

    // Clear world-space text batches
    for (auto& [atlasID, batch] : m_WorldFontBatches) {
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

void TextRenderer::SubmitWorldText(const WorldTextElementData& worldText)
{
    if (!worldText.visible || worldText.text.empty() || !worldText.fontAtlas) {
        return;
    }

    // Get world font batch (or create new one)
    uint32_t atlasTextureID = worldText.fontAtlas->GetTextureID();
    auto [it, inserted] = m_WorldFontBatches.try_emplace(atlasTextureID);
    auto& batch = it->second;

    if (inserted) {
        batch.atlasTextureID = atlasTextureID;
        batch.isMultiChannelSDF = worldText.fontAtlas->IsMultiChannelSDF();
        batch.glyphInstances.reserve(256);  // Pre-allocate for typical text length
    }

    // Layout world-space text into glyphs
    LayoutWorldText(worldText, batch);
    /*spdlog::info("TextRenderer::SubmitWorldText() - batch for atlas {} now has {} glyphs",
        atlasTextureID, batch.glyphInstances.size());*/
}

void TextRenderer::EndFrame()
{
    // Update SSBOs for all dirty batches (screen-space)
    for (auto& [atlasID, batch] : m_FontBatches) {
        if (batch.dirty && !batch.glyphInstances.empty()) {
            UpdateBatchSSBO(batch);
        }
    }

    // Update SSBOs for world-space text batches
    for (auto& [atlasID, batch] : m_WorldFontBatches) {
        if (batch.dirty && !batch.glyphInstances.empty()) {
            UpdateWorldBatchSSBO(batch);
            /*spdlog::info("TextRenderer::EndFrame() - Updated SSBO for atlas {}: {} glyphs",
                atlasID, batch.glyphInstances.size());*/
        }
    }
    if (!m_WorldFontBatches.empty()) {
        /*spdlog::info("TextRenderer::EndFrame() - {} world batches, total glyphs: {}",
            m_WorldFontBatches.size(), m_TotalGlyphs);*/
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
        // Positions (Y-down)   // TexCoords (Y-up for correct texture sampling)
        0.0f, 0.0f,    0.0f, 1.0f,  // Top-left position, top-left texcoord
        0.0f, 1.0f,    0.0f, 0.0f,  // Bottom-left position, bottom-left texcoord
        1.0f, 1.0f,    1.0f, 0.0f,  // Bottom-right position, bottom-right texcoord

        0.0f, 0.0f,    0.0f, 1.0f,  // Top-left
        1.0f, 1.0f,    1.0f, 0.0f,  // Bottom-right
        1.0f, 0.0f,    1.0f, 1.0f   // Top-right position, top-right texcoord
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
        // Pre-allocate with 2x growth factor to minimize future resizes
        // Minimum allocation: 64 glyphs worth of space
        constexpr uint32_t MIN_ALLOCATION = 64 * sizeof(GlyphInstanceData);
        uint32_t allocSize = std::max(instanceDataSize * 2, MIN_ALLOCATION);

        // Create buffer with pre-allocated size (no initial data)
        batch.ssbo = std::make_unique<ShaderStorageBuffer>(nullptr, allocSize, GL_DYNAMIC_DRAW);

        // Upload actual glyph data
        batch.ssbo->SetData(batch.glyphInstances.data(), instanceDataSize, 0);
    } else {
        // Resize buffer if new data exceeds capacity (with 2x growth factor)
        if (instanceDataSize > batch.ssbo->GetSize()) {
            uint32_t newSize = instanceDataSize * 2; // Pre-allocate 2x for future growth
            batch.ssbo->Resize(newSize);
        }
        // Update existing SSBO (only write actual data, not full buffer)
        batch.ssbo->SetData(batch.glyphInstances.data(), instanceDataSize, 0);
    }

    batch.dirty = false;
}

void TextRenderer::UpdateWorldBatchSSBO(WorldFontBatch& batch)
{
    if (batch.glyphInstances.empty()) {
        return;
    }

    uint32_t instanceDataSize = static_cast<uint32_t>(batch.glyphInstances.size() * sizeof(WorldGlyphInstanceData));

    if (!batch.ssbo) {
        // Pre-allocate with 2x growth factor to minimize future resizes
        // Minimum allocation: 64 glyphs worth of space
        constexpr uint32_t MIN_ALLOCATION = 64 * sizeof(WorldGlyphInstanceData);
        uint32_t allocSize = std::max(instanceDataSize * 2, MIN_ALLOCATION);

        // Create buffer with pre-allocated size (no initial data)
        batch.ssbo = std::make_unique<ShaderStorageBuffer>(nullptr, allocSize, GL_DYNAMIC_DRAW);

        // Upload actual glyph data
        batch.ssbo->SetData(batch.glyphInstances.data(), instanceDataSize, 0);
    } else {
        // Resize buffer if new data exceeds capacity (with 2x growth factor)
        if (instanceDataSize > batch.ssbo->GetSize()) {
            uint32_t newSize = instanceDataSize * 2; // Pre-allocate 2x for future growth
            batch.ssbo->Resize(newSize);
        }
        // Update existing SSBO (only write actual data, not full buffer)
        batch.ssbo->SetData(batch.glyphInstances.data(), instanceDataSize, 0);
    }

    batch.dirty = false;
}

void TextRenderer::RenderBatch(RenderPass& renderPass, const FontBatch& batch, const FrameData& /*frameData*/)
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

void TextRenderer::RenderWorldTextToPass(RenderPass& renderPass, const FrameData& frameData)
{
    /*if (m_WorldFontBatches.empty()) {
        spdlog::warn("TextRenderer::RenderWorldTextToPass() - No world font batches to render!");
        return;
    }*/

    if (!m_WorldTextShader) {
        spdlog::warn("TextRenderer::RenderWorldTextToPass() - No world text shader set!");
        return;
    }

    // spdlog::info("TextRenderer::RenderWorldTextToPass() - Rendering {} world batches", m_WorldFontBatches.size());

    // ========== SHARED STATE SETUP (ONCE FOR ALL BATCHES) ==========
    // Bind shader once for all batches
    RenderCommands::BindShaderData bindShaderCmd{ m_WorldTextShader };
    renderPass.Submit(bindShaderCmd);

    // Set camera matrices (for MVP transformation)
    RenderCommands::SetUniformMat4Data viewCmd{
        m_WorldTextShader,
        "u_View",
        frameData.viewMatrix
    };
    renderPass.Submit(viewCmd);

    RenderCommands::SetUniformMat4Data projCmd{
        m_WorldTextShader,
        "u_Projection",
        frameData.projectionMatrix
    };
    renderPass.Submit(projCmd);

    // Enable depth testing for world-space text
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // ========== PER-BATCH RENDERING ==========
    for (const auto& [atlasID, batch] : m_WorldFontBatches) {
        if (!batch.glyphInstances.empty()) {
            RenderWorldBatch(renderPass, batch, frameData);
        }
    }

    // Restore default state (disable depth test if needed)
    // Note: This should be handled by the render pipeline state management
}

void TextRenderer::RenderWorldBatch(RenderPass& renderPass, const WorldFontBatch& batch, const FrameData& /*frameData*/)
{
    if (batch.glyphInstances.empty() || !batch.ssbo) {
        spdlog::warn("TextRenderer::RenderWorldBatch() - Skipping: glyphs={}, ssbo={}",
            batch.glyphInstances.size(), batch.ssbo ? "valid" : "null");
        return;
    }

    // Debug: Check VAO validity and first glyph data
    if (m_QuadVAO == 0) {
        spdlog::error("TextRenderer::RenderWorldBatch() - CRITICAL: m_QuadVAO is 0!");
        return;
    }

    // Log first glyph details for debugging
    //const auto& firstGlyph = batch.glyphInstances[0];
    /*spdlog::info("TextRenderer::RenderWorldBatch() - VAO={}, SSBO={}, atlas={}, glyphs={}",
        m_QuadVAO, batch.ssbo->GetSSBOHandle(), batch.atlasTextureID, batch.glyphInstances.size());
    spdlog::info("  First glyph: pos=({:.2f},{:.2f},{:.2f}), size=({:.4f},{:.4f}), color=({:.2f},{:.2f},{:.2f},{:.2f})",
        firstGlyph.worldPosition.x, firstGlyph.worldPosition.y, firstGlyph.worldPosition.z,
        firstGlyph.size.x, firstGlyph.size.y,
        firstGlyph.color.r, firstGlyph.color.g, firstGlyph.color.b, firstGlyph.color.a);
    spdlog::info("  Billboard: right=({:.2f},{:.2f},{:.2f}), up=({:.2f},{:.2f},{:.2f})",
        firstGlyph.billboardRight.x, firstGlyph.billboardRight.y, firstGlyph.billboardRight.z,
        firstGlyph.billboardUp.x, firstGlyph.billboardUp.y, firstGlyph.billboardUp.z);*/

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
        m_WorldTextShader,
        "u_SDFAtlas"
    };
    renderPass.Submit(bindTextureCmd);

    // Set multi-channel SDF flag
    RenderCommands::SetUniformBoolData multiChannelCmd{
        m_WorldTextShader,
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

glm::vec2 TextRenderer::CalculateTextBounds(const TextElementData& textElement)
{
    const FontAtlas* fontAtlas = textElement.fontAtlas;
    if (!fontAtlas || textElement.text.empty()) return glm::vec2(0.0f);

    float fontScale = textElement.fontSize / static_cast<float>(fontAtlas->GetBaseFontSize());
    float lineHeight = fontAtlas->GetLineHeight() * fontScale * textElement.lineSpacing;

    float maxWidth = 0.0f;
    float currentLineWidth = 0.0f;
    int lineCount = 1;
    char32_t prevCodepoint = 0;

    const char* str = textElement.text.c_str();
    while (*str) {
        char32_t codepoint = DecodeUTF8(str);
        if (codepoint == 0) break;

        if (codepoint == '\n') {
            maxWidth = std::max(maxWidth, currentLineWidth);
            currentLineWidth = 0.0f;
            lineCount++;
            prevCodepoint = 0;
            continue;
        }

        const GlyphData* glyph = fontAtlas->GetGlyph(codepoint);
        if (!glyph) {
            glyph = fontAtlas->GetGlyph(' ');
            if (!glyph) {
                prevCodepoint = codepoint;
                continue;
            }
        }

        if (prevCodepoint != 0) {
            currentLineWidth += fontAtlas->GetKerning(prevCodepoint, codepoint) * fontScale;
        }

        currentLineWidth += (glyph->advance + textElement.letterSpacing) * fontScale;
        prevCodepoint = codepoint;
    }

    maxWidth = std::max(maxWidth, currentLineWidth);
    float totalHeight = lineHeight * lineCount;

    return glm::vec2(maxWidth, totalHeight);
}

glm::vec2 TextRenderer::CalculateAnchorOffset(TextAnchor anchor, const glm::vec2& bounds)
{
    glm::vec2 offset(0.0f);

    // Horizontal offset - anchor describes which edge of text is at position
    // Left: position is left edge of text (text extends right)
    // Right: position is right edge of text (text extends left)
    switch (anchor) {
        case TextAnchor::TopRight:
        case TextAnchor::CenterRight:
        case TextAnchor::BottomRight:
            offset.x = 0.0f;  // Position is right edge, text extends left
            break;
        case TextAnchor::TopCenter:
        case TextAnchor::Center:
        case TextAnchor::BottomCenter:
            offset.x = -bounds.x * 0.5f;  // Position is horizontal center
            break;
        case TextAnchor::TopLeft:
        case TextAnchor::CenterLeft:
        case TextAnchor::BottomLeft:
            offset.x = -bounds.x;  // Position is left edge, text extends right
            break;
    }

    // Vertical offset - anchor describes which edge of text is at position
    // Top: position is top edge of text (text extends down)
    // Bottom: position is bottom edge of text (text extends up)
    switch (anchor) {
        case TextAnchor::BottomLeft:
        case TextAnchor::BottomCenter:
        case TextAnchor::BottomRight:
            offset.y = 0.0f;  // Position is bottom edge, text extends up
            break;
        case TextAnchor::CenterLeft:
        case TextAnchor::Center:
        case TextAnchor::CenterRight:
            offset.y = -bounds.y * 0.5f;  // Position is vertical center
            break;
        case TextAnchor::TopLeft:
        case TextAnchor::TopCenter:
        case TextAnchor::TopRight:
            offset.y = -bounds.y;  // Position is top edge, text extends down
            break;
    }

    return offset;
}

void TextRenderer::LayoutText(const TextElementData& textElement, FontBatch& batch)
{
    const FontAtlas* fontAtlas = textElement.fontAtlas;
    if (!fontAtlas) return;

    // Calculate font scale from base size to desired size
    float fontScale = textElement.fontSize / static_cast<float>(fontAtlas->GetBaseFontSize());

    // Calculate text bounds and anchor offset
    glm::vec2 textBounds = CalculateTextBounds(textElement);
    glm::vec2 anchorOffset = CalculateAnchorOffset(textElement.anchor, textBounds);

    // Starting cursor position (adjusted for anchor)
    glm::vec2 cursor = textElement.position + anchorOffset;

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

        // Convert outline/glow from pixels to SDF space
        // At base font size, 1 pixel = 0.5 / sdfRange in SDF units
        // Divide by fontScale to maintain consistent screen pixel width at any size
        float sdfRange = static_cast<float>(fontAtlas->GetSDFRange());
        float pixelsToSDF = 0.5f / (sdfRange * fontScale);
        glyphInstance.outlineWidth = textElement.outlineWidth * pixelsToSDF;
        glyphInstance.glowStrength = textElement.glowStrength * pixelsToSDF;
        glyphInstance.outlineColor = textElement.outlineColor;
        glyphInstance.glowColor = textElement.glowColor;
        glyphInstance.rotation = glm::radians(textElement.rotation);
        // Individual glyphs always use TopLeft anchor - the bearing already positions
        // the top-left of each glyph relative to the baseline. Text block anchoring
        // is handled via cursor position adjustment in CalculateAnchorOffset().
        glyphInstance.anchor = static_cast<uint32_t>(TextAnchor::TopLeft);
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

void TextRenderer::LayoutWorldText(const WorldTextElementData& worldText, WorldFontBatch& batch)
{
    const FontAtlas* fontAtlas = worldText.fontAtlas;
    if (!fontAtlas) return;

    // Calculate distance from camera to text
    float distanceToCamera = glm::length(worldText.worldPosition - worldText.cameraPosition);

    // Unity-style font scaling: actualSize = (fontSize * referenceDistance) / distanceToCamera
    // This makes text appear at the specified pixel size when at reference distance
    float distanceScale = worldText.referenceDistance / std::max(distanceToCamera, 0.01f);

    // Unity-style "Pixels Per Unit" - 100 pixels = 1 world unit (Unity's default)
    // fontSize 100 at reference distance = ~1 world unit height
    const float PIXELS_PER_WORLD_UNIT = 100.0f;
    float pixelsToWorldUnits = distanceScale / PIXELS_PER_WORLD_UNIT;
    float fontScale = worldText.fontSize / static_cast<float>(fontAtlas->GetBaseFontSize());
    //float worldFontScale = fontScale * pixelsToWorldUnits;

    // Calculate billboard basis vectors based on billboard mode
    glm::vec3 billboardRight, billboardUp;

    switch (worldText.billboardMode) {
        case TextBillboardMode::Full: {
            // Full billboard - text always faces camera
            glm::vec3 toCamera = glm::normalize(worldText.cameraPosition - worldText.worldPosition);
            billboardRight = glm::normalize(glm::cross(worldText.cameraUp, toCamera));
            billboardUp = -glm::normalize(glm::cross(billboardRight, toCamera));
            break;
        }

        case TextBillboardMode::Cylindrical: {
            // Cylindrical billboard - rotate around Y-axis only (stays upright)
            glm::vec3 toCamera = worldText.cameraPosition - worldText.worldPosition;
            toCamera.y = 0.0f;  // Project onto XZ plane
            if (glm::length(toCamera) < 0.001f) {
                // Camera directly above/below - use camera forward
                toCamera = worldText.cameraForward;
                toCamera.y = 0.0f;
            }
            toCamera = glm::normalize(toCamera);
            billboardRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCamera));
            billboardUp = glm::vec3(0, 1, 0);  // Always up
            break;
        }

        case TextBillboardMode::None:
        default: {
            // No billboard - use custom rotation from transform
            billboardRight = glm::vec3(worldText.customRotation[0]);
            billboardUp = glm::vec3(worldText.customRotation[1]);
            break;
        }
    }

    // Calculate text bounds for alignment (in pixels, then convert to world units)
    // For world text, we'll calculate bounds in pixel space then convert
    float totalWidth = 0.0f;
    float lineHeight = fontAtlas->GetLineHeight() * fontScale;

    // Simple width calculation (single line for now)
    const char* measureStr = worldText.text.c_str();
    char32_t prevCodepoint = 0;
    while (*measureStr) {
        char32_t codepoint = DecodeUTF8(measureStr);
        if (codepoint == 0) break;
        if (codepoint == '\n') break;  // Only measure first line for simplicity

        const GlyphData* glyph = fontAtlas->GetGlyph(codepoint);
        if (glyph) {
            if (prevCodepoint != 0) {
                totalWidth += fontAtlas->GetKerning(prevCodepoint, codepoint) * fontScale;
            }
            totalWidth += (glyph->advance + worldText.letterSpacing) * fontScale;
            prevCodepoint = codepoint;
        }
    }

    // Calculate alignment offset in pixel space, then convert to world
    float alignmentOffsetX = 0.0f;
    switch (worldText.alignment) {
        case TextAlignment::Center:
            alignmentOffsetX = -totalWidth * 0.5f;
            break;
        case TextAlignment::Right:
            alignmentOffsetX = -totalWidth;
            break;
        case TextAlignment::Left:
        default:
            alignmentOffsetX = 0.0f;
            break;
    }

    // Starting cursor position (in pixel space, will be converted to world)
    // Cursor Y=0 represents the baseline. Glyph bearing.Y (pre-negated, Y-down) positions each glyph
    // relative to this baseline. Negative bearing.Y = above baseline, positive = below baseline.
    glm::vec2 cursor(alignmentOffsetX, 0.0f);

    // Decode UTF-8 and layout glyphs
    const char* str = worldText.text.c_str();
    prevCodepoint = 0;

    while (*str) {
        // Decode UTF-8 codepoint
        char32_t codepoint = DecodeUTF8(str);
        if (codepoint == 0) break;

        // Handle newline
        if (codepoint == '\n') {
            cursor.x = alignmentOffsetX;
            cursor.y += lineHeight * worldText.lineSpacing;
            prevCodepoint = 0;
            continue;
        }

        // Get glyph data
        const GlyphData* glyph = fontAtlas->GetGlyph(codepoint);
        if (!glyph) {
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

        // Calculate glyph position in pixel space
        glm::vec2 glyphPixelPos = cursor + glyph->bearing * fontScale;
        glm::vec2 glyphPixelSize = glyph->size * fontScale;

        // Convert to world-space offset from text origin
        // Note: Negate Y because text layout uses Y-down but billboardUp points up in world space
        glm::vec3 worldOffset = billboardRight * (glyphPixelPos.x * pixelsToWorldUnits)
                              + billboardUp * (-glyphPixelPos.y * pixelsToWorldUnits);

        // Create world glyph instance
        WorldGlyphInstanceData glyphInstance;
        glyphInstance.worldPosition = worldText.worldPosition + worldOffset;
        glyphInstance.billboardRight = billboardRight;
        glyphInstance.billboardUp = billboardUp;
        glyphInstance.size = glyphPixelSize * pixelsToWorldUnits;
        glyphInstance.uvRect = glm::vec4(glyph->uv_min, glyph->uv_max);
        glyphInstance.color = worldText.color;
        glyphInstance.sdfThreshold = worldText.sdfThreshold;
        glyphInstance.smoothing = worldText.smoothing;

        // Convert outline/glow (same as screen-space)
        float sdfRange = static_cast<float>(fontAtlas->GetSDFRange());
        float pixelsToSDF = 0.5f / (sdfRange * fontScale);
        glyphInstance.outlineWidth = worldText.outlineWidth * pixelsToSDF;
        glyphInstance.glowStrength = worldText.glowStrength * pixelsToSDF;
        glyphInstance.outlineColor = worldText.outlineColor;
        glyphInstance.glowColor = worldText.glowColor;

        // Padding (for alignment)
        glyphInstance._padding0 = 0.0f;
        glyphInstance._padding1 = 0.0f;
        glyphInstance._padding2 = 0.0f;
        glyphInstance._padding3 = glm::vec2(0.0f);

        batch.glyphInstances.push_back(glyphInstance);
        m_TotalGlyphs++;

        // Advance cursor
        cursor.x += (glyph->advance + worldText.letterSpacing) * fontScale;
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
                                                  float /*fontSize*/, float maxWidth)
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
