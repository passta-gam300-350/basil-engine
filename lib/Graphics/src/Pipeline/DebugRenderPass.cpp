#include "../../include/Pipeline/DebugRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/PrimitiveGenerator.h"
#include "../../include/Resources/ResourceManager.h"
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>

DebugRenderPass::DebugRenderPass()
    : RenderPass("DebugPass", FBOSpecs{0, 0, {}})  // No FBO - renders directly to main buffer
{
    InitializeLightVisualization();
}

void DebugRenderPass::Execute(RenderContext& context)
{
    ClearCommands();

    // Render directly to the main color buffer for proper alpha blending
    if (!context.frameData.mainColorBuffer)
    {
        return; // No main buffer to render to
    }

    // Bind the main framebuffer instead of our own debug framebuffer
    context.frameData.mainColorBuffer->Bind();

    // Set viewport to match main framebuffer
    const auto &mainFBOSpecs = context.frameData.mainColorBuffer->GetSpecification();
    glViewport(0, 0, static_cast<int>(mainFBOSpecs.Width), static_cast<int>(mainFBOSpecs.Height));

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Enable alpha blending for debug overlay rendering
    // Make sure blending doesn't affect depth writes
    RenderCommands::SetBlendingData enableBlendCmd{ true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
    Submit(enableBlendCmd);

    // Render light cubes for visualization
    if (m_ShowLightCubes && !context.lights.empty()) {
        RenderLightCubes(context);
    }

    // Render light rays for visualization
    if (m_ShowLightRays && !context.lights.empty()) {
        RenderLightRays(context);
    }

    // Disable blending after debug rendering
    RenderCommands::SetBlendingData disableBlendCmd{ false };
    Submit(disableBlendCmd);


    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // No need to store debug buffer since we rendered directly to main buffer
    // Main buffer already contains the composited result
}

void DebugRenderPass::InitializeLightVisualization()
{
    // Create a unit cube mesh for light visualization (scaling will be done per-light)
    auto cube = PrimitiveGenerator::CreateCube(1.0f);
    m_LightCube = std::make_unique<Mesh>(cube);

    // Create light ray mesh for directional visualization (single line for both spot and directional)
    auto directionRay = PrimitiveGenerator::CreateDirectionalRay(m_RayLength);
    m_DirectionalRay = std::make_unique<Mesh>(directionRay);
}

void DebugRenderPass::RenderLightCubes(RenderContext& context)
{
    if (!m_LightCube) return;

    // Get the primitive shader for simple colored rendering
    auto primitiveShader = context.resourceManager.GetShader("primitive");
    if (!primitiveShader) return;

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ primitiveShader };
    Submit(bindShaderCmd);

    // Get the cube's VAO handle
    auto cubeVAO = m_LightCube->GetVertexArray();
    if (!cubeVAO) return;

    // Render each point and spot light as a cube
    for (const auto& light : context.lights) {
        if (light.type == Light::Type::Point || light.type == Light::Type::Spot) {

            // Calculate cube size based on light intensity
            float intensityBasedSize = m_BaseLightCubeSize + (light.intensity * m_IntensityScaleFactor);
            intensityBasedSize = glm::clamp(intensityBasedSize, m_MinCubeSize, m_MaxCubeSize);

            // Calculate transform matrix for light position with intensity-based scaling
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, light.position);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(intensityBasedSize));

            // Set uniforms using the available SetUniformsData command
            RenderCommands::SetUniformsData uniformsCmd{
                primitiveShader,
                modelMatrix,
                context.frameData.viewMatrix,
                context.frameData.projectionMatrix,
                context.frameData.cameraPosition
            };
            Submit(uniformsCmd);

            // Set the light color using the command buffer
            RenderCommands::SetUniformVec3Data colorCmd{
                primitiveShader,
                "u_Color",
                light.color
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
}

void DebugRenderPass::RenderLightRays(RenderContext& context)
{
    // Get the primitive shader for wireframe rendering
    auto primitiveShader = context.resourceManager.GetShader("primitive");
    if (!primitiveShader) return;

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ primitiveShader };
    Submit(bindShaderCmd);

    // Set line width for better visibility
    RenderCommands::SetLineWidthData lineWidthCmd{ 2.0f };
    Submit(lineWidthCmd);

    // Render each light with appropriate directional ray visualization
    for (const auto& light : context.lights) {
        std::unique_ptr<Mesh>* rayMesh = nullptr;
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // Only render rays for spot lights (remove directional light rays)
        if (light.type == Light::Type::Spot && m_DirectionalRay) {
            rayMesh = &m_DirectionalRay;

            // Ray length directly represents light intensity
            float intensityBasedLength = light.intensity * m_RayIntensityFactor;
            intensityBasedLength = glm::clamp(intensityBasedLength, 0.5f, 15.0f);

            // Spot lights have a specific position - draw single ray
            glm::vec3 rayOrigin = light.position;
            modelMatrix = glm::translate(modelMatrix, rayOrigin);

            // Orient ray in light direction
            glm::vec3 forward = glm::normalize(light.direction);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            // Handle case where light direction is parallel to up vector
            if (abs(glm::dot(forward, up)) > 0.99f) {
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            glm::vec3 right = glm::normalize(glm::cross(up, forward));
            up = glm::normalize(glm::cross(forward, right));

            // Create rotation matrix to align -Z axis with light direction
            glm::mat4 rotationMatrix = glm::mat4(1.0f);
            rotationMatrix[0] = glm::vec4(right, 0.0f);
            rotationMatrix[1] = glm::vec4(up, 0.0f);
            rotationMatrix[2] = glm::vec4(-forward, 0.0f);  // -Z maps to light direction

            modelMatrix = modelMatrix * rotationMatrix;
            modelMatrix = glm::scale(modelMatrix, glm::vec3(intensityBasedLength));

            // Render single spot light ray
            RenderSingleRay(context, rayMesh, modelMatrix, light);

            // Skip the common rendering code since we handled it above
            continue;
        }

        if (rayMesh && *rayMesh) {
            // Get the ray mesh VAO handle
            auto rayVAO = (*rayMesh)->GetVertexArray();
            if (!rayVAO) continue;

            // Set uniforms using the available SetUniformsData command
            RenderCommands::SetUniformsData uniformsCmd{
                primitiveShader,
                modelMatrix,
                context.frameData.viewMatrix,
                context.frameData.projectionMatrix,
                context.frameData.cameraPosition
            };
            Submit(uniformsCmd);

            // Set the light color (slightly dimmer for rays)
            glm::vec3 rayColor = light.color * 0.7f;  // 70% intensity for rays
            RenderCommands::SetUniformVec3Data colorCmd{
                primitiveShader,
                "u_Color",
                rayColor
            };
            Submit(colorCmd);

            // Draw the rays as lines
            RenderCommands::DrawElementsData drawCmd{
                rayVAO->GetVAOHandle(),
                static_cast<uint32_t>((*rayMesh)->GetIndexCount()),
                GL_LINES  // Use lines primitive mode
            };
            Submit(drawCmd);
        }
    }

    // Restore normal line width
    RenderCommands::SetLineWidthData restoreLineWidthCmd{ 1.0f };
    Submit(restoreLineWidthCmd);
}

void DebugRenderPass::RenderSingleRay(RenderContext& context, std::unique_ptr<Mesh>* rayMesh, const glm::mat4& modelMatrix, const SubmittedLightData& light)
{
    if (!rayMesh || !*rayMesh) return;

    // Get the primitive shader for wireframe rendering
    auto primitiveShader = context.resourceManager.GetShader("primitive");
    if (!primitiveShader) return;

    // Get the ray mesh VAO handle
    auto rayVAO = (*rayMesh)->GetVertexArray();
    if (!rayVAO) return;

    // Set uniforms using the available SetUniformsData command
    RenderCommands::SetUniformsData uniformsCmd{
        primitiveShader,
        modelMatrix,
        context.frameData.viewMatrix,
        context.frameData.projectionMatrix,
        context.frameData.cameraPosition
    };
    Submit(uniformsCmd);

    // Set the light color (slightly dimmer for rays)
    glm::vec3 rayColor = light.color * 0.7f;  // 70% intensity for rays
    RenderCommands::SetUniformVec3Data colorCmd{
        primitiveShader,
        "u_Color",
        rayColor
    };
    Submit(colorCmd);

    // Draw the rays as lines
    RenderCommands::DrawElementsData drawCmd{
        rayVAO->GetVAOHandle(),
        static_cast<uint32_t>((*rayMesh)->GetIndexCount()),
        GL_LINES  // Use lines primitive mode
    };
    Submit(drawCmd);
}

