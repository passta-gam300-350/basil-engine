#pragma once
#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <string>

class Window
{
public:
	Window(const std::string& title = "Engine", uint32_t width = 1280, uint32_t height = 720);
	~Window();

	void PollEvents();
	bool ShouldClose() const;
	GLFWwindow* GetNativeWindow() const { return m_Window; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

private:
	GLFWwindow* m_Window;
	uint32_t m_Width, m_Height;

	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};