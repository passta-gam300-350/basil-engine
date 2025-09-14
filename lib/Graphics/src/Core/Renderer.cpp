#include "../../include/Core/Renderer.h"

#include <glad/gl.h>

Renderer* Renderer::s_Instance = nullptr;

Renderer::Renderer() : m_RenderQueue()
{
	s_Instance = this;
}

Renderer::~Renderer()
{
	s_Instance = nullptr;
}

void Renderer::Initialize(GLFWwindow* windowHandle)
{
	// Create the graphics context
	m_Context = std::make_unique<GraphicsContext>(windowHandle);
	m_Context->Initialize();
}

void Renderer::Shutdown()
{
	// Commands will be cleaned up by the RenderQueue destructor
	// Context will be cleaned up by its own destructor
}

void Renderer::BeginFrame()
{
	// Clear the render queue
	m_RenderQueue.Clear();

	// Clear the screen
	m_Context->Clear();
}

void Renderer::EndFrame()
{
	// Execute all commands in the render queue
	m_RenderQueue.Execute();
	
	// Swap buffers
	m_Context->SwapBuffers();
}

void Renderer::Submit(const RenderCommand& command)
{
	// Clone the command and submit it to the render queue
	m_RenderQueue.Submit(command);
}