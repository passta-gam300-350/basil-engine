#pragma once

#include "../Buffer/FrameBuffer.h"
#include "AABB.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

/**
 * FrameData - Shared frame information across all rendering systems
 *
 * Contains all data that needs to be shared between pipelines and passes
 * during a single frame of rendering.
 */
struct FrameData
{
    // Directional shadow mapping data
    std::vector<std::shared_ptr<FrameBuffer>> shadowMaps;
    std::vector<glm::mat4> shadowMatrices;

    // Point light shadow mapping data (cubemaps)
    std::vector<uint32_t> pointShadowCubemaps;  // Cubemap texture IDs
    std::vector<float> pointShadowFarPlanes;    // Far plane for each point light

    // Main rendering output (includes debug overlay when enabled)
    std::shared_ptr<FrameBuffer> mainColorBuffer;

    // HDR resolved buffer (resolved from MSAA mainColorBuffer for tone mapping)
    std::shared_ptr<FrameBuffer> hdrResolvedBuffer;        // Non-MSAA RGB16F for HDR pipeline

    // Editor display buffer (resolved from mainColorBuffer for ImGui sampling)
    std::shared_ptr<FrameBuffer> editorResolvedBuffer;     // Non-MSAA resolved for ImGui

    // Post-processing chain
    std::shared_ptr<FrameBuffer> postProcessBuffer;

    // Camera data (shared across all pipelines)
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f);

    // Viewport dimensions (for HDR and post-processing)
    uint32_t viewportWidth = 1280;
    uint32_t viewportHeight = 720;

    // Debug rendering data
    std::vector<DebugAABB> debugAABBs;

    // Timing data
    float deltaTime = 0.0f;
    uint32_t frameNumber = 0;
};