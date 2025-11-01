/******************************************************************************/
/*!
\file   MainRenderingPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the MainRenderingPass class for main scene forward rendering with PBR lighting

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"

// Forward declarations
class Renderer;
class InstancedRenderer;
class PBRLightingRenderer;

/**
 * Main Rendering Pass - Executes the main forward rendering
 *
 * Handles the main scene rendering with PBR lighting, clearing, and instanced rendering.
 * Contains the render function logic previously in SceneRenderer's main pass.
 */
class MainRenderingPass : public RenderPass
{
public:
    MainRenderingPass();
	MainRenderingPass(const MainRenderingPass&) = delete;
	MainRenderingPass& operator=(const MainRenderingPass&) = delete;
	MainRenderingPass(MainRenderingPass&&) = delete;
	MainRenderingPass& operator=(MainRenderingPass&&) = delete;
    ~MainRenderingPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
    // Update framebuffer to match current window size
    void UpdateFramebufferSize();

    // Create separate FBO copy for editor display
    void CreateEditorFBOCopy(RenderContext &context);

    static constexpr uint8_t MAIN_PASS_ID = 1;  // Execute after shadow pass (ID 0)
};