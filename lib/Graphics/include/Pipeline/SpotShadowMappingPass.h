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
 * Renders scene geometry to 2D depth framebuffer from spot light's position.
 * Populates FrameData::spotShadowMaps and spotShadowMatrices for main pass.
 *
 * Key Features:
 * - Perspective projection (FOV = 2 × outer cutoff angle)
 * - Reuses shadow_depth shader (same as directional shadows)
 * - 1024×1024 depth-only framebuffer
 * - Supports first spot light only (extendable to multiple)
 */
class SpotShadowMappingPass : public RenderPass {
public:
    SpotShadowMappingPass();
    SpotShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader);
    ~SpotShadowMappingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set shadow depth shader (can reuse directional shadow shader!)
    void SetShadowDepthShader(std::shared_ptr<Shader> shader) { m_ShadowDepthShader = shader; }

private:
    // Helper methods for matrix calculation
    glm::mat4 CalculateSpotLightViewMatrix(const glm::vec3& position, const glm::vec3& direction);
    glm::mat4 CalculateSpotLightProjectionMatrix(float outerCutoffDegrees, float range);

    // Shader storage
    std::shared_ptr<Shader> m_ShadowDepthShader;

    // Configuration
    static constexpr uint32_t SPOT_SHADOW_MAP_SIZE = 1024;  // 1K resolution (can increase to 2K)
};
