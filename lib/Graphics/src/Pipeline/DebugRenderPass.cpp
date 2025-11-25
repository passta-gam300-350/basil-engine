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
      m_DebugLineShader(nullptr)
{
}

DebugRenderPass::DebugRenderPass(const std::shared_ptr<Shader>& debugLineShader)
    : RenderPass("DebugPass"),  // No framebuffer needed
      m_DebugLineShader(debugLineShader)
{
}

void DebugRenderPass::Execute(RenderContext& context)
{
    // Render to the main color buffer for proper alpha blending
    if (!context.frameData.mainColorBuffer)
    {
        return; // No main buffer to render to
    }

    // ONLY execute for editor camera (Scene viewport)
    // Debug visualization should not appear in game camera (Game viewport)
    if (context.frameData.currentCamera != FrameData::CameraContext::EDITOR)
    {
        return; // Skip for game camera renders
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

    // FIX: Ensure color mask is enabled (previous pass may have disabled it)
    // This is critical for vertex colors to appear correctly
    RenderCommands::SetColorMaskData enableColorMaskCmd{ true, true, true, true };
    Submit(enableColorMaskCmd);

    // Disable depth writing but keep depth testing for proper overlay rendering
    RenderCommands::SetDepthTestData depthTestCmd{
        true,           // enable depth testing (to respect scene depth)
        GL_LEQUAL,      // depth function (allow equal depth for overlays)
        false           // disable depth writing (preserve main pass depth)
    };
    Submit(depthTestCmd);

    // Render physics debug lines for visualization
    if (m_ShowPhysicsDebug && !context.frameData.debugLines.empty()) {
        RenderDebugLines(context);
    }

    // Disable blending after debug rendering
    RenderCommands::SetBlendingData disableBlendCmd{ false };
    Submit(disableBlendCmd);

    // Restore depth testing and writing to default state
    RenderCommands::SetDepthTestData restoreDepthCmd{
        true,           // enable depth testing
        GL_LESS,        // default depth function
        true            // enable depth writing
    };
    Submit(restoreDepthCmd);

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Unbind the main framebuffer
    context.frameData.mainColorBuffer->Unbind();

    // End the pass (no framebuffer unbinding since we don't have one)
    End();
}


void DebugRenderPass::RenderDebugLines(RenderContext& context)
{
    if (!m_DebugLineShader) {
        spdlog::error("DebugRenderPass: No debug line shader available for physics debug line rendering.");
        return;
    }

    if (context.frameData.debugLines.empty()) {
        return;
    }

    // Initialize VAO/VBO on first use (lazy initialization)
    if (m_DebugLineVAO == 0) {
        glGenVertexArrays(1, &m_DebugLineVAO);
        glGenBuffers(1, &m_DebugLineVBO);

        glBindVertexArray(m_DebugLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_DebugLineVBO);

        // Vertex format: position (vec3) + color (vec3)
        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        // Color attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Prepare vertex data (2 vertices per line: from and to)
    std::vector<float> vertices;
    vertices.reserve(context.frameData.debugLines.size() * 12); // 2 vertices * 6 floats per line

    for (const auto& line : context.frameData.debugLines) {
        // First vertex (from)
        vertices.push_back(line.from.x);
        vertices.push_back(line.from.y);
        vertices.push_back(line.from.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);

        // Second vertex (to)
        vertices.push_back(line.to.x);
        vertices.push_back(line.to.y);
        vertices.push_back(line.to.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);
    }

    // Upload vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_DebugLineVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Bind the debug line shader
    RenderCommands::BindShaderData bindShaderCmd{ m_DebugLineShader };
    Submit(bindShaderCmd);

    // Set line width for better visibility
    RenderCommands::SetLineWidthData lineWidthCmd{ 5.0f };
    Submit(lineWidthCmd);

    // Set camera uniforms (debug_line shader only needs view and projection, no model matrix)
    // Debug lines are already in world space, so no model transform needed
    // Use editor camera matrices (preserved) instead of general matrices (overwritten by game camera)
    Submit(RenderCommands::SetUniformMat4Data{
        m_DebugLineShader,
        "u_View",
        context.frameData.editorViewMatrix
    });

    Submit(RenderCommands::SetUniformMat4Data{
        m_DebugLineShader,
        "u_Projection",
        context.frameData.editorProjectionMatrix  // FIX: Use preserved editor projection matrix
    });

    // Draw the debug lines using command buffer
    Submit(RenderCommands::DrawArraysData{
        m_DebugLineVAO,
        static_cast<uint32_t>(context.frameData.debugLines.size() * 2),
        GL_LINES
    });

    // Restore normal line width
    RenderCommands::SetLineWidthData restoreLineWidthCmd{ 1.0f };
    Submit(restoreLineWidthCmd);
}