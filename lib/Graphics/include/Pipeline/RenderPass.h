#pragma once

#include "../Buffer/FrameBuffer.h"
#include "../Core/RenderCommandBuffer.h"
#include "../Utility/Viewport.h"
#include <memory>
#include <string>

// Forward declaration
struct RenderContext;

class RenderPass
{
public:
	RenderPass(const std::string& name, const FBOSpecs& spec);
	virtual ~RenderPass() = default;

	void Begin();
	virtual void Execute(RenderContext& context) = 0;
	void End();

	std::shared_ptr<FrameBuffer> GetFramebuffer() const { return m_Framebuffer; }
	const std::string& GetName() const { return m_Name; }

	// Viewport management
	void SetViewport(const Viewport& viewport) { m_Viewport = viewport; }
	const Viewport& GetViewport() const { return m_Viewport; }

	// Pass-isolated command buffer API
	void Submit(const VariantRenderCommand& command);
	void ExecuteCommands() const;
	void ClearCommands();

protected:
	std::string m_Name;
	std::shared_ptr<FrameBuffer> m_Framebuffer;
	Viewport m_Viewport;

	// Pass-isolated command buffer for state isolation
	std::unique_ptr<RenderCommandBuffer> m_PassCommandBuffer;
};