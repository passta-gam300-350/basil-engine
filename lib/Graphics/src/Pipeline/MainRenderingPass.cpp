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
            { FBOTextureFormat::RGB16F },  // HDR format (16-bit float) - ogldev style
            { FBOTextureFormat::DEPTH24STENCIL8 }
		},
        4  // 4x MSAA (resolved by HDRResolvePass before tone mapping)
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

    // Ensure stencil testing is disabled before clearing (clean state)
    Submit(RenderCommands::EnableStencilTestData{ false });

    // Ensure stencil write mask is enabled before clearing
    Submit(RenderCommands::SetStencilMaskData{ 0xFF });

    // Clear color, depth, and stencil buffers using command buffer
    RenderCommands::ClearData clearCmd{
        0.7f, 0.7f, 0.7f, 1.0f,  // Gray background
        true,                      // clearColor
        true,                      // clearDepth
        true                       // clearStencil (needed for outline rendering)
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

    // HDR texture ID will be set by HDRResolvePass after MSAA resolve

    End();
}

void MainRenderingPass::UpdateFramebufferSize()
{
    int targetWidth, targetHeight;

    if (m_UseTargetViewport) {
        // Use editor-specified viewport size (ImGui viewport)
        targetWidth = m_TargetViewportWidth;
        targetHeight = m_TargetViewportHeight;
    } else {
        // Use full window framebuffer size (standalone mode)
        GLFWwindow* currentWindow = glfwGetCurrentContext();
        if (!currentWindow) return;

        glfwGetFramebufferSize(currentWindow, &targetWidth, &targetHeight);
    }

    // Skip if viewport is minimized
    if (targetWidth == 0 || targetHeight == 0) {
        return;
    }

    // Check if we need to resize the framebuffer
    auto currentSpecs = m_Framebuffer->GetSpecification();
    if (currentSpecs.Width != static_cast<uint32_t>(targetWidth) ||
        currentSpecs.Height != static_cast<uint32_t>(targetHeight)) {

        spdlog::info("MainRenderingPass: Resizing framebuffer from {}x{} to {}x{}",
                     currentSpecs.Width, currentSpecs.Height, targetWidth, targetHeight);

        // Resize the framebuffer to match target viewport size
        m_Framebuffer->Resize(static_cast<uint32_t>(targetWidth), static_cast<uint32_t>(targetHeight));

        // Update the viewport to match the new framebuffer size
        SetViewport(Viewport(0, 0, static_cast<uint32_t>(targetWidth), static_cast<uint32_t>(targetHeight)));
    }
}

void MainRenderingPass::SetTargetViewportSize(int width, int height)
{
    m_UseTargetViewport = true;
    m_TargetViewportWidth = width;
    m_TargetViewportHeight = height;
    spdlog::info("MainRenderingPass: Target viewport size set to {}x{}", width, height);
}

void MainRenderingPass::UseWindowSize()
{
    m_UseTargetViewport = false;
    spdlog::info("MainRenderingPass: Reverting to window size");
}

void MainRenderingPass::RenderSkybox(RenderContext& context)
{
    if (!m_SkyboxEnabled || !m_SkyboxShader || !m_SkyboxMesh || m_SkyboxCubemapID == 0)
    {
       /* spdlog::warn("Skybox not rendering - Enabled: {}, Shader: {}, Mesh: {}, CubemapID: {}",
                     m_SkyboxEnabled, m_SkyboxShader != nullptr, m_SkyboxMesh != nullptr, m_SkyboxCubemapID);*/
        return; // Skip if not properly configured
    }

    //spdlog::info("Rendering skybox with cubemap ID: {}", m_SkyboxCubemapID);

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

    // CRITICAL: Unbind cubemap from texture unit 0 to prevent conflicts
    // Scene objects will try to bind 2D textures to unit 0, which conflicts with cubemap
    RenderCommands::BindCubemapData unbindCmd{
        0,              // Unbind by binding texture ID 0
        0,              // Texture unit 0
        m_SkyboxShader,
        "u_Skybox"
    };
    Submit(unbindCmd);
}