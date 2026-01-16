/******************************************************************************/
/*!
\file   TextData.h
\author Claude Code
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Declares data structures for submitting text elements to the graphics library

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <string>
#include <cstdint>

// Forward declarations
class FontAtlas;

/**
 * @brief Text alignment modes
 */
enum class TextAlignment {
    Left,
    Center,
    Right,
    Justified
};

/**
 * @brief Anchor point for text element positioning (reuses HUDAnchor values)
 */
enum class TextAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/**
 * @brief Data structure for submitting text elements
 *
 * Represents a single text string to be rendered using SDF font rendering.
 * Uses pixel coordinates with configurable anchor points.
 */
struct TextElementData {
    // Font atlas to use for rendering (required)
    const FontAtlas* fontAtlas = nullptr;

    // Text content (UTF-8 encoded)
    std::string text;

    // Position in screen space (pixels from top-left corner of viewport)
    glm::vec2 position = glm::vec2(0.0f);

    // Font size in pixels (scales glyphs from base font size)
    float fontSize = 16.0f;

    // Anchor point for positioning
    TextAnchor anchor = TextAnchor::TopLeft;

    // Text alignment (for multi-line text)
    TextAlignment alignment = TextAlignment::Left;

    // Line spacing multiplier (1.0 = default line height)
    float lineSpacing = 1.0f;

    // Letter spacing in pixels (added between characters)
    float letterSpacing = 0.0f;

    // Maximum width for word wrapping (0 = no wrapping)
    float maxWidth = 0.0f;

    // Text color (RGBA)
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Outline effect
    float outlineWidth = 0.0f;                              // Outline thickness (0.0-0.5)
    glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Outline color

    // Glow/shadow effect
    float glowStrength = 0.0f;                              // Glow strength (0.0-0.5)
    glm::vec4 glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f); // Glow/shadow color

    // SDF rendering parameters
    float sdfThreshold = 0.5f;                              // SDF cutoff threshold (0.5 = sharp)
    float smoothing = 0.04f;                                // Edge smoothing factor

    // Rotation in degrees (clockwise, around anchor point)
    float rotation = 0.0f;

    // Layer for depth sorting (higher values render on top)
    uint8_t layer = 0;

    // Visibility toggle
    bool visible = true;
};
