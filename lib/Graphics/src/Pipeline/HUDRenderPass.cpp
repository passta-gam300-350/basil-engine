/******************************************************************************/
/*!
\file   HUDRenderPass.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  HUD render pass implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Pipeline/HUDRenderPass.h"
#include "Pipeline/RenderContext.h"
#include "Rendering/HUDRenderer.h"
#include "Rendering/TextRenderer.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

HUDRenderPass::HUDRenderPass()
    : RenderPass("HUDPass")  // No framebuffer - renders to whatever is currently bound
{
    spdlog::info("HUDRenderPass created");
}

HUDRenderPass::HUDRenderPass(const std::shared_ptr<Shader>& hudShader)
    : RenderPass("HUDPass")
    , m_HUDShader(hudShader)
{
    spdlog::info("HUDRenderPass created with shader");
}

void HUDRenderPass::Execute(RenderContext& context)
{
    //spdlog::info("HUDRenderPass::Execute() - called");

    if (!m_HUDShader) {
        spdlog::error("HUDRenderPass::Execute() - No HUD shader set!");
        return;
    }

    // Check if there are any HUD or text elements to render
    uint32_t hudElementCount = context.hudRenderer.GetElementCount();
    uint32_t textGlyphCount = context.textRenderer.GetGlyphCount();
    //spdlog::info("HUDRenderPass::Execute() - HUD element count: {}, text glyph count: {}", hudElementCount, textGlyphCount);

    if (hudElementCount == 0 && textGlyphCount == 0) {
        //spdlog::warn("HUDRenderPass::Execute() - No HUD or text elements, skipping");
        return;  // No HUD or text elements, skip rendering
    }

    // Setup GL state for HUD rendering
    glDisable(GL_DEPTH_TEST);       // HUD always on top of 3D scene
    glDepthMask(GL_FALSE);          // Don't write to depth buffer
    glEnable(GL_BLEND);             // Enable alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);        // Render both sides

    // Enable scissor test to clip HUD to viewport bounds
    // HUD renders at fixed reference resolution (e.g., 1920x1080)
    // Scissor ensures only the visible portion is drawn when viewport is smaller
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, context.frameData.viewportWidth, context.frameData.viewportHeight);

    // Bind appropriate render target
    // HUD should render to the same buffer that will be displayed (postProcessBuffer after tone mapping)
    if (m_RenderToScreen) {
        //spdlog::info("HUDRenderPass::Execute() - rendering to screen (FBO 0)");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Render to screen
    } else {
        // Render to postProcessBuffer (output of tone mapping)
        if (context.frameData.postProcessBuffer) {
            uint32_t fboHandle = context.frameData.postProcessBuffer->GetFBOHandle();
            //spdlog::info("HUDRenderPass::Execute() - binding postProcessBuffer FBO: {}", fboHandle);
            glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

            // Set viewport to match buffer size
            const auto& spec = context.frameData.postProcessBuffer->GetSpecification();
            glViewport(0, 0, spec.Width, spec.Height);
        } else {
            spdlog::warn("HUDRenderPass::Execute() - no postProcessBuffer available!");
            return;
        }
    }

    // Clear command buffer from previous frame (CRITICAL!)
    Begin();

    // Setup command buffer
    SetupCommandBuffer(context);

    //spdlog::info("HUDRenderPass::Execute() - calling HUD renderer");

    // Render HUD elements via the HUD renderer
    context.hudRenderer.RenderToPass(*this, context.frameData);

    //spdlog::info("HUDRenderPass::Execute() - calling Text renderer");

    // Render text elements via the Text renderer (on top of HUD)
    context.textRenderer.RenderToPass(*this, context.frameData);

    //spdlog::info("HUDRenderPass::Execute() - executing commands");

    // Execute commands
    ExecuteCommands();

    End();
    //spdlog::info("HUDRenderPass::Execute() - commands executed");

    // Restore GL state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);  // Restore scissor test state
}

void HUDRenderPass::SetHUDShader(const std::shared_ptr<Shader>& shader)
{
    m_HUDShader = shader;
    spdlog::info("HUDRenderPass: Shader set");
}
