#include "../../include/Pipeline/SpotShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

SpotShadowMappingPass::SpotShadowMappingPass()
    : RenderPass("SpotShadowPass", FBOSpecs{
        SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer (same as directional shadows)
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(nullptr)
{
    spdlog::info("SpotShadowMappingPass created ({}x{} depth-only FBO)",
                 SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE);
}

SpotShadowMappingPass::SpotShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader)
    : RenderPass("SpotShadowPass", FBOSpecs{
        SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE,
        {
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(shadowDepthShader)
{
    spdlog::info("SpotShadowMappingPass created with shader ({}x{} depth-only FBO)",
                 SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE);
}

void SpotShadowMappingPass::Execute(RenderContext& context)
{
    // Find the first enabled spot light
    const auto& lights = context.lights;
    const SubmittedLightData* spotLight = nullptr;

    for (const auto& light : lights) {
        if (light.enabled && light.type == Light::Type::Spot) {
            spotLight = &light;
            break;  // Support first spot light only (can extend to multiple)
        }
    }

    if (!spotLight) {
        // No spot light found - skip shadow mapping and clear frame data
        if (!context.frameData.spotShadowMaps.empty()) {
            context.frameData.spotShadowMaps.clear();
            context.frameData.spotShadowMatrices.clear();
        }
        return;
    }

    // Begin shadow pass - bind depth framebuffer
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Calculate spot light view matrix (look from position toward direction)
    glm::mat4 spotView = CalculateSpotLightViewMatrix(
        spotLight->position,
        spotLight->direction
    );

    // Calculate perspective projection (FOV based on outer cone)
    glm::mat4 spotProjection = CalculateSpotLightProjectionMatrix(
        spotLight->outerCone,  // Outer cutoff in degrees
        spotLight->range       // Light range for far plane
    );

    glm::mat4 spotLightSpaceMatrix = spotProjection * spotView;

    // Store spot shadow matrix in frame data for main pass
    if (context.frameData.spotShadowMaps.empty()) {
        context.frameData.spotShadowMaps.resize(1);
        context.frameData.spotShadowMatrices.resize(1);
    }
    context.frameData.spotShadowMaps[0] = GetFramebuffer();
    context.frameData.spotShadowMatrices[0] = spotLightSpaceMatrix;

    // Clear depth buffer
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // color (not used for depth-only)
        false,                    // don't clear color
        true                      // clear depth
    };
    Submit(clearCmd);

    // Enable face culling for shadows (cull front faces to reduce peter panning)
    RenderCommands::SetFaceCullingData cullingCmd{
        true,      // enable face culling
        GL_FRONT   // cull front faces for shadows (helps with shadow acne)
    };
    Submit(cullingCmd);

    // Render shadow casters (all visible objects) with depth-only shader
    if (!context.renderables.empty())
    {
        // Use injected shadow depth shader (same as directional!)
        if (!m_ShadowDepthShader)
        {
            spdlog::error("SpotShadowMappingPass: No shadow depth shader available. Skipping shadow rendering.");
            End();
            return;
        }

        // Render each object individually with spot-light-space transformation
        for (const auto& renderable : context.renderables)
        {
            if (!renderable.visible || !renderable.mesh) continue;

            // Bind depth-only shader
            RenderCommands::BindShaderData bindShaderCmd{m_ShadowDepthShader};
            Submit(bindShaderCmd);

            // Set spot-light-space uniforms for depth-only shader
            RenderCommands::SetUniformsData uniformsCmd{
                m_ShadowDepthShader,               // Use injected depth-only shader
                renderable.transform,              // Model matrix
                spotView,                          // Spot light view matrix
                spotProjection,                    // Spot light PERSPECTIVE projection
                glm::vec3(0.0f)                   // Camera position (not needed for shadows)
            };
            Submit(uniformsCmd);

            // Submit draw command
            RenderCommands::DrawElementsData drawCmd{
                renderable.mesh->GetVertexArray()->GetVAOHandle(),
                renderable.mesh->GetIndexCount(),
                GL_TRIANGLES  // Shadow mapping uses standard triangle meshes
            };
            Submit(drawCmd);
        }
    }

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // End shadow pass
    End();
}

glm::mat4 SpotShadowMappingPass::CalculateSpotLightViewMatrix(
    const glm::vec3& position,
    const glm::vec3& direction)
{
    // Position spot light at its position, looking toward its direction
    glm::vec3 normalizedDir = glm::normalize(direction);
    glm::vec3 target = position + normalizedDir;

    // Calculate up vector to avoid singularities
    glm::vec3 up;
    if (abs(normalizedDir.y) > 0.9f) {
        // If light points mostly up/down, use X as up vector
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        // Otherwise use Y as up vector
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Create view matrix looking from position to target
    return glm::lookAt(position, target, up);
}

glm::mat4 SpotShadowMappingPass::CalculateSpotLightProjectionMatrix(
    float outerCutoffDegrees,
    float range)
{
    // FOV must cover entire spot light cone
    // outerCutoff is the half-angle of the cone
    // Full cone angle = 2 × outerCutoff
    float fov = outerCutoffDegrees * 2.0f;

    // Add small margin to ensure cone edges are fully covered
    fov += 5.0f;

    // Clamp to reasonable range to avoid extreme projections
    fov = glm::clamp(fov, 10.0f, 120.0f);

    // Shadow map is square (aspect ratio = 1.0)
    float aspectRatio = 1.0f;

    // Near/far planes based on light range
    float nearPlane = 0.1f;
    float farPlane = glm::max(range, 10.0f);  // Use light range or minimum 10 units

    // Create perspective projection matrix
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}
