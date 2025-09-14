#include "Scene/SceneRenderer.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/LightComponent.h"
#include "Core/RenderCommand.h"
#include "Core/Renderer.h"

SceneRenderer::SceneRenderer() {
    InitializePipeline();
}

SceneRenderer::~SceneRenderer() {
    // Pipeline will be cleaned up by unique_ptr
}

void SceneRenderer::InitializePipeline() {
    // Create a basic rendering pipeline
    m_Pipeline = std::make_unique<RenderPipeline>("MainPipeline");

    // Create geometry pass
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", FBOSpecs{
        1280, 720, // Default size, will be resized later
        {
            { FBOTextureFormat::RGBA8 },     // Color
            { FBOTextureFormat::RGBA8 },     // Normal
            { FBOTextureFormat::RED_INTEGER },// Entity ID
            { FBOTextureFormat::DEPTH24STENCIL8 } // Depth
        }
        });

    // Set up render function for geometry pass
    geometryPass->SetRenderFunction([this]()
    {
        // Clear the framebuffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // The actual rendering will be handled by RenderSystem
        // which submits commands to the RenderQueue
        // This pass just sets up the framebuffer and clears it
    });

    // Add the geometry pass to the pipeline
    m_Pipeline->AddPass(geometryPass);

    // Add other passes as needed (e.g., lighting, post-processing)
}

void SceneRenderer::Render() {
    if (!m_Scene || !m_Camera) {
        return;
    }

    // Execute the rendering pipeline
    m_Pipeline->Execute();
}