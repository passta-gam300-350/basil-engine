/******************************************************************************/
/*!
\file   SceneRenderer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the SceneRenderer class for high-level scene rendering coordination

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Pipeline/RenderPipeline.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include "../utility/Particle.h"
#include "../Utility/HUDData.h"
#include "../Utility/TextData.h"
#include "../Resources/ResourceManager.h"
#include "../Resources/TextureSlotManager.h"
#include <memory>
#include <vector>

// Forward declarations for rendering coordinators
class FrustumCuller;
class InstancedRenderer;
class PBRLightingRenderer;
class PickingRenderPass;
class ParticleRenderer;
class OutlineRenderPass;
class ParticleRenderer;
class HUDRenderer;
class TextRenderer;

class SceneRenderer {
public:
    SceneRenderer();
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer& operator=(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator=(SceneRenderer&&) = delete;
    ~SceneRenderer();
    
    // Data submission API - application pushes data each frame
    void SubmitRenderable(const RenderableData& renderable);
    void SubmitLight(const SubmittedLightData& light);
    void SubmitParticles(const ParticleRenderData& particleData);
    void SubmitHUDElement(const HUDElementData& hudElement);
    void SubmitText(const TextElementData& textElement);
    void SubmitWorldText(const WorldTextElementData& worldText);
    void SetAmbientLight(const glm::vec3& ambient) { m_AmbientLight = ambient; }
    
    // Clear submitted data (call at start of frame)
    void ClearFrame();

    // High-level scene rendering coordination
    void Render();

    // Pipeline access
    RenderPipeline* GetPipeline() const { return m_Pipeline.get(); }

    //Access to frame data for custom pipeline setup
    FrameData &GetFrameData()
    {
        return m_FrameData;
    }
    
    // Access to rendering coordinators for advanced usage
    InstancedRenderer* GetInstancedRenderer() const { return m_InstancedRenderer.get(); }
    PBRLightingRenderer* GetPBRLightingRenderer() const { return m_PBRLightingRenderer.get(); }
    ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }
    TextureSlotManager* GetTextureSlotManager() const { return m_TextureSlotManager.get(); }
    ParticleRenderer* GetParticleRenderer() const { return m_ParticleRenderer.get(); }
    HUDRenderer* GetHUDRenderer() const { return m_HUDRenderer.get(); }
    TextRenderer* GetTextRenderer() const { return m_TextRenderer.get(); }
    // Configuration methods for application layer
    void SetShadowDepthShader(const std::shared_ptr<Shader>& shader) const;
    void SetPointShadowShader(const std::shared_ptr<Shader>& shader) const;
    void SetSpotShadowShader(const std::shared_ptr<Shader>& shader) const;
    void SetDebugPrimitiveShader(const std::shared_ptr<Shader>& shader) const;  // For light cube rendering in MainPass
    void SetDebugLineShader(const std::shared_ptr<Shader>& shader) const;       // For physics debug visualization in DebugPass
    void SetDebugLightCubeMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetPickingShader(const std::shared_ptr<Shader>& shader) const;
    void SetOutlineShader(const std::shared_ptr<Shader>& shader) const;
    void SetParticleShader(const std::shared_ptr<Shader>& shader) const;
    void SetHUDShader(const std::shared_ptr<Shader>& shader) const;
    void SetTextShader(const std::shared_ptr<Shader>& shader) const;
    void SetWorldTextShader(const std::shared_ptr<Shader>& shader) const;

    // Picking functionality
    PickingResult QueryObjectPicking(const MousePickingQuery& query);
    void EnablePicking(bool enable) const;

    // Outline functionality
    void AddOutlinedObject(uint32_t objectID) const;
    void RemoveOutlinedObject(uint32_t objectID) const;
    void ClearOutlinedObjects() const;
    void SetOutlineColor(const glm::vec3& color) const;
    void SetOutlineScale(float scale) const;
    void EnableOutlineRendering(bool enable) const;

    void SetSkyboxCubemap(unsigned int cubemapID);
    void SetSkyboxShader(const std::shared_ptr<Shader> &shader);
    void EnableSkybox(bool enable);
    bool IsSkyboxEnabled() const;
    void SetSkyboxExposure(float exposure);
    void SetSkyboxRotation(const glm::vec3& rotation);
    void SetSkyboxTint(const glm::vec3& tint);
    float GetSkyboxExposure() const;
    glm::vec3 GetSkyboxRotation() const;
    glm::vec3 GetSkyboxTint() const;

    // Background color configuration
    void SetBackgroundColor(const glm::vec4& color);
    glm::vec4 GetBackgroundColor() const;

    // Light cube visualization configuration
    void SetDefaultLightCubeSize(float size);
    float GetDefaultLightCubeSize() const;
    void SetShowLightCubes(bool show);
    bool GetShowLightCubes() const;

    // HDR configuration API
    void SetHDRComputeShader(const std::shared_ptr<Shader>& shader) const;
    void SetToneMappingShader(const std::shared_ptr<Shader>& shader) const;
    //void SetEditorResolveShader(const std::shared_ptr<Shader>& shader) const;

    // Bloom configuration
    void SetBloomStrength(float strength);
    float GetBloomStrength() const;
    void EnableBloom(bool enable);
    bool IsBloomEnabled() const;

    // Tone mapping configuration
    void SetToneMappingMethod(int method);  // 0=None, 1=Reinhard, 2=ACES, 3=Exposure
    int GetToneMappingMethod() const;

    // HDR exposure configuration
    void SetExposureClampRange(float minExposure, float maxExposure);
    void GetExposureClampRange(float& outMin, float& outMax) const;

    // Facade methods for decoupling (avoid exposing internal coordinators/pipeline)
    void ToggleRenderPass(const std::string& passName);
    void SetShadowIntensity(float directional, float point, float spot);
    void ClearInstanceCache();
    void ForceRebuildInstanceCache();  // Force rebuild of instanced renderer cache (for editor component updates)

    // Shadow quality configuration
    void SetShadowFilterSize(int filterSize);
    void SetShadowRandomRadius(float radius);
    int GetShadowFilterSize() const;
    float GetShadowRandomRadius() const;

    // Camera control facade
    void SetCameraData(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos);

    // Debug rendering facade
    void EnablePhysicsDebugVisualization(bool enable);  // Control physics debug line rendering

    // Pass control facade
    bool IsPassEnabled(const std::string& passName) const;
    void EnablePass(const std::string& passName, bool enable);
    void ToggleHDRPipeline(bool enable);

    // Debug info (read-only access for debugging/logging)
    const FrameData& GetFrameDataReadOnly() const { return m_FrameData; }

    // Shadow texture array access (for shadow passes)
    uint32_t GetShadow2DTextureArray() const { return m_Shadow2DTextureArray; }

private:
    void InitializeRenderingCoordinators();

    void InitializeDefaultPipeline();

    // Shadow texture array management
    void CreateShadow2DTextureArray();
    void DestroyShadow2DTextureArray();

    // Frame-submitted data
    std::vector<RenderableData> m_SubmittedRenderables;
    std::vector<SubmittedLightData> m_SubmittedLights;
    glm::vec3 m_AmbientLight = glm::vec3(0.03f);

    // Single render pipeline
    std::unique_ptr<RenderPipeline> m_Pipeline;

    // Shared frame data
    FrameData m_FrameData;

    // Core systems - SceneRenderer owns these
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<TextureSlotManager> m_TextureSlotManager;

    // Rendering coordinators - SceneRenderer owns these
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
    std::unique_ptr<InstancedRenderer> m_InstancedRenderer;
    std::unique_ptr<PBRLightingRenderer> m_PBRLightingRenderer;
    std::unique_ptr<ParticleRenderer> m_ParticleRenderer;
    std::unique_ptr<HUDRenderer> m_HUDRenderer;
    std::unique_ptr<TextRenderer> m_TextRenderer;

    // Unified shadow texture array (directional + spot shadows)
    uint32_t m_Shadow2DTextureArray = 0;
    static constexpr uint32_t SHADOW_ARRAY_LAYERS = 16;  // Support up to 16 2D shadows (1 directional + 15 spot)
    static constexpr uint32_t SHADOW_MAP_SIZE = 1024;     // 1024x1024 per shadow map (optimized for laptop GPUs)
};