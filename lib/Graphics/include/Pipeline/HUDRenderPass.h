/******************************************************************************/
/*!
\file   HUDRenderPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/06
\brief    Declares the HUDRenderPass class for rendering screen-space UI/HUD elements

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include "../Resources/Shader.h"
#include <memory>

// Forward declarations
class HUDRenderer;

/**
 * @brief HUD Rendering Pass
 *
 * Renders screen-space UI/HUD elements on top of the final scene.
 * Executes after tone mapping to overlay on the final LDR image.
 *
 * GL State:
 * - Depth test: DISABLED (HUD always on top)
 * - Depth write: DISABLED
 * - Blending: ENABLED (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
 * - Culling: DISABLED
 */
class HUDRenderPass : public RenderPass
{
public:
    HUDRenderPass();
    explicit HUDRenderPass(const std::shared_ptr<Shader>& hudShader);
    HUDRenderPass(const HUDRenderPass&) = delete;
    HUDRenderPass& operator=(const HUDRenderPass&) = delete;
    HUDRenderPass(HUDRenderPass&&) = delete;
    HUDRenderPass& operator=(HUDRenderPass&&) = delete;
    ~HUDRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Shader configuration
    void SetHUDShader(const std::shared_ptr<Shader>& shader);

    // Render target configuration (optional - can render to screen or framebuffer)
    void SetRenderToScreen(bool renderToScreen) { m_RenderToScreen = renderToScreen; }
    bool IsRenderingToScreen() const { return m_RenderToScreen; }

private:
    std::shared_ptr<Shader> m_HUDShader;
    bool m_RenderToScreen = false;  // If true, renders to default framebuffer (0)
};
