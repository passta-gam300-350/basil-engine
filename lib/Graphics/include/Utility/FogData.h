/******************************************************************************/
/*!
\file   FogData.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/15
\brief  Fog configuration data structures

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>

/**
 * FogType - Enumeration of supported fog modes (based on OGLDev Tutorial 39)
 */
enum class FogType {
    None = 0,              // No fog
    Linear = 1,            // Linear fog (start/end distances)
    Exponential = 2,       // Exponential fog (density-based)
    ExponentialSquared = 3 // Exponential squared fog (smoother density)
};

/**
 * FogData - Configuration for atmospheric fog rendering
 *
 * Based on OGLDev Tutorial 39 fog implementation:
 * - Linear: Fog interpolates linearly between start and end distances
 * - Exponential: Fog density increases exponentially with distance
 * - Exponential Squared: Fog density increases with squared exponential (smoother)
 *
 * Usage:
 *   FogData fog;
 *   fog.type = FogType::Linear;
 *   fog.start = 10.0f;
 *   fog.end = 50.0f;
 *   fog.color = glm::vec3(0.5f, 0.5f, 0.5f); // Gray fog
 */
struct FogData {
    FogType type = FogType::None;           // Fog type (disabled by default)

    // Linear fog parameters
    float start = 10.0f;                    // Start distance (Linear mode only)
    float end = 50.0f;                      // End distance (all modes)

    // Exponential fog parameters
    float density = 0.05f;                  // Fog density (Exponential modes only)

    // Common parameters
    glm::vec3 color = glm::vec3(0.5f);     // Fog color (RGB)

    // Utility methods
    bool IsEnabled() const { return type != FogType::None; }
    void Disable() { type = FogType::None; }
};
