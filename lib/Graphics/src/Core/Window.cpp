/******************************************************************************/
/*!
\file   Window.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of window management using GLFW and OpenGL initialization

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Core/Window.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

Window::Window(const std::string& title, uint32_t width, uint32_t height)
	: m_Width(width), m_Height(height), m_Window(nullptr)
{
	if (glfwInit() == GLFW_FALSE)
	{
		spdlog::error("Failed to initialize GLFW");
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Create the GLFW window
	m_Window = glfwCreateWindow(static_cast<int>(m_Width), static_cast<int>(m_Height), title.c_str(), nullptr, nullptr);
	if (m_Window == nullptr)
	{
		spdlog::error("Failed to create GLFW window");
		glfwTerminate();
		return;
	}

	// Make this window's context current (moved from Window class)
	glfwMakeContextCurrent(m_Window);

	if (gladLoadGL() == 0)
	{
		spdlog::error("Failed to initialize GLAD2");
		return;
	}

	// Print OpenGL info
	spdlog::info("OpenGL Vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	spdlog::info("OpenGL Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	spdlog::info("OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	spdlog::info("GLSL Version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

	// Set default OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable sRGB framebuffer for automatic linear-to-sRGB conversion
	glEnable(GL_FRAMEBUFFER_SRGB);
	spdlog::info("sRGB framebuffer enabled for linear color pipeline");

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
	return glfwWindowShouldClose(m_Window) != 0;
}

void Window::SwapBuffers()
{
	glfwSwapBuffers(m_Window);
}

void Window::SetVSync(bool enabled)
{
	glfwSwapInterval(enabled ? 1 : 0);
}

void Window::SetTitle(const std::string& title)
{
	glfwSetWindowTitle(m_Window, title.c_str());
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