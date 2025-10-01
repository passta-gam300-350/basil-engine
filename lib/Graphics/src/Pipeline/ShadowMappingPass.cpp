#include "../../include/Pipeline/ShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

ShadowMappingPass::ShadowMappingPass()
    : RenderPass("ShadowPass", FBOSpecs{
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer for shadow mapping
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(nullptr)
{
}

ShadowMappingPass::ShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader)
    : RenderPass("ShadowPass", FBOSpecs{
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer for shadow mapping
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(std::move(shadowDepthShader))
{
}

void ShadowMappingPass::Execute(RenderContext& context)
{
    // Find the primary directional light (first one in the list)
    const auto& lights = context.lights;
    const SubmittedLightData* directionalLight = nullptr;

    for (const auto& light : lights) {
        if (light.enabled && light.type == Light::Type::Directional) {
            directionalLight = &light;
            break;  // Use first directional light
        }
    }

    if (directionalLight == nullptr) {
        // No directional light found - skip shadow mapping
        return;
    }

    // Begin shadow pass - bind depth framebuffer
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Calculate light-space matrices
    glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);  // Center of our 2x2 grid
    glm::mat4 lightView = CalculateLightViewMatrix(directionalLight->direction, sceneCenter);
    glm::mat4 lightProjection = CalculateLightProjectionMatrix(directionalLight->direction, context.frameData);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Store shadow matrix in frame data for main pass
    if (context.frameData.shadowMaps.empty()) {
        context.frameData.shadowMaps.resize(1);
        context.frameData.shadowMatrices.resize(1);
    }
    context.frameData.shadowMaps[0] = GetFramebuffer();
    context.frameData.shadowMatrices[0] = lightSpaceMatrix;

    // Clear depth buffer
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // color (not used for depth-only)
        false,                    // don't clear color
        true                      // clear depth
    };

    Submit(clearCmd);

    // Render shadow casters (all visible objects) with depth-only shader
    if (!context.renderables.empty())
    {
        // Use injected shadow depth shader
        if (!m_ShadowDepthShader)
        {
            spdlog::error("ShadowMappingPass: No shadow depth shader available. Skipping shadow rendering.");
            End();
            return;
        }

        // Render each object individually with light-space transformation
        for (const auto& renderable : context.renderables)
        {
            if (!renderable.visible || !renderable.mesh) {
                continue;
            }

            // Using depth-only shader for shadow casting
            // Bind depth-only shader first
            RenderCommands::BindShaderData bindShaderCmd{m_ShadowDepthShader};
            Submit(bindShaderCmd);

            // Set light-space uniforms for depth-only shader
            RenderCommands::SetUniformsData uniformsCmd{
                m_ShadowDepthShader,               // Use injected depth-only shader
                renderable.transform,              // Model matrix
                lightView,                         // Light view matrix
                lightProjection,                   // Light projection matrix
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

glm::mat4 ShadowMappingPass::CalculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter)
{
    // Position light far away in the opposite direction
    glm::vec3 normalizedDir = glm::normalize(lightDirection);
    glm::vec3 lightPosition = sceneCenter - normalizedDir * 20.0f;  // Closer light

    // Better up vector calculation to avoid singularities
    glm::vec3 up;
    // Extract y component without union access warning
    float dirY = normalizedDir[1];
    if (glm::abs(dirY) > 0.9f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);  // If light points mostly up/down, use X as up
    } else {
        up = glm::vec3(0.0f, 1.0f, 0.0f);  // Otherwise use Y as up
    }

    // Light view matrix calculated

    return glm::lookAt(lightPosition, sceneCenter, up);
}

glm::mat4 ShadowMappingPass::CalculateLightProjectionMatrix(const glm::vec3& /*lightDirection*/, const FrameData& /*frameData*/)
{
    // Create orthographic projection that covers the scene
    // For a 2x2 grid with spacing 3.0f, scene spans roughly -1.5 to +1.5 in X/Z
    // Add margin for ground plane and light angle
    float orthoSize = 15.0f;  // Smaller, tighter bounds
    float nearPlane = 1.0f;
    float farPlane = 50.0f;

    // Orthographic projection calculated

    return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
}