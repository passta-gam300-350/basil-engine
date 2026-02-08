/******************************************************************************/
/*!
\file   EditorResolvePass.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Editor resolve render pass

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include "Resources/Shader.h"
#include <memory>

/**
 * EditorResolvePass - Resolves and copies rendering output for ImGui display
 *
 * Uses glBlitFramebuffer for reliable, tear-free copying. The main editor thread
 * has GL_FRAMEBUFFER_SRGB enabled, which correctly handles gamma when displaying
 * SRGB8 textures from the tone mapping pass.
 *
 * Priority order (matches PresentPass logic):
 * 1. postProcessBuffer (if HDR tone mapping is active) - SRGB8
 * 2. mainColorBuffer (if no post-processing) - RGB16F
 */
class EditorResolvePass : public RenderPass
{
public:
    EditorResolvePass();
    ~EditorResolvePass() override;

    void Execute(RenderContext &context) override;

private:
    // Kept for potential future use, but not currently needed
    void CreateFullScreenQuad();
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;
};