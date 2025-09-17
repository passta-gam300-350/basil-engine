#include "../../include/Core/Window.h"
#include <glad/gl.h>
#include <iostream>

Window::Window(const std::string& title, uint32_t width, uint32_t height)
	: m_Width(width), m_Height(height), m_Window(nullptr)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW\n";
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
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return;
	}

	// Make this window's context current (moved from Window class)
	glfwMakeContextCurrent(m_Window);

	if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD2\n";
		return;
	}

	// Print OpenGL info
	std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << '\n';
	std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << '\n';
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << '\n';
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

	// Set default OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_Initialized = true;

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

void Window::SwapBuffers()
{
	glfwSwapBuffers(m_Window);
}

void Window::SetVSync(bool enabled)
{
	glfwSwapInterval(enabled ? 1 : 0);
}

void Window::SetClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void Window::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Update stored dimensions and viewport
	if (Window* windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(window)))
	{
		windowInstance->m_Width = width;
		windowInstance->m_Height = height;
		glViewport(0, 0, width, height);
	}
}