#pragma once

#include "GraphicsContext.h"
#include "RenderCommand.h"
#include "RenderQueue.h"
#include <memory>

class Renderer
{
public:
	Renderer(GLFWwindow* windowHandle);
	~Renderer();

	void Initialize();
	void Shutdown();

	void BeginFrame();
	void EndFrame();
	void Submit(const RenderCommand& command);

	static Renderer& Get() { return *s_Instance; }
	GraphicsContext* GetContext() const { return m_Context.get(); }

private:
	std::unique_ptr<GraphicsContext> m_Context;
	RenderQueue m_RenderQueue;
	
	static Renderer* s_Instance;
};