/******************************************************************************/
/*!
\file   EditorResolvePass.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Editor resolve pass implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Pipeline/EditorResolvePass.h"
#include "../../include/Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

EditorResolvePass::EditorResolvePass()
    : RenderPass("EditorResolvePass", FBOSpecs{ 1, 1, {{FBOTextureFormat::RGBA8}} })  // Minimal valid spec (unused)
{
    // This pass doesn't render to its own framebuffer
    // It creates a separate editorResolvedBuffer for the editor
    // The 1x1 FBO is never used, just needed to satisfy RenderPass requirements

    // Create fullscreen quad for shader-based gamma correction
    CreateFullScreenQuad();
}

EditorResolvePass::~EditorResolvePass()
{
    if (m_QuadVAO != 0)
    {
        glDeleteVertexArrays(1, &m_QuadVAO);
        m_QuadVAO = 0;
    }
    if (m_QuadVBO != 0)
    {
        glDeleteBuffers(1, &m_QuadVBO);
        m_QuadVBO = 0;
    }
}

void EditorResolvePass::Execute(RenderContext &context)
{
    // Determine which buffer to use for editor display (same logic as PresentPass)
    std::shared_ptr<FrameBuffer> sourceBuffer;

    if (context.frameData.postProcessBuffer)
    {
        // Use tone-mapped LDR result (HDR pipeline active)
        sourceBuffer = context.frameData.postProcessBuffer;
    }
    else if (context.frameData.mainColorBuffer)
    {
        // Use main color buffer directly (no HDR/post-processing)
        sourceBuffer = context.frameData.mainColorBuffer;
    }
    else
    {
        // No buffer available
        return;
    }

    const auto &sourceSpec = sourceBuffer->GetSpecification();

    // Create final editor buffer - keep it simple with same format as source
    // We'll rely on pure glBlitFramebuffer (proven to work without tearing)
    if (!context.frameData.editorResolvedBuffer ||
        context.frameData.editorResolvedBuffer->GetSpecification().Width != sourceSpec.Width ||
        context.frameData.editorResolvedBuffer->GetSpecification().Height != sourceSpec.Height)
    {
        // Match source format exactly (SRGB8 if HDR, RGB16F if no HDR)
        FBOSpecs resolvedSpec = sourceSpec;
        resolvedSpec.Samples = 1;  // Force non-MSAA
        context.frameData.editorResolvedBuffer = std::make_shared<FrameBuffer>(resolvedSpec);

        spdlog::debug("EditorResolvePass: Created editor buffer ({}x{}, same format as source)",
            resolvedSpec.Width, resolvedSpec.Height);
    }

    // Use ONLY glBlitFramebuffer - the proven reliable method
    if (sourceBuffer->IsMultisampled())
    {
        sourceBuffer->ResolveToFramebuffer(context.frameData.editorResolvedBuffer.get());
    }
    else
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceBuffer->GetFBOHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, context.frameData.editorResolvedBuffer->GetFBOHandle());

        glBlitFramebuffer(
            0, 0, sourceSpec.Width, sourceSpec.Height,
            0, 0, sourceSpec.Width, sourceSpec.Height,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Ensure GPU is completely done before releasing to main thread
    // This might help with any residual tearing issues
    glFinish();
}

void EditorResolvePass::CreateFullScreenQuad()
{
    // Full-screen quad in NDC coordinates
    float quadVertices[] = {
        // Positions   // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f   // Top-right
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    // TexCoord attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void *)(2 * sizeof(float)));

    glBindVertexArray(0);

    spdlog::info("EditorResolvePass: Full-screen quad created");
}
