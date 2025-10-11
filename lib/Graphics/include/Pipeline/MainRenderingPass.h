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

    // HDR texture access (for tone mapping pipeline)
    uint32_t GetHDRTexture() const {
        return GetFramebuffer() ? GetFramebuffer()->GetColorAttachmentRendererID(0) : 0;
    }

private:
    // Update framebuffer to match current window size
    void UpdateFramebufferSize();

    // Render skybox if enabled
    void RenderSkybox(RenderContext& context);

    // Skybox resources
    unsigned int m_SkyboxCubemapID = 0;
    std::shared_ptr<Shader> m_SkyboxShader;
    std::shared_ptr<Mesh> m_SkyboxMesh;
    bool m_SkyboxEnabled = false;

};