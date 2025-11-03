#pragma once

#include "RenderPass.h"
#include <memory>

/**
 * HDR Resolve Pass - Resolves MSAA HDR buffer to non-MSAA for tone mapping
 *
 * Takes the MSAA mainColorBuffer (RGB16F) and resolves it to a non-MSAA
 * HDR buffer that can be sampled by the HDR luminance and tone mapping passes.
 *
 * This pass has no framebuffer of its own - it creates a resolved buffer
 * and stores it in the context for subsequent passes.
 */
class HDRResolvePass : public RenderPass
{
public:
    HDRResolvePass();
    ~HDRResolvePass() override = default;

    void Execute(RenderContext& context) override;
};
