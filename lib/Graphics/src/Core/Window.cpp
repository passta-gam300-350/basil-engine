#include "../../include/Core/Window.h"
#include <iostream>

Window::Window(const std::string& title, uint32_t width, uint32_t height)
	: m_Width(width), m_Height(height), m_Window(nullptr)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Create the GLFW window
	m_Window = glfwCreateWindow(m_Width, m_Height, title.c_str(), nullptr, nullptr);
	if (!m_Window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	// Note: OpenGL context creation moved to GraphicsContext
	// glfwMakeContextCurrent is now handled by GraphicsContext

	// Set up callbacks
	glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
	glfwSetWindowUserPointer(m_Window, this);
}

Window::~Window()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Window::PollEvents()
{
	glfwPollEvents();
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Update stored dimensions (OpenGL viewport update moved to GraphicsContext)
	Window* windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (windowInstance) {
		windowInstance->m_Width = width;
		windowInstance->m_Height = height;
	}
	
	// Note: glViewport call moved to GraphicsContext::OnWindowResize()
	// Window class should not make OpenGL calls
}