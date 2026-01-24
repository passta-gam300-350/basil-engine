/******************************************************************************/
/*!
\file   TextRenderer.h
\author Claude Code
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Declares the TextRenderer class for rendering SDF text elements

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/TextData.h"
#include "../Resources/Shader.h"
#include "../Resources/FontAtlas.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declarations
class RenderPass;
struct FrameData;

/**
 * @brief SDF Text Renderer using GPU instancing
 *
 * Renders screen-space text elements with SDF font rendering.
 * Batches glyphs by font atlas for efficient GPU instancing.
 * Supports outline, glow, shadow effects, and rich text markup.
 */
class TextRenderer {
public:
    /**
     * @brief Glyph instance data for SSBO (std430 layout)
     *
     * Each glyph becomes one instance in the GPU.
     * Batched by font atlas for efficient rendering.
     */
    struct GlyphInstanceData {
        glm::vec2 position;         // Screen position (pixels)
        glm::vec2 size;             // Glyph size (pixels)
        glm::vec4 uvRect;           // Atlas UV coordinates (min.xy, max.zw)
        glm::vec4 color;            // Text color
        float sdfThreshold;         // SDF cutoff threshold
        float smoothing;            // Edge smoothing
        float outlineWidth;         // Outline thickness
        float glowStrength;         // Glow/shadow strength
        glm::vec4 outlineColor;     // Outline color
        glm::vec4 glowColor;        // Glow/shadow color
        float rotation;             // Rotation (radians)
        uint32_t anchor;            // Anchor point
        float padding[2];           // Alignment padding to 128 bytes
        // Total: 128 bytes (aligned to vec4)
    };

public:
    TextRenderer();
    ~TextRenderer();

    // Element submission API
    void BeginFrame();
    void SubmitText(const TextElementData& textElement);
    void EndFrame();

    // Rendering
    void RenderToPass(RenderPass& renderPass, const FrameData& frameData);

    // Shader configuration
    void SetTextShader(const std::shared_ptr<Shader>& shader) { m_TextShader = shader; }

    // Reference resolution configuration
    void SetReferenceResolution(const glm::vec2& resolution) { m_ReferenceResolution = resolution; }
    glm::vec2 GetReferenceResolution() const { return m_ReferenceResolution; }

    // Stats
    uint32_t GetGlyphCount() const { return m_TotalGlyphs; }
    uint32_t GetBatchCount() const { return static_cast<uint32_t>(m_FontBatches.size()); }

    // SSBO binding point (must match shader)
    static constexpr uint32_t GLYPH_INSTANCE_SSBO_BINDING = 4;

private:
    // Batch storage per font atlas
    struct FontBatch {
        uint32_t atlasTextureID;
        bool isMultiChannelSDF;
        std::vector<GlyphInstanceData> glyphInstances;
        std::unique_ptr<ShaderStorageBuffer> ssbo;
        bool dirty = true;
    };

    // Font batches (key = atlasTextureID)
    std::unordered_map<uint32_t, FontBatch> m_FontBatches;

    // Text shader (SDF rendering)
    std::shared_ptr<Shader> m_TextShader;

    // Quad mesh (shared by all glyphs)
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;

    // Fixed reference resolution for text layout (default: 1920x1080)
    glm::vec2 m_ReferenceResolution = glm::vec2(1920.0f, 1080.0f);

    // Stats
    uint32_t m_TotalGlyphs = 0;

    // Internal methods
    void CreateQuadMesh();
    void UpdateBatchSSBO(FontBatch& batch);
    void RenderBatch(RenderPass& renderPass, const FontBatch& batch, const FrameData& frameData);

    // Text layout engine
    void LayoutText(const TextElementData& textElement, FontBatch& batch);
    char32_t DecodeUTF8(const char*& str);
    std::vector<std::string> WrapText(const std::string& text, const FontAtlas* fontAtlas,
                                      float fontSize, float maxWidth);

    // Text bounds calculation for anchor support
    glm::vec2 CalculateTextBounds(const TextElementData& textElement);
    glm::vec2 CalculateAnchorOffset(TextAnchor anchor, const glm::vec2& bounds);
};
