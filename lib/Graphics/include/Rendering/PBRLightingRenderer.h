/******************************************************************************/
/*!
\file   PBRLightingRenderer.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  PBR lighting renderer

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#include "../Resources/Shader.h"
#include "../Resources/OffsetTexture.h"
#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/ShadowData.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct SubmittedLightData;
struct FrameData;
class Material;
class RenderPass;

/**
 * PBR Lighting Renderer - Independent Lighting System
 *
 * Manages all physically-based rendering lighting using submitted data.
 * Completely independent from any specific renderer (InstancedRenderer, MeshRenderer, etc.).
 * Other renderers can query this system for lighting data.
 *
 * This system handles:
 * - Light management from submitted data (Point, Directional, Spot)
 * - PBR material properties
 * - Lighting uniform setup for any shader
 * - Integration with the existing command system
 * - UNLIMITED lights via SSBO (no more 16 light limit!)
 */
class PBRLightingRenderer {
public:
    // Light type enumeration (must match shader)
    enum class LightType : int32_t {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    // Unified light data structure for SSBO (must match shader LightData struct)
    // std430 layout: 80 bytes per light
    struct LightData {
        int32_t type;               // 4 bytes - Light type (0=directional, 1=point, 2=spot)
        float _pad0;                // 4 bytes - Padding for alignment
        float _pad1;                // 4 bytes - Padding for alignment
        float _pad2;                // 4 bytes - Padding for alignment
        glm::vec3 position;         // 12 bytes
        float intensity;            // 4 bytes - Diffuse intensity (ogldev-style)
        glm::vec3 direction;        // 12 bytes
        float ambientIntensity;     // 4 bytes - Per-light ambient contribution
        glm::vec3 color;            // 12 bytes
        float cutOff;               // 4 bytes - Spot inner cutoff (cos angle)
        float outerCutOff;          // 4 bytes - Spot outer cutoff (cos angle)
        float constant;             // 4 bytes - Attenuation constant
        float linear;               // 4 bytes - Attenuation linear
        float quadratic;            // 4 bytes - Attenuation quadratic
        // Total: 80 bytes per light (std430 aligned)
    };

    // Legacy light data structures (kept for backward compatibility, converted to LightData internally)
    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;            // Diffuse intensity (ogldev-style)
        float ambientIntensity;     // Per-light ambient contribution (ogldev-style)
        float constant;
        float linear;
        float quadratic;
    };

    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;            // Diffuse intensity (ogldev-style)
        float ambientIntensity;     // Per-light ambient contribution (ogldev-style)
    };

    struct SpotLight {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;            // Diffuse intensity (ogldev-style)
        float ambientIntensity;     // Per-light ambient contribution (ogldev-style)
        float cutOff;
        float outerCutOff;
        float constant;
        float linear;
        float quadratic;
    };
    

public:
    PBRLightingRenderer();
    ~PBRLightingRenderer() = default;
    
    // Light management (moved from InstancedRenderer)
    void ClearLights();
    void AddPointLight(const PointLight& light);
    void AddDirectionalLight(const DirectionalLight& light);
    void AddSpotLight(const SpotLight& light);

    // Command-based lighting setup using RenderPass
    void SubmitLightingCommands(RenderPass& renderPass,
                                 std::shared_ptr<Shader> shader,
                                 const Material* material = nullptr);
    void SubmitShadowCommands(RenderPass& renderPass,
                              std::shared_ptr<Shader> shader,
                              const FrameData& frameData);

    // Getters for light data (used by other renderers)
    const std::vector<PointLight>& GetPointLights() const { return m_PointLights; }
    const std::vector<DirectionalLight>& GetDirectionalLights() const { return m_DirectionalLights; }
    const std::vector<SpotLight>& GetSpotLights() const { return m_SpotLights; }

    // Light count getters
    size_t GetPointLightCount() const { return m_PointLights.size(); }
    size_t GetDirectionalLightCount() const { return m_DirectionalLights.size(); }
    size_t GetSpotLightCount() const { return m_SpotLights.size(); }

    // Lighting update with submitted data (called once per frame)
    void UpdateLighting(const std::vector<SubmittedLightData>& submittedLights,
                       const glm::vec3& ambientLight, const FrameData& frameData);

    // Shadow intensity configuration
    void SetShadowIntensity(float directionalIntensity, float pointIntensity);
    float GetDirectionalShadowIntensity() const { return m_DirectionalShadowIntensity; }
    float GetPointShadowIntensity() const { return m_PointShadowIntensity; }
    void SetSpotShadowIntensity(float intensity) { m_SpotShadowIntensity = intensity; }
    float GetSpotShadowIntensity() const { return m_SpotShadowIntensity; }

    // Shadow quality configuration
    void SetShadowFilterSize(int filterSize);
    void SetShadowRandomRadius(float radius);
    int GetShadowFilterSize() const { return m_ShadowFilterSize; }
    float GetShadowRandomRadius() const { return m_ShadowRandomRadius; }

    // Reset per-frame caches (forces re-submission of lighting/shadow commands)
    void ResetFrameCache() { m_LastLightingShader.reset(); m_LastShadowShader.reset(); }

private:
    // Light storage (moved from InstancedRenderer)
    std::vector<PointLight> m_PointLights;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<SpotLight> m_SpotLights;

    // Ambient lighting
    glm::vec3 m_AmbientLight = glm::vec3(0.03f);

    // Shadow intensity parameters
    float m_DirectionalShadowIntensity = 0.8f;
    float m_PointShadowIntensity = 0.8f;
    float m_SpotShadowIntensity = 0.8f;

    // Unified light data (SSBO-based, supports UNLIMITED lights!)
    std::unique_ptr<TypedShaderStorageBuffer<LightData>> m_LightSSBO;

    // Unified shadow data (SSBO-based, supports 50+ lights)
    std::unique_ptr<TypedShaderStorageBuffer<ShadowData>> m_ShadowSSBO;

    // Shadow offset texture for random sampling
    std::unique_ptr<OffsetTexture> m_ShadowOffsetTexture;

    // Shadow quality configuration
    // OPTIMIZED FOR TEXTURE ARRAYS: Lower sample count and radius for better cache coherency
    int m_ShadowFilterSize = 4;        // 4x4 = 16 samples (optimized for texture array performance)
    int m_ShadowOffsetTextureSize = 16; // 16x16 screen-space tiling
    float m_ShadowRandomRadius = 5.0f; // Reduced for better texture cache locality with arrays

    // PERFORMANCE: Track which shader has lighting set up to avoid redundant uniform uploads
    std::shared_ptr<Shader> m_LastLightingShader;
    std::shared_ptr<Shader> m_LastShadowShader;

    // Helper methods for setting up specific light types
    void SetupPointLights(std::shared_ptr<Shader> shader);
    void SetupDirectionalLights(std::shared_ptr<Shader> shader);
    void SetupSpotLights(std::shared_ptr<Shader> shader);
    void SetupMaterialProperties(std::shared_ptr<Shader> shader, const Material* material);

    // Shadow setup helpers - removed (now using unified SSBO system in SubmitShadowCommands)
};