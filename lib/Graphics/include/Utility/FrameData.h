/******************************************************************************/
/*!
\file   FrameData.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the FrameData struct for shared frame information across rendering systems

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Buffer/FrameBuffer.h"
#include "AABB.h"
#include "ShadowData.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

/**
 * DebugLine - Debug line primitive for physics visualization
 *
 * Represents a colored line segment for debug rendering (physics colliders,
 * velocities, contact normals, etc.)
 */
struct DebugLine
{
    glm::vec3 from;     // Start point in world space
    glm::vec3 to;       // End point in world space
    glm::vec3 color;    // RGB color (0.0-1.0 range)
    float width;        // Line width (for future use, currently defaults to 1.0)

    // Constructor with default width
    DebugLine(const glm::vec3& startPos, const glm::vec3& endPos, const glm::vec3& lineColor, float lineWidth = 1.0f)
        : from(startPos), to(endPos), color(lineColor), width(lineWidth) {}
};

/**
 * FrameData - Shared frame information across all rendering systems
 *
 * Contains all data that needs to be shared between pipelines and passes
 * during a single frame of rendering.
 */
struct FrameData
{
    // UNIFIED SHADOW DATA (SSBO-based, supports UNLIMITED lights!)
    // This vector stores shadow metadata for all active shadows (directional, point, spot)
    std::vector<ShadowData> shadowDataArray;

    // Shadow texture IDs
    // 2D texture array (directional/spot shadows use textureIndex to index into array layers)
    uint32_t shadow2DTextureArray = 0;            // Single layered texture array for all 2D shadows
    // Cubemap textures (point shadows - still individual textures for now)
    std::vector<uint32_t> shadowCubemapTextures;  // Cubemap depth maps (point)

    // Main rendering output (includes debug overlay when enabled)
    std::shared_ptr<FrameBuffer> mainColorBuffer;

    // HDR resolved buffer (resolved from MSAA mainColorBuffer for tone mapping)
    std::shared_ptr<FrameBuffer> hdrResolvedBuffer;        // Non-MSAA RGB16F for HDR pipeline

    // Bloom texture (output of BloomRenderPass)
    uint32_t bloomTexture = 0;

    // Editor display buffer (resolved from mainColorBuffer for ImGui sampling)
    std::shared_ptr<FrameBuffer> editorResolvedBuffer;     // Non-MSAA resolved for ImGui (Scene viewport)
    std::shared_ptr<FrameBuffer> gameResolvedBuffer;       // Non-MSAA resolved for ImGui (Game viewport)

    // Post-processing chain
    std::shared_ptr<FrameBuffer> postProcessBuffer;

    // Camera data (shared across all pipelines)
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f);

    // Camera context tracking for dual viewport rendering
    enum class CameraContext
    {
        EDITOR,  // Rendering for Scene viewport (editor camera)
        GAME     // Rendering for Game viewport (game camera)
    };
    CameraContext currentCamera = CameraContext::EDITOR;

    // Editor camera matrices (stored separately for picking system)
    // These are preserved even when rendering with game camera
    glm::mat4 editorViewMatrix = glm::mat4(1.0f);
    glm::mat4 editorProjectionMatrix = glm::mat4(1.0f);
    glm::vec3 editorCameraPosition = glm::vec3(0.0f);

    // Viewport dimensions (for HDR and post-processing)
    uint32_t viewportWidth = 1280;
    uint32_t viewportHeight = 720;

    // Debug rendering data
    std::vector<DebugLine> debugLines;  // Physics debug lines (collision shapes, velocities, contacts)

    // Timing data
    float deltaTime = 0.0f;
    uint32_t frameNumber = 0;
};