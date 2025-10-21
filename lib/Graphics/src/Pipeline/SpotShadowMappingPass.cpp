#include "../../include/Pipeline/SpotShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

SpotShadowMappingPass::SpotShadowMappingPass()
    : RenderPass("SpotShadowPass"),  // No default framebuffer - we'll create dynamically
      m_ShadowDepthShader(nullptr)
{
    spdlog::info("SpotShadowMappingPass created (dynamic framebuffers for multiple spot lights)");
}

SpotShadowMappingPass::SpotShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader)
    : RenderPass("SpotShadowPass"),
      m_ShadowDepthShader(shadowDepthShader)
{
    spdlog::info("SpotShadowMappingPass created with shader (supports up to {} spot lights)",
                 MAX_SPOT_LIGHTS);
}

void SpotShadowMappingPass::Execute(RenderContext& context)
{
    // Collect all enabled spot lights
    std::vector<const SubmittedLightData*> spotLights;
    for (const auto& light : context.lights) {
        if (light.enabled && light.type == Light::Type::Spot) {
            spotLights.push_back(&light);
            if (spotLights.size() >= MAX_SPOT_LIGHTS) {
                spdlog::warn("SpotShadowMappingPass: Exceeded maximum spot lights ({}), ignoring extras", MAX_SPOT_LIGHTS);
                break;
            }
        }
    }

    // If no spot lights, clear frame data and return
    if (spotLights.empty()) {
        context.frameData.spotShadowMaps.clear();
        context.frameData.spotShadowMatrices.clear();
        return;
    }

    // Ensure we have enough framebuffers
    EnsureFramebuffers(spotLights.size());

    // Clear and resize frame data
    context.frameData.spotShadowMaps.clear();
    context.frameData.spotShadowMatrices.clear();
    context.frameData.spotShadowMaps.resize(spotLights.size());
    context.frameData.spotShadowMatrices.resize(spotLights.size());

    // Render shadow map for each spot light
    for (size_t i = 0; i < spotLights.size(); ++i) {
        const auto* spotLight = spotLights[i];
        auto& shadowFBO = m_SpotShadowFramebuffers[i];

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

        // Store spot shadow map and matrix in frame data for main pass
        context.frameData.spotShadowMaps[i] = shadowFBO;
        context.frameData.spotShadowMatrices[i] = spotLightSpaceMatrix;

        // Bind this spot light's framebuffer
        shadowFBO->Bind();

        // Set viewport to match shadow map size
        glViewport(0, 0, SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE);

        // Setup command buffer with systems from context
        SetupCommandBuffer(context);

        // Clear depth buffer
        RenderCommands::ClearData clearCmd{
            0.0f, 0.0f, 0.0f, 1.0f,  // color (not used for depth-only)
            false,                    // don't clear color
            true,                     // clear depth
            false                      // don't clear stencil
        };
        Submit(clearCmd);

        // Enable face culling for shadows (cull front faces to reduce peter panning)
        RenderCommands::SetFaceCullingData cullingCmd{
            true,      // enable face culling
            GL_FRONT   // cull front faces for shadows (helps with shadow acne)
        };
        Submit(cullingCmd);

        // Render shadow casters using INSTANCED rendering
        if (!context.renderables.empty() && m_ShadowDepthShader)
        {
            // Set light-space uniforms (same for all instances)
            RenderCommands::SetUniformMat4Data viewCmd{
                m_ShadowDepthShader,
                "u_View",
                spotView
            };
            Submit(viewCmd);

            RenderCommands::SetUniformMat4Data projCmd{
                m_ShadowDepthShader,
                "u_Projection",
                spotProjection
            };
            Submit(projCmd);

            // Use InstancedRenderer to render all objects with instancing
            context.instancedRenderer.RenderShadowToPass(
                *this,                    // This render pass
                context.renderables,      // All visible renderables
                m_ShadowDepthShader       // Instanced shadow depth shader
            );
        }

        // Execute all commands submitted to this pass's command buffer
        ExecuteCommands();

        // Unbind framebuffer
        shadowFBO->Unbind();
    }
}

void SpotShadowMappingPass::EnsureFramebuffers(size_t count)
{
    // Create additional framebuffers if needed
    while (m_SpotShadowFramebuffers.size() < count) {
        FBOSpecs specs{
            SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE,
            {
                // Depth-only framebuffer (same as directional shadows)
                { FBOTextureFormat::DEPTH24STENCIL8 }
            }
        };

        auto fbo = std::make_shared<FrameBuffer>(specs);
        m_SpotShadowFramebuffers.push_back(fbo);

        spdlog::info("SpotShadowMappingPass: Created framebuffer #{} ({}x{})",
                     m_SpotShadowFramebuffers.size(),
                     SPOT_SHADOW_MAP_SIZE, SPOT_SHADOW_MAP_SIZE);
    }
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
