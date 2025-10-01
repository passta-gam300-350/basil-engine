#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Scene/SceneRenderer.h"
#include "../../include/Resources/PrimitiveGenerator.h"
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

MainRenderingPass::MainRenderingPass()
    : RenderPass("MainPass", FBOSpecs
	{
        1280, 720,
{
            { FBOTextureFormat::RGBA8 },
            { FBOTextureFormat::DEPTH24STENCIL8 }
		}
    })
{
    // Create default skybox cube mesh
    m_SkyboxMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
}

void MainRenderingPass::Execute(RenderContext& context)
{
    // Update framebuffer size to match current window before rendering
    UpdateFramebufferSize();

    // New context-based execution - use references instead of copies!
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Clear color and depth buffers using command buffer
    RenderCommands::ClearData clearCmd{
        0.7f, 0.7f, 0.7f, 1.0f,  // r, g, b, a
        true,                      // clearColor
        true                       // clearDepth
    };

    Submit(clearCmd);

    // Render skybox first (after clear) if enabled
    RenderSkybox(context);

    // Re-enable face culling for normal scene objects
    RenderCommands::SetFaceCullingData cullingReenableCmd{
        true,     // enable face culling
        GL_BACK   // cull back faces
    };
    Submit(cullingReenableCmd);

    // Ensure proper depth testing state for opaque objects
    RenderCommands::SetDepthTestData depthTestCmd{
        true,           // enable depth testing
        GL_LESS,        // depth function (objects closer to camera pass)
        true            // enable depth writing
    };
    Submit(depthTestCmd);

    // Standard forward rendering with context data (no copies!)
    if (!context.renderables.empty())
    {
        // 1. Update scene-wide lighting with submitted lights
        context.pbrLighting.UpdateLighting(context.lights, context.ambientLight, context.frameData);

        // 2. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(context.renderables, context.frameData);

        // 3. Forward instanced rendering with visible renderables using pass-local buffer
        context.instancedRenderer.RenderToPass(*this, context.renderables, context.frameData);
    }

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Store main color buffer in frame data (direct update via reference!)
    context.frameData.mainColorBuffer = GetFramebuffer();

    // Create separate editor FBO copy to avoid conflicts with PresentPass
    CreateEditorFBOCopy(context);

    End();
}

void MainRenderingPass::UpdateFramebufferSize()
{
    // Get current window size
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (!currentWindow) return;

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(currentWindow, &windowWidth, &windowHeight);

    // Check if we need to resize the framebuffer
    auto currentSpecs = m_Framebuffer->GetSpecification();
    if (currentSpecs.Width != static_cast<uint32_t>(windowWidth) ||
        currentSpecs.Height != static_cast<uint32_t>(windowHeight)) {

        // Resize the framebuffer to match window size
        m_Framebuffer->Resize(static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight));

        // Update the viewport to match the new framebuffer size
        SetViewport(Viewport(0, 0, static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight)));
    }
}

void MainRenderingPass::CreateEditorFBOCopy(RenderContext &context)
{
    // Create editor FBO if it doesn't exist or size changed
    auto mainFBO = GetFramebuffer();
    const auto &mainSpec = mainFBO->GetSpecification();

    if (!context.frameData.editorColorBuffer ||
        context.frameData.editorColorBuffer->GetSpecification().Width != mainSpec.Width ||
        context.frameData.editorColorBuffer->GetSpecification().Height != mainSpec.Height)
    {
        // Create identical FBO specs for editor copy
        FBOSpecs editorSpec = mainSpec;
        context.frameData.editorColorBuffer = std::make_shared<FrameBuffer>(editorSpec);
    }

    // Blit main FBO to editor FBO
    auto editorFBO = context.frameData.editorColorBuffer;

    // Use OpenGL blit directly since we're not going through command buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO->GetFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, editorFBO->GetFBOHandle());

    glBlitFramebuffer(
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // Restore framebuffer binding
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void MainRenderingPass::RenderSkybox(RenderContext& context)
{
    if (!m_SkyboxEnabled || !m_SkyboxShader || !m_SkyboxMesh || m_SkyboxCubemapID == 0)
    {
        spdlog::warn("Skybox not rendering - Enabled: {}, Shader: {}, Mesh: {}, CubemapID: {}",
                     m_SkyboxEnabled, m_SkyboxShader != nullptr, m_SkyboxMesh != nullptr, m_SkyboxCubemapID);
        return; // Skip if not properly configured
    }

    spdlog::info("Rendering skybox with cubemap ID: {}", m_SkyboxCubemapID);

    // Disable face culling - we're inside the cube looking out
    RenderCommands::SetFaceCullingData cullingCmd{
        false  // disable face culling for skybox
    };
    Submit(cullingCmd);

    // Configure depth testing for skybox
    // Skybox should be rendered with depth = 1.0 (farthest)
    // Use LEQUAL so skybox pixels pass when depth buffer is cleared to 1.0
    RenderCommands::SetDepthTestData depthCmd{
        true,           // enable depth testing
        GL_LEQUAL,      // depth function (allow equal depth values)
        false           // disable depth writing (skybox shouldn't update depth)
    };
    Submit(depthCmd);

    // Bind skybox shader
    RenderCommands::BindShaderData shaderCmd{ m_SkyboxShader };
    Submit(shaderCmd);

    // Bind skybox cubemap
    RenderCommands::BindCubemapData cubemapCmd{
        m_SkyboxCubemapID,
        0,              // Texture unit 0
        m_SkyboxShader,
        "u_Skybox"
    };
    Submit(cubemapCmd);

    // Set up matrices for skybox rendering
    // Remove translation from view matrix so skybox appears infinitely far
    glm::mat4 skyboxView = glm::mat4(glm::mat3(context.frameData.viewMatrix));

    RenderCommands::SetUniformsData uniformCmd{
        m_SkyboxShader,
        glm::mat4(1.0f),                    // Identity model matrix
        skyboxView,                         // View matrix without translation
        context.frameData.projectionMatrix, // Normal projection matrix
        context.frameData.cameraPosition    // Camera position (for potential effects)
    };
    Submit(uniformCmd);

    // Draw skybox geometry
    RenderCommands::DrawElementsData drawCmd{
        m_SkyboxMesh->GetVertexArray()->GetVAOHandle(),
        m_SkyboxMesh->GetIndexCount(),
        GL_TRIANGLES
    };
    Submit(drawCmd);
}