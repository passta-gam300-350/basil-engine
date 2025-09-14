#include "../../include/Pipeline/RenderPass.h"

RenderPass::RenderPass(const std::string& name, const FBOSpecs& spec)
	: m_Name(name), m_Framebuffer(std::make_shared<FrameBuffer>(spec)), m_Viewport(0, 0, spec.Width, spec.Height)
{
}

RenderPass::~RenderPass()
{
	// Framebuffer will be cleaned up by shared_ptr
}

void RenderPass::Begin()
{
	// Bind the framebuffer
	m_Framebuffer->Bind();

	// Apply the viewport
	m_Viewport.Apply();
}

void RenderPass::Execute()
{
	Begin();

	// Execute the render function if set
	if (m_RenderFunction)
	{
		m_RenderFunction();
	}

	// End the render pass
	End();
}

void RenderPass::End()
{
	// Unbind the framebuffer
	m_Framebuffer->Unbind();
}