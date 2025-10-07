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
        // Not multisampled, just use it directly for editor
        context.frameData.editorResolvedBuffer = mainFBO;
        return;
    }

    // Create or resize resolved buffer if needed
    if (!context.frameData.editorResolvedBuffer ||
        context.frameData.editorResolvedBuffer->GetSpecification().Width != mainSpec.Width ||
        context.frameData.editorResolvedBuffer->GetSpecification().Height != mainSpec.Height)
    {
        // Create non-MSAA version with same format as main
        FBOSpecs resolvedSpec = mainSpec;
        resolvedSpec.Samples = 1;  // Force non-MSAA
        context.frameData.editorResolvedBuffer = std::make_shared<FrameBuffer>(resolvedSpec);

        spdlog::debug("EditorResolvePass: Created resolved buffer ({}x{}, 1x sample)",
            resolvedSpec.Width, resolvedSpec.Height);
    }

    // Resolve MSAA main buffer to non-MSAA resolved buffer
    mainFBO->ResolveToFramebuffer(context.frameData.editorResolvedBuffer.get());
}
