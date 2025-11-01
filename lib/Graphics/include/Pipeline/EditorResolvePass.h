#pragma once

#include "RenderPass.h"
#include <memory>

/**
 * EditorResolvePass - Resolves MSAA editor FBO to non-MSAA for ImGui
 *
 * Resolves the SAME buffer that PresentPass displays to screen, ensuring
 * the editor viewport shows identical content (with tone mapping if enabled).
 *
 * Priority order (matches PresentPass logic):
 * 1. postProcessBuffer (if HDR tone mapping is active) - SRGB8 LDR
 * 2. mainColorBuffer (if no post-processing) - RGB16F HDR
 *
 * This pass has no framebuffer of its own - it performs a blit operation
 * between two existing framebuffers.
 */
class EditorResolvePass : public RenderPass
{
public:
    EditorResolvePass();
    ~EditorResolvePass() override = default;

    void Execute(RenderContext& context) override;
};
