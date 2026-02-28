/******************************************************************************/
/*!
\file   WorldUIData.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/15
\brief  Data structures for world-space UI element submission

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <cstdint>

/**
 * @brief Billboard mode for world-space UI elements
 */
enum class WorldUIBillboardMode : uint8_t {
    None,         ///< Fixed orientation from transform
    Full,         ///< Always faces camera
    Cylindrical   ///< Y-axis constrained billboard
};

/**
 * @brief Data for submitting a world-space UI element to the renderer
 *
 * Applications fill this struct and submit it each frame via SceneRenderer::SubmitWorldUI().
 * The renderer handles billboard calculation, batching, and instanced drawing.
 */
struct WorldUIElementData {
    // Texture (0 = solid color quad)
    uint32_t textureID = 0;

    // Transform
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec2 size = glm::vec2(1.0f);  ///< World-space size (width, height)

    // Billboard
    WorldUIBillboardMode billboardMode = WorldUIBillboardMode::Full;
    glm::mat3 customRotation = glm::mat3(1.0f);  ///< Used when billboardMode == None

    // Camera data (for billboard calculation)
    glm::vec3 cameraPosition = glm::vec3(0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Visual
    glm::vec4 color = glm::vec4(1.0f);  ///< Tint or solid color
    uint8_t layer = 0;
    bool visible = true;

    // Interaction (for raycast hit detection)
    uint32_t entityID = 0;  ///< ECS entity ID for identifying hits
};
