#pragma once

#include "../Core/Renderer.h"
#include "../Core/RenderCommandBuffer.h"
#include "../Resources/Shader.h"
#include "../Utility/Camera.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct SubmittedLightData;

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
 */
class PBRLightingRenderer {
public:
    // Light data structures (moved from InstancedRenderer)
    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float constant;
        float linear;
        float quadratic;
    };
    
    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
    };
    
    struct SpotLight {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float cutOff;
        float outerCutOff;
        float constant;
        float linear;
        float quadratic;
    };
    
    // PBR Material properties (simplified since PBRMaterial was removed)
    struct PBRMaterialProperties {
        glm::vec3 albedoColor = glm::vec3(0.8f, 0.7f, 0.6f);
        float metallicValue = 0.7f;
        float roughnessValue = 0.3f;
        bool hasTexture = false;
    };

public:
    PBRLightingRenderer();
    ~PBRLightingRenderer() = default;
    
    // Light management (moved from InstancedRenderer)
    void ClearLights();
    void AddPointLight(const PointLight& light);
    void AddDirectionalLight(const DirectionalLight& light);
    void AddSpotLight(const SpotLight& light);
    
    // PBR lighting setup for any shader
    void SetupPBRLighting(std::shared_ptr<Shader> shader, 
                          const Camera& camera, 
                          const PBRMaterialProperties& material = PBRMaterialProperties{});
    
    // Command-based lighting setup (integrates with render pipeline)
    void SubmitLightingCommands(std::shared_ptr<Shader> shader, 
                                const Camera& camera, 
                                const PBRMaterialProperties& material,
                                const RenderCommands::CommandSortKey& sortKey);
    
    // Static access for other renderers to query lighting data
    static PBRLightingRenderer* GetInstance();
    
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
                       const glm::vec3& ambientLight, Camera& camera);
    
    // Helper method for any renderer to apply lighting to their shader
    void ApplyLightingToShader(std::shared_ptr<Shader> shader, const PBRMaterialProperties& material = PBRMaterialProperties{});

private:
    // Light storage (moved from InstancedRenderer)
    std::vector<PointLight> m_PointLights;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<SpotLight> m_SpotLights;
    
    // Static instance for scene-wide access
    static PBRLightingRenderer* s_Instance;
    
    // Helper methods for setting up specific light types
    void SetupPointLights(std::shared_ptr<Shader> shader);
    void SetupDirectionalLights(std::shared_ptr<Shader> shader);
    void SetupSpotLights(std::shared_ptr<Shader> shader);
    void SetupMaterialProperties(std::shared_ptr<Shader> shader, const PBRMaterialProperties& material);
};