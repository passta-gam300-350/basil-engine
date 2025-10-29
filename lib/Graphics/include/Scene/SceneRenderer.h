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
#include "../Resources/ResourceManager.h"
#include "../Resources/TextureSlotManager.h"
#include <memory>
#include <vector>

// Forward declarations for rendering coordinators
class FrustumCuller;
class InstancedRenderer;
class PBRLightingRenderer;
class PickingRenderPass;
class OutlineRenderPass;

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
    void SetAmbientLight(const glm::vec3& ambient) { m_AmbientLight = ambient; }
    
    // Clear submitted data (call at start of frame)
    void ClearFrame();

    // High-level scene rendering coordination
    void Render();

    // Setup resize rendering - registers Render() with the window's resize callback
    // This allows the scene to be rendered during window resize instead of showing a blank screen
    void SetupResizeRendering(class Window* window);

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

    // Configuration methods for application layer
    void SetShadowDepthShader(const std::shared_ptr<Shader>& shader) const;
    void SetPointShadowShader(const std::shared_ptr<Shader>& shader) const;
    void SetSpotShadowShader(const std::shared_ptr<Shader>& shader) const;
    void SetDebugPrimitiveShader(const std::shared_ptr<Shader>& shader) const;
    void SetDebugLightCubeMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetDebugDirectionalRayMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetDebugAABBWireframeMesh(const std::shared_ptr<Mesh>& mesh) const;
    void SetPickingShader(const std::shared_ptr<Shader>& shader) const;
    void SetOutlineShader(const std::shared_ptr<Shader>& shader) const;

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

    // HDR configuration API
    void SetHDRComputeShader(const std::shared_ptr<Shader>& shader) const;
    void SetToneMappingShader(const std::shared_ptr<Shader>& shader) const;

    // Facade methods for decoupling (avoid exposing internal coordinators/pipeline)
    void ToggleRenderPass(const std::string& passName);
    void SetShadowIntensity(float directional, float point, float spot);
    void ClearInstanceCache();

    // Shadow quality configuration
    void SetShadowFilterSize(int filterSize);
    void SetShadowRandomRadius(float radius);
    int GetShadowFilterSize() const;
    float GetShadowRandomRadius() const;

    // Camera control facade
    void SetCameraData(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos);

    // Debug rendering facade
    void SetDebugAABBs(const std::vector<DebugAABB>& aabbs);
    void ToggleAABBVisualization();
    void SetAABBVisualization(bool show);

    // Editor viewport size control (overrides glfwGetFramebufferSize)
    void SetEditorViewportSize(int width, int height);
    void UseWindowFramebufferSize(); // Revert to using GLFW window size

    // Pass control facade
    bool IsPassEnabled(const std::string& passName) const;
    void EnablePass(const std::string& passName, bool enable);
    void ToggleHDRPipeline(bool enable);

    // Debug info (read-only access for debugging/logging)
    const FrameData& GetFrameDataReadOnly() const { return m_FrameData; }

private:
    void InitializeRenderingCoordinators();

    void InitializeDefaultPipeline();

    // Frame-submitted data
    std::vector<RenderableData> m_SubmittedRenderables;
    std::vector<SubmittedLightData> m_SubmittedLights;
    glm::vec3 m_AmbientLight = glm::vec3(0.1f);

    // Editor viewport override (for rendering to ImGui viewport instead of full window)
    bool m_UseEditorViewport = false;
    int m_EditorViewportWidth = 0;
    int m_EditorViewportHeight = 0;

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
    
};