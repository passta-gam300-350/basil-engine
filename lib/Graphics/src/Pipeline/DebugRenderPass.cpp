#include "../../include/Pipeline/DebugRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/PrimitiveGenerator.h"
#include "../../include/Resources/ResourceManager.h"
#include <glm/ext.hpp>
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

    // Disable blending after debug rendering
    RenderCommands::SetBlendingData disableBlendCmd{ false };
    Submit(disableBlendCmd);

    // Reset depth function to default
    glDepthFunc(GL_LESS);

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
                static_cast<uint32_t>(m_LightCube->GetIndexCount())
            };
            Submit(drawCmd);
        }
    }
}