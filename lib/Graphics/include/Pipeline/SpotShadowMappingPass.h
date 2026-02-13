/******************************************************************************/
/*!
\file   SpotShadowMappingPass.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Spot light shadow mapping pass

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <glm/glm.hpp>

// Forward declaration
class Shader;

/**
 * Spot Shadow Mapping Pass - Renders depth maps from spot light perspective
 *
 * Similar to directional shadow mapping but uses PERSPECTIVE projection.
 * Renders scene geometry to layers 1-N of the shared shadow texture array.
 *
 * Key Features:
 * - Perspective projection (FOV = 2 × outer cutoff angle)
 * - Supports multiple spot lights (uses texture array layers 1-63)
 * - Uses instanced rendering for efficiency
 * - Shares 2048×2048 depth texture array with directional shadows
 */
class SpotShadowMappingPass : public RenderPass {
public:
    SpotShadowMappingPass();
    SpotShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader);
    ~SpotShadowMappingPass() override;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set shadow depth shader (instanced version!)
    void SetShadowDepthShader(std::shared_ptr<Shader> shader) { m_ShadowDepthShader = shader; }

    // Set shadow texture array (called by SceneRenderer during initialization)
    void SetShadowTextureArray(uint32_t textureArray) { m_ShadowTextureArray = textureArray; }

private:
    // Helper methods for matrix calculation
    glm::mat4 CalculateSpotLightViewMatrix(const glm::vec3& position, const glm::vec3& direction);
    glm::mat4 CalculateSpotLightProjectionMatrix(float outerCutoffDegrees, float range);

    // Shader storage
    std::shared_ptr<Shader> m_ShadowDepthShader;

    // Shadow texture array (shared with directional pass)
    uint32_t m_ShadowTextureArray = 0;

    // Temporary FBO for rendering to texture array layers
    uint32_t m_TempFBO = 0;

    // Configuration
    static constexpr uint32_t SPOT_SHADOW_MAP_SIZE = 1024;  // 1K resolution (directional uses 2K)
    static constexpr size_t MAX_SPOT_LIGHTS = 15;  // Max spot lights (layers 1-15, layer 0 reserved for directional)
    static constexpr int FIRST_SPOT_LAYER = 1;  // Spot lights start at layer 1 (layer 0 = directional)
};
