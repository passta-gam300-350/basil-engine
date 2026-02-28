/******************************************************************************/
/*!
\file   RenderTextureResolvePass.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/25
\brief  Render texture resolve pass - resolves rendered output for use as a
        runtime texture by HUD/WorldUI components.

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
 * RenderTextureResolvePass - Resolves rendering output into renderTextureCameraBuffer
 *
 * Used by the third render pass (render texture camera) to capture its output
 * into FrameData::renderTextureCameraBuffer. The resulting texture ID is then
 * made available for HUDComponent and WorldUIComponent to display.
 *
 * Mirrors GameResolvePass but targets renderTextureCameraBuffer instead of gameResolvedBuffer.
 */
class RenderTextureResolvePass : public RenderPass
{
public:
    RenderTextureResolvePass();
    ~RenderTextureResolvePass() override;

    void Execute(RenderContext &context) override;

private:
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;
};
