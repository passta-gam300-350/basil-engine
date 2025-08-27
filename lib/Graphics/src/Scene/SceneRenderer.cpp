#include "Scene/SceneRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/FrustumCuller.h"
#include "Rendering/InstancedRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Buffer/FrameBuffer.h"
#include <glad/gl.h>

SceneRenderer::SceneRenderer() {
    InitializePipeline();
    InitializeRenderingCoordinators();
}

SceneRenderer::~SceneRenderer() {
    // Pipeline and coordinators will be cleaned up by unique_ptr
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

    // Set up render function for geometry pass - now properly coordinates rendering
    geometryPass->SetRenderFunction([this]()
    {
        // Clear the framebuffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Coordinate rendering pipeline properly
        if (m_Scene && m_Camera) {
            // 1. Frustum culling updates VisibilityComponent based on camera
            m_FrustumCuller->CullAgainstCamera(m_Scene.get(), *m_Camera);
            
            // 2. Mesh renderer queries visible entities and generates commands  
            m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
        }
    });

    // Add the geometry pass to the pipeline
    m_Pipeline->AddPass(geometryPass);

    // Add other passes as needed (e.g., lighting, post-processing)
}

void SceneRenderer::InitializeRenderingCoordinators() {
    // Create rendering coordinators - graphics-specific, not ECS systems
    m_MeshRenderer = std::make_unique<MeshRenderer>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
    m_InstancedRenderer = std::make_unique<InstancedRenderer>();
}

void SceneRenderer::Render() {
    if (!m_Scene || !m_Camera) {
        return;
    }

    // Execute the rendering pipeline - which coordinates the rendering systems
    m_Pipeline->Execute();
}