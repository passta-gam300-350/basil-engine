#pragma once

#include "RenderPass.h"
#include "Resources/Shader.h"
#include <memory>

/**
 * GameResolvePass - Resolves and copies rendering output for Game viewport display
 *
 * Similar to EditorResolvePass, but targets gameResolvedBuffer instead.
 * Uses glBlitFramebuffer for reliable, tear-free copying.
 *
 * Priority order (matches PresentPass logic):
 * 1. postProcessBuffer (if HDR tone mapping is active) - SRGB8
 * 2. mainColorBuffer (if no post-processing) - RGB16F
 */
class GameResolvePass : public RenderPass
{
public:
    GameResolvePass();
    ~GameResolvePass() override;

    void Execute(RenderContext &context) override;

private:
    // Kept for potential future use, but not currently needed
    void CreateFullScreenQuad();
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;
};
