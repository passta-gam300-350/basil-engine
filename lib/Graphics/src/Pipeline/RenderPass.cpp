#include "../../include/Pipeline/RenderPass.h"

RenderPass::RenderPass(const std::string& name, const FramebufferSpecification& spec)
	: m_Name(name), m_Framebuffer(std::make_shared<Framebuffer>(spec))
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