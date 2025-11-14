/******************************************************************************/
/*!
\file   MainRenderingPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the MainRenderingPass class for main scene forward rendering with PBR lighting

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include <memory>

// Forward declarations
class Renderer;
class InstancedRenderer;
class PBRLightingRenderer;
class Mesh;
class Shader;

/**
 * Main Rendering Pass - Executes the main forward rendering
 *
 * Handles the main scene rendering with PBR lighting, clearing, and instanced rendering.
 * Contains the render function logic previously in SceneRenderer's main pass.
 */
class MainRenderingPass : public RenderPass
{
public:
    MainRenderingPass();
	MainRenderingPass(const MainRenderingPass&) = delete;
	MainRenderingPass& operator=(const MainRenderingPass&) = delete;
	MainRenderingPass(MainRenderingPass&&) = delete;
	MainRenderingPass& operator=(MainRenderingPass&&) = delete;
    ~MainRenderingPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Skybox configuration (accessed by SceneRenderer)
    void SetSkyboxCubemap(unsigned int cubemapID) { m_SkyboxCubemapID = cubemapID; }
    void SetSkyboxShader(std::shared_ptr<Shader> shader) { m_SkyboxShader = shader; }
    void SetSkyboxMesh(std::shared_ptr<Mesh> mesh) { m_SkyboxMesh = mesh; }
    void EnableSkybox(bool enabled) { m_SkyboxEnabled = enabled; }
    bool IsSkyboxEnabled() const { return m_SkyboxEnabled; }
    void SetSkyboxExposure(float exposure) { m_SkyboxExposure = exposure; }
    void SetSkyboxRotation(const glm::vec3& rotation) { m_SkyboxRotation = rotation; }
    void SetSkyboxTint(const glm::vec3& tint) { m_SkyboxTint = tint; }
    float GetSkyboxExposure() const { return m_SkyboxExposure; }
    glm::vec3 GetSkyboxRotation() const { return m_SkyboxRotation; }
    glm::vec3 GetSkyboxTint() const { return m_SkyboxTint; }

    // Light cube visualization configuration (accessed by SceneRenderer)
    void SetPrimitiveShader(const std::shared_ptr<Shader>& shader) { m_PrimitiveShader = shader; }
    void SetLightCubeMesh(const std::shared_ptr<Mesh>& mesh) { m_LightCube = mesh; }
    void SetShowLightCubes(bool show) { m_ShowLightCubes = show; }
    bool GetShowLightCubes() const { return m_ShowLightCubes; }
    void SetBaseLightCubeSize(float size) { m_BaseLightCubeSize = size; }
    void SetIntensityScaleFactor(float factor) { m_IntensityScaleFactor = factor; }
    void SetCubeSizeRange(float minSize, float maxSize) { m_MinCubeSize = minSize; m_MaxCubeSize = maxSize; }

    // HDR texture access (for tone mapping pipeline)
    uint32_t GetHDRTexture() const {
        return GetFramebuffer() ? GetFramebuffer()->GetColorAttachmentRendererID(0) : 0;
    }

    // Background clear color configuration
    void SetClearColor(const glm::vec4& color) { m_ClearColor = color; }
    glm::vec4 GetClearColor() const { return m_ClearColor; }

private:
    // Render skybox if enabled
    void RenderSkybox(RenderContext& context);

    // Render light cubes for visualization
    void RenderLightCubes(RenderContext& context);

    // Skybox resources
    unsigned int m_SkyboxCubemapID = 0;
    std::shared_ptr<Shader> m_SkyboxShader;
    std::shared_ptr<Mesh> m_SkyboxMesh;
    bool m_SkyboxEnabled = false;
    float m_SkyboxExposure = 1.0f;
    glm::vec3 m_SkyboxRotation = glm::vec3(0.0f);
    glm::vec3 m_SkyboxTint = glm::vec3(1.0f);

    // Light cube visualization resources
    std::shared_ptr<Shader> m_PrimitiveShader;
    std::shared_ptr<Mesh> m_LightCube;
    bool m_ShowLightCubes = true;
    float m_BaseLightCubeSize = 0.3f;
    float m_IntensityScaleFactor = 0.2f;
    float m_MinCubeSize = 0.1f;
    float m_MaxCubeSize = 2.0f;

    // Background clear color (default: light gray)
    glm::vec4 m_ClearColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

};