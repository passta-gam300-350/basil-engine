#include "../../include/Pipeline/DebugRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/Shader.h"
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>
#include <cassert>

DebugRenderPass::DebugRenderPass()
    : RenderPass("DebugPass"),  // No framebuffer needed
      m_PrimitiveShader(nullptr),
      m_LightCube(nullptr),
      m_DirectionalRay(nullptr),
      m_AABBWireframe(nullptr)
{
}

DebugRenderPass::DebugRenderPass(std::shared_ptr<Shader> primitiveShader)
    : RenderPass("DebugPass"),  // No framebuffer needed
      m_PrimitiveShader(primitiveShader),
      m_LightCube(nullptr),
      m_DirectionalRay(nullptr),
      m_AABBWireframe(nullptr)
{
}

void DebugRenderPass::Execute(RenderContext& context)
{
    // Render to the main color buffer for proper alpha blending
    if (!context.frameData.mainColorBuffer)
    {
        return; // No main buffer to render to
    }

    // Begin the pass (no framebuffer binding since we don't have one)
    Begin();

    // Bind the main framebuffer for rendering
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

    // Disable depth writing but keep depth testing for proper overlay rendering
    //RenderCommands::SetDepthTestData depthTestCmd{
    //    true,           // enable depth testing (to respect scene depth)
    //    GL_LEQUAL,      // depth function (allow equal depth for overlays)
    //    false           // disable depth writing (preserve main pass depth)
    //};
    //Submit(depthTestCmd);

    // Render light cubes for visualization
    if (m_ShowLightCubes && !context.lights.empty()) {
        RenderLightCubes(context);
    }

    // Render light rays for visualization
    if (m_ShowLightRays && !context.lights.empty()) {
        RenderLightRays(context);
    }

    // Render AABB wireframes for visualization
    if (m_ShowAABBs && !context.frameData.debugAABBs.empty()) {
        RenderAABBs(context);
    }

    // Disable blending after debug rendering
    RenderCommands::SetBlendingData disableBlendCmd{ false };
    Submit(disableBlendCmd);

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Unbind the main framebuffer
    context.frameData.mainColorBuffer->Unbind();

    // End the pass (no framebuffer unbinding since we don't have one)
    End();
}

void DebugRenderPass::RenderLightCubes(RenderContext& context)
{
    if (!m_LightCube) {
        spdlog::warn("DebugRenderPass: No light cube mesh available for rendering.");
        return;
    }

    // Use injected primitive shader
    if (!m_PrimitiveShader) {
        spdlog::error("DebugRenderPass: No primitive shader available for light cube rendering.");
        return;
    }

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PrimitiveShader };
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
}

void DebugRenderPass::RenderLightRays(RenderContext& context)
{
    // Use injected primitive shader
    if (!m_PrimitiveShader) {
        spdlog::error("DebugRenderPass: No primitive shader available for light ray rendering.");
        return;
    }

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PrimitiveShader };
    Submit(bindShaderCmd);

    // Set line width for better visibility
    RenderCommands::SetLineWidthData lineWidthCmd{ 2.0f };
    Submit(lineWidthCmd);

    // Render each light with appropriate directional ray visualization
    for (const auto& light : context.lights) {
        std::shared_ptr<Mesh> rayMesh = nullptr;
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // Only render rays for spot lights (remove directional light rays)
        if (light.type == Light::Type::Spot && m_DirectionalRay) {
            rayMesh = m_DirectionalRay;

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

        if (rayMesh) {
            // Get the ray mesh VAO handle
            auto rayVAO = rayMesh->GetVertexArray();
            if (!rayVAO) continue;

            // Set uniforms using the available SetUniformsData command
            RenderCommands::SetUniformsData uniformsCmd{
                m_PrimitiveShader,
                modelMatrix,
                context.frameData.viewMatrix,
                context.frameData.projectionMatrix,
                context.frameData.cameraPosition
            };
            Submit(uniformsCmd);

            // Set the light color (slightly dimmer for rays)
            glm::vec3 rayColor = light.color * 0.7f;  // 70% intensity for rays
            RenderCommands::SetUniformVec3Data colorCmd{
                m_PrimitiveShader,
                "u_Color",
                rayColor
            };
            Submit(colorCmd);

            // Draw the rays as lines
            RenderCommands::DrawElementsData drawCmd{
                rayVAO->GetVAOHandle(),
                static_cast<uint32_t>(rayMesh->GetIndexCount()),
                GL_LINES  // Use lines primitive mode
            };
            Submit(drawCmd);
        }
    }

    // Restore normal line width
    RenderCommands::SetLineWidthData restoreLineWidthCmd{ 1.0f };
    Submit(restoreLineWidthCmd);
}

