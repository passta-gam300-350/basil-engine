#include "../../include/Core/Window.h"
#include <glad/gl.h>
#include <iostream>

Window::Window(const std::string& title, uint32_t width, uint32_t height)
	: m_Width(width), m_Height(height)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create the GLFW window
	m_Window = glfwCreateWindow(m_Width, m_Height, title.c_str(), nullptr, nullptr);
	if (!m_Window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(m_Window);

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

void Window::SwapBuffers()
{
	glfwSwapBuffers(m_Window);
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Update viewport when window is resized
	glViewport(0, 0, width, height);

	// Update stored dimensions
	Window* windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	windowInstance->m_Width = width;
	windowInstance->m_Height = height;
}