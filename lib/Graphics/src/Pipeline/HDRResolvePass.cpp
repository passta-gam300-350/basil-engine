#include "../../include/Pipeline/HDRResolvePass.h"
#include "../../include/Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

HDRResolvePass::HDRResolvePass()
    : RenderPass("HDRResolvePass", FBOSpecs{1, 1, {{FBOTextureFormat::RGB16F}}})  // Minimal valid spec (unused)
{
    // This pass doesn't render to its own framebuffer
    // It performs a resolve blit from mainColorBuffer to a dedicated HDR resolve buffer
    // The 1x1 FBO is never used, just needed to satisfy RenderPass requirements
}

void HDRResolvePass::Execute(RenderContext& context)
{
    // Check if we have a main buffer to resolve
    if (!context.frameData.mainColorBuffer)
    {
        // No main buffer yet, skip
        return;
    }

    auto mainFBO = context.frameData.mainColorBuffer;
    const auto& mainSpec = mainFBO->GetSpecification();

    // Only resolve if the main buffer is actually multisampled
    if (!mainFBO->IsMultisampled())
    {
        // Not multisampled, just use it directly for HDR pipeline
        context.hdrTextureID = mainFBO->GetColorAttachmentRendererID(0);
        return;
    }

    // Check if we need to create a new buffer
    bool needsNewBuffer = !context.frameData.hdrResolvedBuffer ||
        context.frameData.hdrResolvedBuffer->GetSpecification().Width != mainSpec.Width ||
        context.frameData.hdrResolvedBuffer->GetSpecification().Height != mainSpec.Height;

    std::shared_ptr<FrameBuffer> targetBuffer;

    if (needsNewBuffer) {
        // Create non-MSAA version with same HDR format as main
        FBOSpecs resolvedSpec = mainSpec;
        resolvedSpec.Samples = 1;  // Force non-MSAA
        targetBuffer = std::make_shared<FrameBuffer>(resolvedSpec);

        spdlog::info("HDRResolvePass: Created HDR resolved buffer ({}x{}, RGB16F, 1x sample)",
            resolvedSpec.Width, resolvedSpec.Height);
    } else {
        // Reuse existing buffer
        targetBuffer = context.frameData.hdrResolvedBuffer;
    }

    // Resolve MSAA main buffer to target buffer
    mainFBO->ResolveToFramebuffer(targetBuffer.get());

    // CRITICAL: Update pointer and texture ID AFTER blit is complete
    // This prevents downstream passes (luminance, tone mapping) from reading empty buffers
    if (needsNewBuffer) {
        context.frameData.hdrResolvedBuffer = targetBuffer;
    }

    // Store resolved HDR texture ID - read from filled targetBuffer, not potentially-old context buffer
    context.hdrTextureID = targetBuffer->GetColorAttachmentRendererID(0);

    // Validate texture ID
    if (context.hdrTextureID == 0) {
        spdlog::error("HDRResolvePass: Failed to get valid texture ID from resolved buffer!");
    }
}
