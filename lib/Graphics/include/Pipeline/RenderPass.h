#pragma once

#include "../Buffer/FrameBuffer.h"
#include <memory>
#include <string>
#include <functional>

class RenderPass
{
public:
	using RenderFunction = std::function<void()>;

	RenderPass(const std::string& name, const FBOSpecs& spec);
	virtual ~RenderPass();

	virtual void Begin();
	virtual void Execute();
	virtual void End();

	void SetRenderFunction(const RenderFunction& func) { m_RenderFunction = func; }
	std::shared_ptr<FrameBuffer> GetFramebuffer() const { return m_Framebuffer; }
	const std::string& GetName() const { return m_Name; }
protected:
	std::string m_Name;
	std::shared_ptr<FrameBuffer> m_Framebuffer;
	RenderFunction m_RenderFunction;
};