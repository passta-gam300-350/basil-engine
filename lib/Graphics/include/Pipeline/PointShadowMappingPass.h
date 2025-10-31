#pragma once

#include "RenderPass.h"
#include "../Buffer/CubemapFrameBuffer.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declaration
class Shader;

/**
 * Point Shadow Mapping Pass - Renders cubemap depth maps for point lights
 *
 * Uses geometry shader to render to all 6 cubemap faces in a single pass.
 * Each point light gets its own shadow cubemap for omnidirectional shadows.
 *
 * Geometry shader emits geometry to gl_Layer 0-5 (6 cubemap faces).
 *
 * FOLLOWS FRAMEWORK CONVENTION:
 * - Cubemap FBOs created in constructor (up to MAX_POINT_LIGHTS)
 * - Only active FBOs are used during Execute()
 * - Uniform setup via direct shader calls (like PBRLightingRenderer)
 * - Draw commands submitted via Submit() and executed via ExecuteCommands()
 */
class PointShadowMappingPass : public RenderPass {
public:
    PointShadowMappingPass();
    PointShadowMappingPass(std::shared_ptr<Shader> pointShadowShader);
    PointShadowMappingPass(std::shared_ptr<Shader> pointShadowShader, uint32_t shadowCubemapSize, uint32_t maxPointLights);
    ~PointShadowMappingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set point shadow shader (alternative to constructor injection)
    void SetPointShadowShader(std::shared_ptr<Shader> shader) { m_PointShadowShader = shader; }

    // Configuration (can only be set before first Execute)
    void SetShadowCubemapSize(uint32_t size);
    void SetMaxPointLights(uint32_t maxLights);

private:
    // Helper methods
    std::vector<glm::mat4> CalculateShadowMatrices(const glm::vec3& lightPos, float farPlane);
    void RenderPointShadowCubemap(RenderContext& context,
                                  const SubmittedLightData& light,
                                  size_t lightIndex);

    // Initialization
    void InitializeCubemapFBOs();

    // Shader storage
    std::shared_ptr<Shader> m_PointShadowShader;

    // Configuration
    static constexpr uint32_t DEFAULT_SHADOW_CUBEMAP_SIZE = 1024;
    static constexpr uint32_t DEFAULT_MAX_POINT_LIGHTS = 4;
    static constexpr float DEFAULT_SHADOW_FAR_PLANE = 25.0f;

    uint32_t m_ShadowCubemapSize = DEFAULT_SHADOW_CUBEMAP_SIZE;
    uint32_t m_MaxPointLights = DEFAULT_MAX_POINT_LIGHTS;
    float m_ShadowFarPlane = DEFAULT_SHADOW_FAR_PLANE;

    // Cubemap framebuffers (created in constructor, one per max point light)
    std::vector<std::shared_ptr<CubemapFrameBuffer>> m_ShadowCubemaps;
    bool m_CubemapsInitialized = false;
};
