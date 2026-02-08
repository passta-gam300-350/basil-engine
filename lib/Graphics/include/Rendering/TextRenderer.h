/******************************************************************************/
/*!
\file   TextRenderer.h
\author Team PASSTA
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

    /**
     * @brief World-space glyph instance data for SSBO (std430 layout)
     *
     * Extended version for 3D world-space text with billboard support.
     * Each glyph has its own 3D position and billboard basis vectors.
     */
    struct WorldGlyphInstanceData {
        glm::vec3 worldPosition;    // 3D world position (12 bytes)
        float _padding0;            // Alignment to vec4 (4 bytes)
        glm::vec3 billboardRight;   // Billboard right vector (12 bytes)
        float _padding1;            // Alignment to vec4 (4 bytes)
        glm::vec3 billboardUp;      // Billboard up vector (12 bytes)
        float _padding2;            // Alignment to vec4 (4 bytes)
        glm::vec2 size;             // Glyph size in world units (8 bytes)
        glm::vec2 _padding3;        // Alignment to vec4 (8 bytes)
        glm::vec4 uvRect;           // Atlas UV coordinates (min.xy, max.zw) (16 bytes)
        glm::vec4 color;            // Text color (16 bytes)
        float sdfThreshold;         // SDF cutoff threshold (4 bytes)
        float smoothing;            // Edge smoothing (4 bytes)
        float outlineWidth;         // Outline thickness (4 bytes)
        float glowStrength;         // Glow/shadow strength (4 bytes)
        glm::vec4 outlineColor;     // Outline color (16 bytes)
        glm::vec4 glowColor;        // Glow/shadow color (16 bytes)
        // Total: 144 bytes (aligned to vec4)
    };

public:
    TextRenderer();
    ~TextRenderer();

    // Element submission API (screen-space text)
    void BeginFrame();
    void SubmitText(const TextElementData& textElement);
    void EndFrame();

    // World-space text submission API
    void SubmitWorldText(const WorldTextElementData& worldText);

    // Rendering
    void RenderToPass(RenderPass& renderPass, const FrameData& frameData);
    void RenderWorldTextToPass(RenderPass& renderPass, const FrameData& frameData);

    // Shader configuration
    void SetTextShader(const std::shared_ptr<Shader>& shader) { m_TextShader = shader; }
    void SetWorldTextShader(const std::shared_ptr<Shader>& shader) { m_WorldTextShader = shader; }

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

    // World-space batch storage per font atlas
    struct WorldFontBatch {
        uint32_t atlasTextureID;
        bool isMultiChannelSDF;
        std::vector<WorldGlyphInstanceData> glyphInstances;
        std::unique_ptr<ShaderStorageBuffer> ssbo;
        bool dirty = true;
    };

    // World-space font batches (key = atlasTextureID)
    std::unordered_map<uint32_t, WorldFontBatch> m_WorldFontBatches;

    // Text shaders (SDF rendering)
    std::shared_ptr<Shader> m_TextShader;         // Screen-space text shader
    std::shared_ptr<Shader> m_WorldTextShader;    // World-space text shader

    // Quad mesh (shared by all glyphs)
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;

    // Fixed reference resolution for text layout (default: 1920x1080)
    glm::vec2 m_ReferenceResolution = glm::vec2(1920.0f, 1080.0f);

    // Stats
    uint32_t m_TotalGlyphs = 0;

    // Internal methods (screen-space text)
    void CreateQuadMesh();
    void UpdateBatchSSBO(FontBatch& batch);
    void RenderBatch(RenderPass& renderPass, const FontBatch& batch, const FrameData& frameData);

    // Internal methods (world-space text)
    void UpdateWorldBatchSSBO(WorldFontBatch& batch);
    void RenderWorldBatch(RenderPass& renderPass, const WorldFontBatch& batch, const FrameData& frameData);

    // Text layout engine (screen-space)
    void LayoutText(const TextElementData& textElement, FontBatch& batch);

    // World text layout engine
    void LayoutWorldText(const WorldTextElementData& worldText, WorldFontBatch& batch);
    char32_t DecodeUTF8(const char*& str);
    std::vector<std::string> WrapText(const std::string& text, const FontAtlas* fontAtlas,
                                      float fontSize, float maxWidth);

    // Text bounds calculation for anchor support
    glm::vec2 CalculateTextBounds(const TextElementData& textElement);
    glm::vec2 CalculateAnchorOffset(TextAnchor anchor, const glm::vec2& bounds);
};
