#include "../../include/Pipeline/ShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
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
    m_ShadowDepthShader(shadowDepthShader)
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

    if (!directionalLight) {
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

    // Add directional shadow to the unified shadow data array
    ShadowData dirShadow;
    dirShadow.shadowType = ShadowData::Directional;
    dirShadow.lightSpaceMatrix = lightSpaceMatrix;
    dirShadow.textureIndex = static_cast<int32_t>(context.frameData.shadow2DTextures.size());
    dirShadow.farPlane = 0.0f;  // Not used for directional lights
    dirShadow.intensity = 0.8f;  // Default shadow intensity

    // Store shadow data and texture ID
    context.frameData.shadowDataArray.push_back(dirShadow);
    context.frameData.shadow2DTextures.push_back(GetFramebuffer()->GetDepthAttachmentRendererID());

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

    // Render shadow casters using INSTANCED rendering
    if (!context.renderables.empty() && m_ShadowDepthShader)
    {
        // Set light-space uniforms (same for all instances)
        RenderCommands::SetUniformMat4Data viewCmd{
            m_ShadowDepthShader,
            "u_View",
            lightView
        };
        Submit(viewCmd);

        RenderCommands::SetUniformMat4Data projCmd{
            m_ShadowDepthShader,
            "u_Projection",
            lightProjection
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
    if (abs(normalizedDir.y) > 0.9f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);  // If light points mostly up/down, use X as up
    } else {
        up = glm::vec3(0.0f, 1.0f, 0.0f);  // Otherwise use Y as up
    }

    // Light view matrix calculated

    return glm::lookAt(lightPosition, sceneCenter, up);
}

glm::mat4 ShadowMappingPass::CalculateLightProjectionMatrix(const glm::vec3& lightDirection, const FrameData& frameData)
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