#include "../../include/Pipeline/EditorResolvePass.h"
#include "../../include/Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

EditorResolvePass::EditorResolvePass()
    : RenderPass("EditorResolvePass", FBOSpecs{1, 1, {{FBOTextureFormat::RGBA8}}})  // Minimal valid spec (unused)
{
    // This pass doesn't render to its own framebuffer
    // It performs a resolve blit from mainColorBuffer
    // The 1x1 FBO is never used, just needed to satisfy RenderPass requirements
}

void EditorResolvePass::Execute(RenderContext& context)
{
    // Determine which buffer to use for editor display (same logic as PresentPass)
    std::shared_ptr<FrameBuffer> sourceBuffer;

    if (context.frameData.postProcessBuffer) {
        // Use tone-mapped LDR result (HDR pipeline active)
        sourceBuffer = context.frameData.postProcessBuffer;
    } else if (context.frameData.mainColorBuffer) {
        // Use main color buffer directly (no HDR/post-processing)
        sourceBuffer = context.frameData.mainColorBuffer;
    } else {
        // No buffer available
        return;
    }

    const auto& sourceSpec = sourceBuffer->GetSpecification();

    // Only resolve if the source buffer is actually multisampled
    if (!sourceBuffer->IsMultisampled())
    {
        // Not multisampled, just use it directly for editor
        context.frameData.editorResolvedBuffer = sourceBuffer;
        return;
    }

    // Create or resize resolved buffer if needed
    if (!context.frameData.editorResolvedBuffer ||
        context.frameData.editorResolvedBuffer->GetSpecification().Width != sourceSpec.Width ||
        context.frameData.editorResolvedBuffer->GetSpecification().Height != sourceSpec.Height)
    {
        // Create non-MSAA version with same format as source
        FBOSpecs resolvedSpec = sourceSpec;
        resolvedSpec.Samples = 1;  // Force non-MSAA
        context.frameData.editorResolvedBuffer = std::make_shared<FrameBuffer>(resolvedSpec);

        spdlog::debug("EditorResolvePass: Created resolved buffer ({}x{}, 1x sample)",
            resolvedSpec.Width, resolvedSpec.Height);
    }

    // Resolve MSAA source buffer to non-MSAA resolved buffer
    sourceBuffer->ResolveToFramebuffer(context.frameData.editorResolvedBuffer.get());
}
