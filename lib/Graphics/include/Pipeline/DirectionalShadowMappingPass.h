/******************************************************************************/
/*!
\file   ShadowMappingPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the ShadowMappingPass class for shadow map rendering from light perspective

Copyright (C) 2025 DigiPen Institute of Technology.
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
 * Directional Shadow Mapping Pass - Renders depth maps from directional light perspective
 *
 * Renders scene geometry to depth-only framebuffer from the perspective of
 * the primary directional light. Populates FrameData::shadowMaps[0] and
 * shadowMatrices[0] for use by the main rendering pass.
 *
 * Uses pass ID 0 to ensure execution before main rendering pass (pass ID 1).
 */
class DirectionalShadowMappingPass : public RenderPass {
public:
    DirectionalShadowMappingPass();
    DirectionalShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader);
    ~DirectionalShadowMappingPass() = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set shadow depth shader (alternative to constructor injection)
    void SetShadowDepthShader(std::shared_ptr<Shader> shader) { m_ShadowDepthShader = shader; }

private:
    // Helper methods
    glm::mat4 CalculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter);
    glm::mat4 CalculateLightProjectionMatrix(const glm::vec3& lightDirection, const FrameData& frameData, float sceneRadius);
    void UpdateSceneBounds(const std::vector<RenderableData>& renderables);

    // Shader storage
    std::shared_ptr<Shader> m_ShadowDepthShader;

    // Cached scene bounds with transform change detection
    glm::vec3 m_CachedSceneMin = glm::vec3(FLT_MAX);
    glm::vec3 m_CachedSceneMax = glm::vec3(-FLT_MAX);
    glm::vec3 m_CachedSceneCenter = glm::vec3(0.0f);
    float m_CachedSceneRadius = 10.0f;
    size_t m_CachedRenderableCount = 0;
    size_t m_CachedTransformHash = 0;  // Hash of all transform positions and scales
    bool m_BoundsDirty = true;

    // Configuration
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
};