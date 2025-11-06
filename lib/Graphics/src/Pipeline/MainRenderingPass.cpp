#include "../../include/Pipeline/MainRenderingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Rendering/PBRLightingRenderer.h"
#include "../../include/Rendering/ParticleRenderer.h"
#include "../../include/Scene/SceneRenderer.h"
#include "../../include/Resources/PrimitiveGenerator.h"
#include "../../include/Rendering/ParticleRenderer.h"
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

    // Configure auto-resize to match viewport
    SetResizeMode(ResizeMode::MatchViewport);
}

void MainRenderingPass::Execute(RenderContext& context)
{
    // Auto-resize framebuffer to match viewport if needed
    CheckAndResizeIfNeeded(context);

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
        m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a,  // Configurable background color
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

    // Explicitly disable blending for opaque pass (prevent state leak from previous passes)
    Submit(RenderCommands::SetBlendingData{ false });

    // Standard forward rendering with context data (no copies!)
    if (!context.renderables.empty())
    {
        // 1. Update scene-wide lighting with submitted lights
        context.pbrLighting.UpdateLighting(context.lights, context.ambientLight, context.frameData);

        // 2. Separate opaque and transparent objects
        std::vector<RenderableData> opaqueRenderables;
        std::vector<RenderableData> transparentRenderables;

        for (const auto& renderable : context.renderables)
        {
            if (renderable.visible && renderable.mesh && renderable.material)
            {
                if (renderable.material->GetBlendMode() == BlendingMode::Transparent)
                {
                    transparentRenderables.push_back(renderable);
                }
                else
                {
                    opaqueRenderables.push_back(renderable);
                }
            }
        }

        // 3. Frustum culling on submitted renderables (currently skipped)
        // auto visibleRenderables = m_FrustumCuller->CullRenderables(opaqueRenderables, context.frameData);

        // CRITICAL FIX: Build instance data ONCE with ALL renderables to avoid SSBO destruction between passes
        // The transparent pass was destroying opaque SSBOs before GPU finished rendering them
        context.instancedRenderer.RenderToPass(*this, context.renderables, context.frameData, true);

        // 6. Particle rendering
        context.particleRenderer.RenderToPass(*this, context.frameData);

        // 7. Render transparent objects (after opaque, with alpha blending)
        if (!transparentRenderables.empty())
        {
            // Enable alpha blending for transparent objects
            Submit(RenderCommands::SetBlendingData{
                true,                        // enable blending
                GL_SRC_ALPHA,                // source factor
                GL_ONE_MINUS_SRC_ALPHA       // destination factor
            });

            // Disable depth writing but keep depth testing
            Submit(RenderCommands::SetDepthTestData{
                true,           // enable depth testing
                GL_LESS,        // depth function
                false           // DISABLE depth writing (critical for transparency)
            });

            // Render transparent pass - instance data already built, just render with isOpaque=false
            context.instancedRenderer.RenderToPass(*this, context.renderables, context.frameData, false);

            // Restore depth writing
            Submit(RenderCommands::SetDepthTestData{
                true,           // enable depth testing
                GL_LESS,        // depth function
                true            // enable depth writing
            });

            // Disable blending
            Submit(RenderCommands::SetBlendingData{
                false  // disable blending
            });
        }
    }

    // Render light cubes for visualization (if enabled)
    if (m_ShowLightCubes && !context.lights.empty()) {
        // Enable alpha blending for light cube overlay rendering
        Submit(RenderCommands::SetBlendingData{ true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });

        // Disable depth writing but keep depth testing for proper overlay rendering
        Submit(RenderCommands::SetDepthTestData{
            true,           // enable depth testing (to respect scene depth)
            GL_LEQUAL,      // depth function (allow equal depth for overlays)
            false           // disable depth writing (preserve main pass depth)
        });

        RenderLightCubes(context);

        // Restore depth writing
        Submit(RenderCommands::SetDepthTestData{
            true,           // enable depth testing
            GL_LESS,        // depth function
            true            // enable depth writing
        });

        // Disable blending
        Submit(RenderCommands::SetBlendingData{ false });
    }

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Store main color buffer in frame data (direct update via reference!)
    context.frameData.mainColorBuffer = GetFramebuffer();

    // HDR texture ID will be set by HDRResolvePass after MSAA resolve

    End();
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

void MainRenderingPass::RenderLightCubes(RenderContext& context)
{
    if (!m_LightCube) {
        spdlog::warn("MainRenderingPass: No light cube mesh available for rendering.");
        return;
    }

    // Use injected primitive shader
    if (!m_PrimitiveShader) {
        spdlog::error("MainRenderingPass: No primitive shader available for light cube rendering.");
        return;
    }

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PrimitiveShader };
    Submit(bindShaderCmd);

    // Get the cube's VAO handle
    auto cubeVAO = m_LightCube->GetVertexArray();
    if (!cubeVAO) return;

    // Render each light as a cube (all light types: Directional, Point, Spot)
    for (const auto& light : context.lights) {
        // Calculate cube size based on light intensity
        float intensityBasedSize = m_BaseLightCubeSize + (light.diffuseIntensity * m_IntensityScaleFactor);
        intensityBasedSize = glm::clamp(intensityBasedSize, m_MinCubeSize, m_MaxCubeSize);

        // Calculate transform matrix for light position with intensity-based scaling
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, light.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(intensityBasedSize));

        // Set uniforms using the available SetUniformsData command
        RenderCommands::SetUniformsData uniformsCmd{
            m_PrimitiveShader,
            modelMatrix,
            context.frameData.viewMatrix,
            context.frameData.projectionMatrix,
            context.frameData.cameraPosition
        };
        Submit(uniformsCmd);

        // Set the light color using the command buffer
        // Boost color for HDR visibility (light cubes should be bright emissive objects)
        glm::vec3 cubeColor = light.color * 10.0f;  // Make very bright for HDR
        RenderCommands::SetUniformVec3Data colorCmd{
            m_PrimitiveShader,
            "u_Color",
            cubeColor
        };
        Submit(colorCmd);

        // Draw the cube using the available DrawElementsData command
        RenderCommands::DrawElementsData drawCmd{
            cubeVAO->GetVAOHandle(),
            static_cast<uint32_t>(m_LightCube->GetIndexCount()),
            GL_TRIANGLES  // Light cubes are solid triangular meshes
        };
        Submit(drawCmd);
    }
}