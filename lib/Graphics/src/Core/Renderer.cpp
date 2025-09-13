#include "../../include/Core/Renderer.h"
#include "../../include/Core/RenderCommandBuffer.h"

#include <glad/gl.h>

Renderer::Renderer() : m_CommandBuffer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Initialize(GLFWwindow* windowHandle)
{
	// Create the graphics context
	m_Context = std::make_unique<GraphicsContext>(windowHandle);
	m_Context->Initialize();

	// Initialize command buffer (after OpenGL context is ready)
	m_CommandBuffer.Initialize();
}

void Renderer::Shutdown()
{
	// Commands will be cleaned up by the RenderQueue destructor
	// Context will be cleaned up by its own destructor
}

void Renderer::BeginFrame()
{
	// Clear the command buffer
	m_CommandBuffer.Clear();

	// Clear the screen
	m_Context->Clear();
}

void Renderer::EndFrame()
{
	// Sort and execute command buffer for optimal performance
	m_CommandBuffer.Sort();
	m_CommandBuffer.Execute();
	
	// Swap buffers
	m_Context->SwapBuffers();
}

void Renderer::Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey)
{
	// Submit to efficient command buffer
	m_CommandBuffer.Submit(command, sortKey);
}

void Renderer::SortCommands()
{
	// Explicitly sort commands for optimal rendering
	m_CommandBuffer.Sort();
}