#include "../../include/Pipeline/OutlineRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

OutlineRenderPass::OutlineRenderPass()
    : RenderPass("OutlinePass"),  // No framebuffer needed - renders to main buffer
      m_OutlineShader(nullptr)
{
}

OutlineRenderPass::OutlineRenderPass(std::shared_ptr<Shader> outlineShader)
    : RenderPass("OutlinePass"),
      m_OutlineShader(outlineShader)
{
}

void OutlineRenderPass::Execute(RenderContext& context)
{
    // Skip if disabled or no objects to outline
    if (!m_Enabled || m_OutlinedObjects.empty()) {
        return;
    }

    // Check if we have the main color buffer to render to
    if (!context.frameData.mainColorBuffer) {
        spdlog::warn("OutlineRenderPass: No main color buffer available");
        return;
    }

    // Check if shader is available
    if (!m_OutlineShader) {
        spdlog::warn("OutlineRenderPass: No outline shader available");
        return;
    }

    // Begin the pass (no framebuffer binding since we don't have one)
    Begin();

    // Bind the main framebuffer for rendering
    context.frameData.mainColorBuffer->Bind();

    // Set viewport to match main framebuffer
    const auto& mainFBOSpecs = context.frameData.mainColorBuffer->GetSpecification();
    glViewport(0, 0, static_cast<int>(mainFBOSpecs.Width), static_cast<int>(mainFBOSpecs.Height));

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // ===== STENCIL SETUP =====
    // Enable stencil testing
    Submit(RenderCommands::EnableStencilTestData{ true });

    // Configure stencil operation: replace stencil value when tests pass
    Submit(RenderCommands::SetStencilOpData{ GL_KEEP, GL_KEEP, GL_REPLACE });

    // ===== FIRST PASS: Render objects to stencil buffer =====
    RenderFirstPass(context);

    // ===== SECOND PASS: Render scaled outlines =====
    RenderSecondPass(context);

    // ===== CLEANUP =====
    // Reset stencil function to default (always pass)
    Submit(RenderCommands::SetStencilFuncData{ GL_ALWAYS, 0, 0xFF });

    // Reset stencil mask to default
    Submit(RenderCommands::SetStencilMaskData{ 0xFF });

    // Disable stencil testing
    Submit(RenderCommands::EnableStencilTestData{ false });

    // Re-enable depth testing and depth writes
    Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, true });

    // FIX: Restore color mask to default (first pass disables it at line 99)
    Submit(RenderCommands::SetColorMaskData{ true, true, true, true });

    // Ensure face culling is properly set (should already be enabled)
    Submit(RenderCommands::SetFaceCullingData{ true, GL_BACK });

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Unbind the main framebuffer
    context.frameData.mainColorBuffer->Unbind();

    // End the pass
    End();
}

void OutlineRenderPass::RenderFirstPass(RenderContext& context)
{
    // FIRST PASS: Write to stencil buffer only (LearnOpenGL approach)
    // Disable color and depth writes to avoid visual artifacts

    // Disable color writing (only write to stencil buffer)
    Submit(RenderCommands::SetColorMaskData{ false, false, false, false });

    // Disable depth testing (ensures stencil is written regardless of depth)
    Submit(RenderCommands::SetDepthTestData{ false, GL_LESS, false });

    // Configure stencil to always pass and write reference value 1
    Submit(RenderCommands::SetStencilFuncData{ GL_ALWAYS, 1, 0xFF });
    Submit(RenderCommands::SetStencilMaskData{ 0xFF });  // Enable stencil writing

    // Bind simple outline shader once (more reliable than complex material shaders)
    Submit(RenderCommands::BindShaderData{ m_OutlineShader });

    // Render each outlined object to write to stencil buffer
    // Uses simple outline shader to ensure reliable fragment generation
    for (const auto& renderable : context.renderables) {
        // Skip if this object is not in the outlined set
        if (m_OutlinedObjects.count(renderable.objectID) == 0) {
            continue;
        }

        // Skip if not visible
        if (!renderable.visible) {
            continue;
        }

        // Set uniforms (shader already bound above)
        Submit(RenderCommands::SetUniformsData{
            m_OutlineShader,
            renderable.transform,  // Use normal transform (not scaled)
            context.frameData.viewMatrix,
            context.frameData.projectionMatrix,
            context.frameData.cameraPosition
        });

        // Draw the mesh
        Submit(RenderCommands::DrawElementsData{
            renderable.mesh->GetVertexArray()->GetVAOHandle(),
            renderable.mesh->GetIndexCount(),
            GL_TRIANGLES
        });
    }
}

void OutlineRenderPass::RenderSecondPass(RenderContext& context)
{
    // SECOND PASS: Render scaled versions where stencil != 1

    // Re-enable color writing (was disabled in first pass)
    Submit(RenderCommands::SetColorMaskData{ true, true, true, true });

    // Configure stencil to only pass where stencil value != 1
    Submit(RenderCommands::SetStencilFuncData{ GL_NOTEQUAL, 1, 0xFF });
    Submit(RenderCommands::SetStencilMaskData{ 0x00 });  // Disable stencil writing

    // Enable depth testing so outline is occluded by closer objects
    // Use GL_LESS and don't write depth (false) so outline respects existing depth
    Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, false });

    // Enable face culling for proper outline rendering
    Submit(RenderCommands::SetFaceCullingData{ true, GL_BACK });

    // Bind outline shader
    Submit(RenderCommands::BindShaderData{ m_OutlineShader });

    // Set outline color uniform
    Submit(RenderCommands::SetUniformVec3Data{ m_OutlineShader, "u_OutlineColor", m_OutlineColor });

    // Collect all outlined objects first
    std::vector<const RenderableData*> outlinedPtrs;
    outlinedPtrs.reserve(m_OutlinedObjects.size());

    for (const auto& renderable : context.renderables) {
        // Skip if this object is not in the outlined set
        if (m_OutlinedObjects.count(renderable.objectID) == 0) {
            continue;
        }

        // Skip if not visible
        if (!renderable.visible) {
            continue;
        }

        outlinedPtrs.push_back(&renderable);
    }

    // Render each outlined object with scaled transform
    // (Must be done individually since each has a unique scaled transform)
    for (const auto* renderable : outlinedPtrs) {
        // Calculate scaled transform
        glm::mat4 scaledTransform = glm::scale(renderable->transform, glm::vec3(m_OutlineScale));

        // Set uniforms
        Submit(RenderCommands::SetUniformsData{
            m_OutlineShader,
            scaledTransform,
            context.frameData.viewMatrix,
            context.frameData.projectionMatrix,
            context.frameData.cameraPosition
        });

        // Draw the mesh
        Submit(RenderCommands::DrawElementsData{
            renderable->mesh->GetVertexArray()->GetVAOHandle(),
            renderable->mesh->GetIndexCount(),
            GL_TRIANGLES
        });
    }
}
