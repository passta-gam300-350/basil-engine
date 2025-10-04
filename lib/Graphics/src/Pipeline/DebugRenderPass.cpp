/******************************************************************************/
/*!
\file   DebugRenderPass.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of debug rendering pass for visualizing lights and AABBs

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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

DebugRenderPass::DebugRenderPass(const std::shared_ptr<Shader>& primitiveShader)
    : RenderPass("DebugPass"),  // No framebuffer needed
      m_PrimitiveShader(primitiveShader),
      m_LightCube(nullptr),
      m_DirectionalRay(nullptr),
      m_AABBWireframe(nullptr)
{
}

void DebugRenderPass::Execute(RenderContext& context)
{
    // Check if we have a main buffer to copy to editor FBO
    if (!context.frameData.mainColorBuffer)
    {
        return; // No main buffer to copy
    }

	// Render debug visualizations if enabled
	if (m_ShowAABBs) {
		// Clear any previous commands
		ClearCommands();

		// Bind the main color buffer to draw AABBs on top of the rendered scene
		if (context.frameData.mainColorBuffer) {
			context.frameData.mainColorBuffer->Bind();

			RenderAABBs(context);

			// Execute all submitted commands on the main color buffer
			ExecuteCommands();

			context.frameData.mainColorBuffer->Unbind();
		}
	}

    // Copy main scene (now with AABBs drawn on it) to editor FBO
    UpdateEditorFBOWithDebug(context);
}

void DebugRenderPass::RenderLightCubes(RenderContext& context)
{
    if (!m_LightCube) {
        spdlog::warn("DebugRenderPass: No light cube mesh available for rendering.");
        return;
    }

    // Use injected primitive shader
    if (m_PrimitiveShader == nullptr) {
        spdlog::error("DebugRenderPass: No primitive shader available for light cube rendering.");
        return;
    }

    // Bind the shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PrimitiveShader };
    Submit(bindShaderCmd);

    // Get the cube's VAO handle
    auto cubeVAO = m_LightCube->GetVertexArray();
    if (!cubeVAO) {
        return;
    }

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
            RenderCommands::SetUniformVec3Data colorCmd{
                m_PrimitiveShader,
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
    // Use injected primitive shader
    if (m_PrimitiveShader == nullptr) {
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
            if (!rayVAO) {
                continue;
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
    if (!rayMesh) {
        return;
    }

    // Use injected primitive shader
    if (m_PrimitiveShader == nullptr) {
        spdlog::error("DebugRenderPass: No primitive shader available for single ray rendering.");
        return;
    }

    // Get the ray mesh VAO handle
    auto rayVAO = rayMesh->GetVertexArray();
    if (!rayVAO) {
        return;
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
    if (!m_AABBWireframe) {
        spdlog::warn("DebugRenderPass: No AABB wireframe mesh available for rendering.");
        return;
    }

    // Use injected primitive shader
    if (m_PrimitiveShader == nullptr) {
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

void DebugRenderPass::UpdateEditorFBOWithDebug(RenderContext &context)
{
    // Only update editor FBO if main buffer exists (we can create editor buffer if needed)
    if (!context.frameData.mainColorBuffer)
    {
        return;
    }

    auto mainFBO = context.frameData.mainColorBuffer;
    const auto &mainSpec = mainFBO->GetSpecification();

    // Create or update editor FBO to match main FBO size
    if (!context.frameData.editorColorBuffer ||
        context.frameData.editorColorBuffer->GetSpecification().Width != mainSpec.Width ||
        context.frameData.editorColorBuffer->GetSpecification().Height != mainSpec.Height)
    {
        // Create identical FBO specs for editor copy
        FBOSpecs editorSpec = mainSpec;
        context.frameData.editorColorBuffer = std::make_shared<FrameBuffer>(editorSpec);
    }

    auto editorFBO = context.frameData.editorColorBuffer;

    // Copy main scene content to editor FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO->GetFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, editorFBO->GetFBOHandle());

    glBlitFramebuffer(
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        0, 0, static_cast<int>(mainSpec.Width), static_cast<int>(mainSpec.Height),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // Restore framebuffer binding
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}