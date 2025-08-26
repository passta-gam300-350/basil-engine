#pragma once

#include "GraphicsContext.h"
#include "RenderCommandBuffer.h"
#include <memory>

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Initialize(GLFWwindow* windowHandle);
	void Shutdown();

	void BeginFrame();
	void EndFrame();
	
	// Modern command buffer API
	void Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey = {});
	void SortCommands();  // Sort commands for optimal rendering

	static Renderer& Get() { return *s_Instance; }
	GraphicsContext* GetContext() const { return m_Context.get(); }

private:
	std::unique_ptr<GraphicsContext> m_Context;
	RenderCommandBuffer m_CommandBuffer; // Modern efficient command buffer
	
	static Renderer* s_Instance;
};