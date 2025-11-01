#include "../../include/Pipeline/DirectionalShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <cfloat>  // For FLT_MAX

DirectionalShadowMappingPass::DirectionalShadowMappingPass()
    : RenderPass("DirectionalShadowPass", FBOSpecs{
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer for shadow mapping
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(nullptr)
{
}

DirectionalShadowMappingPass::DirectionalShadowMappingPass(std::shared_ptr<Shader> shadowDepthShader)
    : RenderPass("DirectionalShadowPass", FBOSpecs{
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer for shadow mapping
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    }),
    m_ShadowDepthShader(shadowDepthShader)
{
}

void DirectionalShadowMappingPass::Execute(RenderContext& context)
{
    // Find the primary directional light (first one in the list)
    const auto& lights = context.lights;
    const SubmittedLightData* directionalLight = nullptr;

    for (const auto& light : lights) {
        if (light.enabled && light.castShadows && light.type == Light::Type::Directional) {
            directionalLight = &light;
            break;  // Use first directional light
        }
    }

    if (!directionalLight) {
        // No directional light found or shadows disabled - skip shadow mapping
        return;
    }

    // Begin shadow pass - bind depth framebuffer
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Calculate scene bounds from actual renderables
    glm::vec3 sceneMin(FLT_MAX);
    glm::vec3 sceneMax(-FLT_MAX);
    for (const auto& renderable : context.renderables) {
        // Extract position from transform matrix (last column)
        glm::vec3 position = glm::vec3(renderable.transform[3]);
        sceneMin = glm::min(sceneMin, position);
        sceneMax = glm::max(sceneMax, position);
    }
    glm::vec3 sceneCenter = (sceneMin + sceneMax) * 0.5f;
    float sceneRadius = glm::length(sceneMax - sceneMin) * 0.5f;

    spdlog::debug("DirectionalShadow: SceneCenter=({:.1f},{:.1f},{:.1f}), Radius={:.1f}",
                 sceneCenter.x, sceneCenter.y, sceneCenter.z, sceneRadius);

    // Calculate light-space matrices using actual scene bounds
    glm::mat4 lightView = CalculateLightViewMatrix(directionalLight->direction, sceneCenter);
    glm::mat4 lightProjection = CalculateLightProjectionMatrix(directionalLight->direction, context.frameData, sceneRadius);
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

glm::mat4 DirectionalShadowMappingPass::CalculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter)
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

glm::mat4 DirectionalShadowMappingPass::CalculateLightProjectionMatrix(const glm::vec3& lightDirection, const FrameData& frameData, float sceneRadius)
{
    // Use scene radius to calculate optimal ortho size
    // Add 20% padding to ensure all objects are covered
    float orthoSize = sceneRadius * 1.2f;

    // Clamp to reasonable range (minimum 10 units, maximum 500 units)
    orthoSize = glm::clamp(orthoSize, 10.0f, 500.0f);

    // Near/far planes based on scene size
    float nearPlane = 0.1f;
    float farPlane = sceneRadius * 4.0f;  // Far enough to cover scene depth
    farPlane = glm::max(farPlane, 50.0f);  // Minimum 50 units

    spdlog::debug("DirectionalShadow: orthoSize={}, near={}, far={}, sceneRadius={}",
                 orthoSize, nearPlane, farPlane, sceneRadius);

    return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
}