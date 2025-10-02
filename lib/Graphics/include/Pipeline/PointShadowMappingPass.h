#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <array>
#include <glm/glm.hpp>

class Shader;

class PointShadowMappingPass : public RenderPass
{
public:
    PointShadowMappingPass();
    PointShadowMappingPass(std::shared_ptr<Shader> depthShader);
    ~PointShadowMappingPass() = default;

    void Execute(RenderContext &context) override;

    void SetPointShadowDepthShader(std::shared_ptr<Shader> shader)
    {
        m_PointShadowDepthShader = shader;
    }

    // Configuration
    void SetShadowCubemapSize(uint32_t size)
    {
        m_ShadowCubemapSize = size;
    }
    void SetShadowFarPlane(float farPlane)
    {
        m_ShadowFarPlane = farPlane;
    }
    void SetMaxShadowCastingLights(uint32_t maxLights)
    {
        m_MaxShadowCastingLights = maxLights;
    }

private:
    // Helper: Calculate 6 view matrices for cubemap faces
    std::array<glm::mat4, 6> CalculateCubemapViewMatrices(const glm::vec3 &lightPos);

    // Helper: Calculate perspective projection for point shadows
    glm::mat4 CalculatePointShadowProjection();

    // Helper: Get or create cubemap FBO for a point light
    std::shared_ptr<FrameBuffer> GetOrCreateCubemapFBO(size_t lightIndex);

    // Helper: Render scene to entire cubemap (single pass with geometry shader)
    void RenderToCubemap(RenderContext &context,
        const std::array<glm::mat4, 6> &viewMatrices,
        const glm::mat4 &projectionMatrix,
        const glm::vec3 &lightPos,
        size_t lightIndex);

    std::shared_ptr<Shader> m_PointShadowDepthShader;

    // Cubemap FBOs (persistent across frames, one per shadow-casting point light)
    std::vector<std::shared_ptr<FrameBuffer>> m_CubemapFBOs;

    // Configuration
    uint32_t m_ShadowCubemapSize = 1024;
    float m_ShadowFarPlane = 25.0f;
    uint32_t m_MaxShadowCastingLights = 4;  // Max number of point lights with shadows
};