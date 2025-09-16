#include "../../include/Pipeline/ShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/Renderer.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/ResourceManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

ShadowMappingPass::ShadowMappingPass()
    : RenderPass("ShadowPass", FBOSpecs{
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        {
            // Depth-only framebuffer for shadow mapping
            { FBOTextureFormat::DEPTH24STENCIL8 }
        }
    })
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

    std::cout << "ShadowMappingPass: Rendering shadows for directional light" << std::endl;

    // Begin shadow pass - bind depth framebuffer
    Begin();

    // Calculate light-space matrices
    glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);  // Center of our 2x2 grid
    glm::mat4 lightView = CalculateLightViewMatrix(directionalLight->direction, sceneCenter);
    glm::mat4 lightProjection = CalculateLightProjectionMatrix(directionalLight->direction, context.frameData);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Store shadow matrix in frame data for main pass
    if (context.frameData.shadowMaps.size() == 0) {
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

    // Create sort key for shadow pass - use pass ID 0 to execute first
    RenderCommands::CommandSortKey shadowSortKey;
    shadowSortKey.pass = SHADOW_PASS_ID;
    shadowSortKey.material = 0;
    shadowSortKey.mesh = 0;
    shadowSortKey.instance = 0;

    context.renderer.Submit(clearCmd, shadowSortKey);

    // Render shadow casters (all visible objects) with depth-only shader
    if (!context.renderables.empty()) {
        std::cout << "ShadowMappingPass: Rendering " << context.renderables.size() << " shadow casters" << std::endl;

        // TODO: We need a depth-only shader here
        // For now, we'll create a simplified render without proper shader
        // This will be implemented in the next step

        // Load depth-only shader for shadow mapping
        auto shadowShader = context.resourceManager.GetShader("shadow_depth");
        if (!shadowShader) {
            std::cout << "Warning: shadow_depth shader not found, skipping shadow mapping" << std::endl;
            End();
            return;
        }

        // Render each object individually with light-space transformation
        for (const auto& renderable : context.renderables) {
            if (!renderable.visible || !renderable.mesh) continue;

            shadowSortKey.mesh = reinterpret_cast<uintptr_t>(renderable.mesh.get()) & 0xFFFF;

            // Set light-space uniforms for depth-only shader
            RenderCommands::SetUniformsData uniformsCmd{
                shadowShader,                      // Use depth-only shader
                renderable.transform,              // Model matrix
                lightView,                         // Light view matrix
                lightProjection,                   // Light projection matrix
                glm::vec3(0.0f)                   // Camera position (not needed for shadows)
            };
            context.renderer.Submit(uniformsCmd, shadowSortKey);

            // Submit draw command
            RenderCommands::DrawElementsData drawCmd{
                renderable.mesh->GetVertexArray()->GetVAOHandle(),
                renderable.mesh->GetIndexCount()
            };
            context.renderer.Submit(drawCmd, shadowSortKey);
        }
    }

    std::cout << "ShadowMappingPass: Shadow map stored in FrameData::shadowMaps[0]" << std::endl;
    std::cout << "ShadowMappingPass: Shadow map texture ID = " << GetFramebuffer()->GetDepthAttachmentRendererID() << std::endl;

    // End shadow pass
    End();
}

glm::mat4 ShadowMappingPass::CalculateLightViewMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter)
{
    // Position light far away in the opposite direction
    glm::vec3 lightPosition = sceneCenter - normalize(lightDirection) * 50.0f;

    // Look towards the scene center
    glm::vec3 up = abs(lightDirection.y) > 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    return glm::lookAt(lightPosition, sceneCenter, up);
}

glm::mat4 ShadowMappingPass::CalculateLightProjectionMatrix(const glm::vec3& lightDirection, const FrameData& frameData)
{
    // Create orthographic projection that covers the scene
    // For a 2x2 grid with spacing 3.0f, scene spans roughly -1.5 to +1.5 in X/Z
    // Add margin for ground plane and light angle
    float orthoSize = 10.0f;  // Covers scene + margin

    return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
}