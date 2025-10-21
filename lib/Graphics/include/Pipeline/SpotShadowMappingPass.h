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
 * Renders scene geometry to 2D depth framebuffers from each spot light's position.
 * Populates FrameData::spotShadowMaps and spotShadowMatrices for main pass.
 *
 * Key Features:
 * - Perspective projection (FOV = 2 × outer cutoff angle)
 * - Supports multiple spot lights with separate shadow maps
 * - Uses instanced rendering for efficiency
 * - 1024×1024 depth-only framebuffers
 */
class SpotShadowMappingPass : public RenderPass {
public:
    SpotShadowMappingPass();
    SpotShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader);
    ~SpotShadowMappingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set shadow depth shader (instanced version!)
    void SetShadowDepthShader(std::shared_ptr<Shader> shader) { m_ShadowDepthShader = shader; }

private:
    // Helper methods for matrix calculation
    glm::mat4 CalculateSpotLightViewMatrix(const glm::vec3& position, const glm::vec3& direction);
    glm::mat4 CalculateSpotLightProjectionMatrix(float outerCutoffDegrees, float range);

    // Ensure we have enough framebuffers for spot lights
    void EnsureFramebuffers(size_t count);

    // Shader storage
    std::shared_ptr<Shader> m_ShadowDepthShader;

    // Multiple framebuffers for multiple spot lights
    std::vector<std::shared_ptr<FrameBuffer>> m_SpotShadowFramebuffers;

    // Configuration
    static constexpr uint32_t SPOT_SHADOW_MAP_SIZE = 1024;  // 1K resolution (can increase to 2K)
    static constexpr size_t MAX_SPOT_LIGHTS = 4;  // Maximum supported spot lights
};
