/******************************************************************************/
/*!
\file   ShadowData.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Shadow data structures

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
 * ShadowData - GPU-side shadow information (matches GLSL struct)
 *
 * This structure is uploaded to a SSBO and read by shaders.
 * Must match the GLSL struct layout in main_pbr.frag.
 */
struct ShadowData
{
    // Light-space transformation matrix (used for directional/spot shadows)
    // For point shadows, this is unused (point shadows use cubemap sampling)
    glm::mat4 lightSpaceMatrix;     // 64 bytes

    // Shadow type: 0 = none, 1 = directional, 2 = point, 3 = spot
    int32_t shadowType;             // 4 bytes

    // Index into the appropriate texture array
    // - For directional/spot: index into sampler2DArray
    // - For point: index into samplerCubeArray
    int32_t textureIndex;           // 4 bytes

    // Far plane distance (used for point light shadows for normalization)
    float farPlane;                 // 4 bytes

    // Shadow intensity (0.0 = no shadow, 1.0 = full shadow)
    float intensity;                // 4 bytes

    // Total size: 80 bytes (std430 alignment)

    // Shadow type enumeration
    enum Type : int32_t {
        None = 0,
        Directional = 1,
        Point = 2,
        Spot = 3
    };

    // Default constructor
    ShadowData()
        : lightSpaceMatrix(1.0f)
        , shadowType(Type::None)
        , textureIndex(-1)
        , farPlane(100.0f)
        , intensity(0.8f)
    {}
};

// Ensure std430 alignment (80 bytes)
static_assert(sizeof(ShadowData) == 80, "ShadowData must be 80 bytes for std430 layout");
