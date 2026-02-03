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
    float outlineWidth = 0.0f;                              // Outline thickness in pixels
    glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Outline color

    // Glow/shadow effect
    float glowStrength = 0.0f;                              // Glow strength in pixels
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

/**
 * @brief Billboard modes for world-space text
 */
enum class TextBillboardMode {
    None,         ///< No billboarding - uses provided transform rotation
    Full,         ///< Always faces camera (rotates on all axes)
    Cylindrical   ///< Rotates to face camera but constrained to Y-axis (stays upright)
};

/**
 * @brief Data structure for submitting world-space text elements
 *
 * Represents text positioned in 3D world space (not screen space).
 * Uses camera matrices for transformation and supports billboarding.
 * Font sizes are specified in "pixels at reference distance" (Unity-style).
 */
struct WorldTextElementData {
    // Font atlas to use for rendering (required)
    const FontAtlas* fontAtlas = nullptr;

    // Text content (UTF-8 encoded)
    std::string text;

    // World-space position
    glm::vec3 worldPosition = glm::vec3(0.0f);

    // Billboard mode
    TextBillboardMode billboardMode = TextBillboardMode::Full;

    // Optional custom rotation (used if billboardMode == None)
    glm::mat3 customRotation = glm::mat3(1.0f);  // Identity matrix

    // Font sizing (Unity-style: pixels at reference distance)
    float fontSize = 16.0f;         ///< Font size in pixels at reference distance
    float referenceDistance = 10.0f; ///< Distance at which fontSize is measured (in world units)

    // Camera data (for billboard calculation and distance-based scaling)
    glm::vec3 cameraPosition = glm::vec3(0.0f);
    glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Text alignment (for multi-line text)
    TextAlignment alignment = TextAlignment::Center;

    // Line spacing multiplier (1.0 = default line height)
    float lineSpacing = 1.0f;

    // Letter spacing in world units (scaled by distance)
    float letterSpacing = 0.0f;

    // Maximum width for word wrapping in world units (0 = no wrapping)
    float maxWidth = 0.0f;

    // Text color (RGBA)
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Outline effect
    float outlineWidth = 0.0f;                              // Outline thickness (scaled by distance)
    glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Outline color

    // Glow/shadow effect
    float glowStrength = 0.0f;                              // Glow strength (scaled by distance)
    glm::vec4 glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f); // Glow/shadow color

    // SDF rendering parameters
    float sdfThreshold = 0.5f;                              // SDF cutoff threshold (0.5 = sharp)
    float smoothing = 0.04f;                                // Edge smoothing factor

    // Visibility toggle
    bool visible = true;
};
