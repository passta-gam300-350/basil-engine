#include "../../include/Pipeline/RenderPass.h"
#include "../../include/Pipeline/RenderContext.h"

RenderPass::RenderPass(const std::string& name, const FBOSpecs& spec)
	: m_Name(name), m_Framebuffer(std::make_shared<FrameBuffer>(spec)), m_Viewport(0, 0, spec.Width, spec.Height)
{
	// Create pass-isolated command buffer for state isolation
	m_PassCommandBuffer = std::make_unique<RenderCommandBuffer>();
	m_PassCommandBuffer->Initialize();
}

RenderPass::~RenderPass()
{
	// Framebuffer and command buffer will be cleaned up by unique_ptr/shared_ptr
}

void RenderPass::Begin()
{
	// Clear pass command buffer for new frame
	m_PassCommandBuffer->Clear();

	// Bind the framebuffer
	m_Framebuffer->Bind();

	// Apply the viewport
	m_Viewport.Apply();
}

void RenderPass::Execute(RenderContext& context)
{
	Begin();

	// Execute the render function if set
	if (m_RenderFunction)
	{
		m_RenderFunction();
	}

	// Execute all commands submitted to this pass's command buffer
	ExecuteCommands();

	End();
}

void RenderPass::End()
{
	// Unbind the framebuffer
	m_Framebuffer->Unbind();
}

void RenderPass::Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey)
{
	// Submit command to this pass's isolated command buffer
	m_PassCommandBuffer->Submit(command, sortKey);
}

void RenderPass::ExecuteCommands()
{
	// Sort commands for optimal rendering within this pass
	m_PassCommandBuffer->Sort();

	// Execute all commands for this pass
	m_PassCommandBuffer->Execute();
}

void RenderPass::ClearCommands()
{
	// Clear the pass command buffer
	m_PassCommandBuffer->Clear();
}