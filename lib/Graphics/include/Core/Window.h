#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <string>

class Window
{
public:
	Window(const std::string& title = "Engine", uint32_t width = 1280, uint32_t height = 720);
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;
	~Window();

	void PollEvents();
	bool ShouldClose() const;
	GLFWwindow* GetNativeWindow() const { return m_Window; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }


	void SwapBuffers();
	void SetVSync(bool enabled);
	void SetClearColor(float r, float g, float b, float a = 1.0f);
	void Clear();

	bool IsInitialized() const { return m_Initialized; }

private:
	GLFWwindow* m_Window;
	uint32_t m_Width, m_Height;
	bool m_Initialized = false;

	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};