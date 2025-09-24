#include "../../include/Pipeline/RenderPass.h"
#include "../../include/Pipeline/RenderContext.h"

RenderPass::RenderPass(const std::string& name, const FBOSpecs& spec)
	: m_Name(name), m_Framebuffer(std::make_shared<FrameBuffer>(spec)), m_Viewport(0, 0, spec.Width, spec.Height)
{
	// Create pass-isolated command buffer for state isolation
	m_PassCommandBuffer = std::make_unique<RenderCommandBuffer>();
}

void RenderPass::SetupCommandBuffer(RenderContext& context)
{
	// Setup the command buffer's TextureSlotManager from the context
	m_PassCommandBuffer->SetTextureSlotManager(&context.textureSlotManager);
}

void RenderPass::Begin()
{
	// Clear pass command buffer for new frame
	ClearCommands();

	// Bind the framebuffer
	m_Framebuffer->Bind();

	// Apply the viewport
	m_Viewport.Apply();
}

void RenderPass::End()
{
	// Unbind the framebuffer
	m_Framebuffer->Unbind();
}

void RenderPass::Submit(const VariantRenderCommand& command)
{
	// Submit command to this pass's isolated command buffer
	m_PassCommandBuffer->Submit(command);
}

void RenderPass::ExecuteCommands() const
{
	// Execute all commands for this pass
	m_PassCommandBuffer->Execute();
}

void RenderPass::ClearCommands()
{
	// Clear the pass command buffer
	m_PassCommandBuffer->Clear();
}