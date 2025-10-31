/******************************************************************************/
/*!
\file   PresentPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the PresentPass class for blitting the render target to screen

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"

/**
 * Present Pass - Blits main render target to screen
 *
 * Final pass in the rendering pipeline that copies the main color buffer
 * to the screen framebuffer (FBO 0). Uses command buffer for proper
 * state management and no raw OpenGL calls.
 *
 * Uses pass ID 3 to ensure execution after debug rendering pass (pass ID 2).
 */
class PresentPass : public RenderPass {
public:
    PresentPass();
	PresentPass(const PresentPass&) = delete;
	PresentPass& operator=(const PresentPass&) = delete;
	PresentPass(PresentPass&&) = delete;
	PresentPass& operator=(PresentPass&&) = delete;
    ~PresentPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

private:
};