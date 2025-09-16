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
	// NOTE: Commands are now executed by individual passes for better state isolation
	// Global command buffer is kept for backward compatibility but not automatically executed
	// If you need to execute global commands, call ExecuteGlobalCommands() manually

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

void Renderer::ExecuteGlobalCommands()
{
	// Sort and execute global command buffer (for legacy/direct rendering)
	m_CommandBuffer.Sort();
	m_CommandBuffer.Execute();
	m_CommandBuffer.Clear();
}