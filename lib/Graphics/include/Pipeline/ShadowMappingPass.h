#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <glm/glm.hpp>

// Forward declaration
class Shader;

/**
 * Shadow Mapping Pass - Renders depth maps from light perspective
 *
 * Renders scene geometry to depth-only framebuffer from the perspective of
 * the primary directional light. Populates FrameData::shadowMaps[0] and
 * shadowMatrices[0] for use by the main rendering pass.
 *
 * Uses pass ID 0 to ensure execution before main rendering pass (pass ID 1).
 */
class ShadowMappingPass : public RenderPass {
public:
    ShadowMappingPass();
    ShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader);
    ~ShadowMappingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set shadow depth shader (alternative to constructor injection)
    void SetShadowDepthShader(std::shared_ptr<Shader> shader) { m_ShadowDepthShader = shader; }

private:
    // Helper methods
    glm::mat4 CalculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter);
    glm::mat4 CalculateLightProjectionMatrix(const glm::vec3& lightDirection, const FrameData& frameData);

    // Shader storage
    std::shared_ptr<Shader> m_ShadowDepthShader;

    // Configuration
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
};