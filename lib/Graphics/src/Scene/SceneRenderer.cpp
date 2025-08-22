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

        // Render all entities with mesh and material components
        if (m_Scene)
        {
            auto view = m_Scene->GetRegistry().view<MeshComponent, MaterialComponent, TransformComponent>();

            view.each([&](auto entity, auto& mesh, auto& material, auto& transform)
            {
                // Skip if no mesh or material
                if (!mesh.mesh || material.Materials.empty())
                {
                    return;
                }

                // Bind the first material
                material.Materials[0]->Bind();

                // Set transform
                material.Materials[0]->SetMat4("u_Model", transform.GetTransform());

                // Set view and projection from camera
                if (m_Camera)
                {
                    material.Materials[0]->SetMat4("u_View", m_Camera->GetViewMatrix());
                    material.Materials[0]->SetMat4("u_Projection", m_Camera->GetProjectionMatrix());
                }

                // Draw the mesh
                mesh.mesh->Bind();
                glDrawElements(GL_TRIANGLES, mesh.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
            });
        }
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