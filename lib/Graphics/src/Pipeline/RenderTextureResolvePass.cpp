/******************************************************************************/
/*!
\file   RenderTextureResolvePass.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/25
\brief  Render texture resolve pass implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Pipeline/RenderTextureResolvePass.h"
#include "../../include/Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

RenderTextureResolvePass::RenderTextureResolvePass()
    : RenderPass("RenderTextureResolvePass", FBOSpecs{ 1, 1, {{FBOTextureFormat::RGBA8}} })
{
    // This pass does not render to its own framebuffer.
    // It creates/resizes renderTextureCameraBuffer in FrameData each frame.
}

RenderTextureResolvePass::~RenderTextureResolvePass()
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

void RenderTextureResolvePass::Execute(RenderContext &context)
{
    // Determine source buffer (prefer tone-mapped LDR output)
    std::shared_ptr<FrameBuffer> sourceBuffer;

    if (context.frameData.postProcessBuffer)
    {
        sourceBuffer = context.frameData.postProcessBuffer;
    }
    else if (context.frameData.mainColorBuffer)
    {
        sourceBuffer = context.frameData.mainColorBuffer;
    }
    else
    {
        return;
    }

    const auto &sourceSpec = sourceBuffer->GetSpecification();

    // Create or resize the render texture camera buffer to match the source
    if (!context.frameData.renderTextureCameraBuffer ||
        context.frameData.renderTextureCameraBuffer->GetSpecification().Width != sourceSpec.Width ||
        context.frameData.renderTextureCameraBuffer->GetSpecification().Height != sourceSpec.Height)
    {
        FBOSpecs resolvedSpec = sourceSpec;
        resolvedSpec.Samples = 1;  // Force non-MSAA so it can be sampled as a texture
        context.frameData.renderTextureCameraBuffer = std::make_shared<FrameBuffer>(resolvedSpec);

        spdlog::debug("RenderTextureResolvePass: Created render texture buffer ({}x{})",
            resolvedSpec.Width, resolvedSpec.Height);
    }

    // Copy source into renderTextureCameraBuffer via glBlitFramebuffer
    if (sourceBuffer->IsMultisampled())
    {
        sourceBuffer->ResolveToFramebuffer(context.frameData.renderTextureCameraBuffer.get());
    }
    else
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceBuffer->GetFBOHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, context.frameData.renderTextureCameraBuffer->GetFBOHandle());

        glBlitFramebuffer(
            0, 0, sourceSpec.Width, sourceSpec.Height,
            0, 0, sourceSpec.Width, sourceSpec.Height,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glFinish();
}