void DebugRenderPass::RenderSingleRay(RenderContext& context, const std::shared_ptr<Mesh>& rayMesh, const glm::mat4& modelMatrix, const SubmittedLightData& light)
{
    if (!rayMesh) return;

    // Use injected primitive shader
    if (!m_PrimitiveShader) {
        spdlog::error("DebugRenderPass: No primitive shader available for single ray rendering.");
        return;
    }

    // Get the ray mesh VAO handle
    auto rayVAO = rayMesh->GetVertexArray();
    if (!rayVAO) return;

    // Set uniforms using the available SetUniformsData command
    RenderCommands::SetUniformsData uniformsCmd{
        m_PrimitiveShader,
        modelMatrix,
        context.frameData.viewMatrix,
        context.frameData.projectionMatrix,
        context.frameData.cameraPosition
    };
    Submit(uniformsCmd);

    // Set the light color (slightly dimmer for rays)
    glm::vec3 rayColor = light.color * 0.7f;  // 70% intensity for rays
    RenderCommands::SetUniformVec3Data colorCmd{
        m_PrimitiveShader,
        "u_Color",
        rayColor
    };
    Submit(colorCmd);

    // Draw the rays as lines
    RenderCommands::DrawElementsData drawCmd{
        rayVAO->GetVAOHandle(),
        static_cast<uint32_t>(rayMesh->GetIndexCount()),
        GL_LINES  // Use lines primitive mode
    };
    Submit(drawCmd);
}

void DebugRenderPass::RenderAABBs(RenderContext& context)
{
    assert(m_PrimitiveShader && "Primitive shader must be set for AABB rendering");

    if (!m_AABBWireframe) {
        spdlog::warn("DebugRenderPass: No AABB wireframe mesh available for rendering.");
        return;
    }

    // Use injected primitive shader
    if (!m_PrimitiveShader) {
        spdlog::error("DebugRenderPass: No primitive shader available for AABB rendering.");
        return;
    }

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PrimitiveShader };
    Submit(bindShaderCmd);

    // Set line width for better visibility
    RenderCommands::SetLineWidthData lineWidthCmd{ 1.5f };
    Submit(lineWidthCmd);

    // Get the wireframe mesh VAO handle
    auto wireframeVAO = m_AABBWireframe->GetVertexArray();
    if (!wireframeVAO) {
        spdlog::error("DebugRenderPass: AABB wireframe mesh has no vertex array.");
        return;
    }

    // Render each AABB wireframe
    for (const auto& debugAABB : context.frameData.debugAABBs) {
        if (!debugAABB.visible || !debugAABB.boundingBox.IsValid()) {
            continue;
        }

        glm::mat4 modelMatrix;

        if (debugAABB.isLocalAABB) {
            // For local AABBs, apply the object's transform matrix to the local bounds
            glm::vec3 center = debugAABB.boundingBox.GetCenter();
            glm::vec3 size = debugAABB.boundingBox.GetSize();

            // Create local AABB transform
            glm::mat4 localAABBMatrix = glm::mat4(1.0f);
            localAABBMatrix = glm::translate(localAABBMatrix, center);
            localAABBMatrix = glm::scale(localAABBMatrix, size);

            // Apply object transform to show local AABB in world space
            modelMatrix = debugAABB.objectTransform * localAABBMatrix;
        } else {
            // For world AABBs, use the AABB bounds directly (already in world space)
            glm::vec3 center = debugAABB.boundingBox.GetCenter();
            glm::vec3 size = debugAABB.boundingBox.GetSize();

            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, center);
            modelMatrix = glm::scale(modelMatrix, size);
        }

        // Set uniforms using the available SetUniformsData command
        RenderCommands::SetUniformsData uniformsCmd{
            m_PrimitiveShader,
            modelMatrix,
            context.frameData.viewMatrix,
            context.frameData.projectionMatrix,
            context.frameData.cameraPosition
        };
        Submit(uniformsCmd);

        // Set the AABB color with different line width for local vs world AABBs
        if (debugAABB.isLocalAABB) {
            // Local AABBs use thicker lines to distinguish them
            RenderCommands::SetLineWidthData localLineWidthCmd{ 2.0f };
            Submit(localLineWidthCmd);
        }

        RenderCommands::SetUniformVec3Data colorCmd{
            m_PrimitiveShader,
            "u_Color",
            debugAABB.color
        };
        Submit(colorCmd);

        // Draw the wireframe using lines
        RenderCommands::DrawElementsData drawCmd{
            wireframeVAO->GetVAOHandle(),
            static_cast<uint32_t>(m_AABBWireframe->GetIndexCount()),
            GL_LINES  // Use lines primitive mode for wireframe
        };
        Submit(drawCmd);

        // Restore line width after local AABB
        if (debugAABB.isLocalAABB) {
            RenderCommands::SetLineWidthData restoreLineWidthCmd{ 1.5f };
            Submit(restoreLineWidthCmd);
        }
    }

    // Restore normal line width
    RenderCommands::SetLineWidthData restoreLineWidthCmd{ 1.0f };
    Submit(restoreLineWidthCmd);
}