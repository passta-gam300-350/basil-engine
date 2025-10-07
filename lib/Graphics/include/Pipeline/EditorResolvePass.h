#pragma once

#include "RenderPass.h"
#include <memory>

/**
 * EditorResolvePass - Resolves MSAA editor FBO to non-MSAA for ImGui
 *
 * Takes the MSAA editorColorBuffer and resolves it to a non-MSAA
 * editorResolvedBuffer that can be sampled by ImGui as a regular texture.
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
