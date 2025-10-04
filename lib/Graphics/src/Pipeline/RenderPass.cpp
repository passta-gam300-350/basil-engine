/******************************************************************************/
/*!
\file   RenderPass.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of the render pass base class for managing framebuffer
          rendering, command buffer execution, and viewport configuration

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Pipeline/RenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include <cassert>

RenderPass::RenderPass(const std::string& name, const FBOSpecs& spec)
	: m_Name(name), m_Framebuffer(std::make_shared<FrameBuffer>(spec)), m_Viewport(0, 0, spec.Width, spec.Height)
{
	assert(!name.empty() && "RenderPass name cannot be empty");
	assert(spec.Width > 0 && spec.Height > 0 && "RenderPass framebuffer dimensions must be positive");
	assert(!spec.Attachments.Attachments.empty() && "RenderPass framebuffer must have at least one attachment");

	// Create pass-isolated command buffer for state isolation
	m_PassCommandBuffer = std::make_unique<RenderCommandBuffer>();
	assert(m_PassCommandBuffer && "Failed to create RenderCommandBuffer");
}

RenderPass::RenderPass(const std::string& name)
	: m_Name(name), m_Framebuffer(nullptr), m_Viewport(0, 0, 0, 0)
{
	assert(!name.empty() && "RenderPass name cannot be empty");

	// Create pass-isolated command buffer for state isolation
	m_PassCommandBuffer = std::make_unique<RenderCommandBuffer>();
	assert(m_PassCommandBuffer && "Failed to create RenderCommandBuffer");
}

void RenderPass::SetupCommandBuffer(RenderContext& context)
{
	assert(m_PassCommandBuffer && "Command buffer must be valid before setup");

	// Setup the command buffer's TextureSlotManager from the context
	m_PassCommandBuffer->SetTextureSlotManager(&context.textureSlotManager);
}

void RenderPass::Begin()
{
	// Clear pass command buffer for new frame
	ClearCommands();

	// Bind the framebuffer if it exists
	if (m_Framebuffer) {
		m_Framebuffer->Bind();
		// Apply the viewport
		m_Viewport.Apply();
	}
}

void RenderPass::End()
{
	// Unbind the framebuffer if it exists
	if (m_Framebuffer) {
		m_Framebuffer->Unbind();
	}
}

void RenderPass::Submit(const VariantRenderCommand& command)
{
	assert(m_PassCommandBuffer && "Command buffer must be valid before submitting commands");

	// Submit command to this pass's isolated command buffer
	m_PassCommandBuffer->Submit(command);
}

void RenderPass::ExecuteCommands() const
{
	assert(m_PassCommandBuffer && "Command buffer must be valid before executing commands");

	// Execute all commands for this pass
	m_PassCommandBuffer->Execute();
}

void RenderPass::ClearCommands()
{
	assert(m_PassCommandBuffer && "Command buffer must be valid before clearing commands");

	// Clear the pass command buffer
	m_PassCommandBuffer->Clear();
}