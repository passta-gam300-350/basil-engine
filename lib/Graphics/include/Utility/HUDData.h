/******************************************************************************/
/*!
\file   HUDData.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/06
\brief    Declares data structures for submitting HUD/UI elements to the graphics library

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <cstdint>

// Forward declaration
struct Texture;

/**
 * @brief Anchor point for HUD element positioning
 *
 * Determines which point of the element corresponds to the specified position.
 * For example, TopLeft means position=(0,0) places the top-left corner at screen origin.
 */
enum class HUDAnchor {
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
 * @brief Data structure for submitting HUD/UI elements
 *
 * Represents a single HUD sprite/quad to be rendered in screen space.
 * Uses pixel coordinates with configurable anchor points.
 */
struct HUDElementData {
    // Texture to display (nullptr = colored quad)
    uint32_t textureID = 0;

    // Position in screen space (pixels from top-left corner of viewport)
    glm::vec2 position = glm::vec2(0.0f);

    // Size in pixels
    glm::vec2 size = glm::vec2(100.0f);

    // Anchor point for positioning
    HUDAnchor anchor = HUDAnchor::TopLeft;

    // Tint color (multiplied with texture, or solid color if no texture)
    glm::vec4 color = glm::vec4(1.0f);

    // Rotation in degrees (clockwise, around anchor point)
    float rotation = 0.0f;

    // Layer for depth sorting (higher values render on top)
    uint8_t layer = 0;

    // Visibility toggle
    bool visible = true;
};
