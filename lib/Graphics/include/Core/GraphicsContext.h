#pragma once

#include <cstdint>
#include <glfw/glfw3.h>

class GraphicsContext
{
public:
	GraphicsContext(GLFWwindow* windowHandle);
	~GraphicsContext();

	// All graphics operations
	void Initialize();
	void SwapBuffers();
	void SetVSync(bool enabled);
	void SetClearColor(float r, float g, float b, float a = 1.0f);
	void Clear();

	bool IsInitialized() const { return m_Initialized; }

private:
	GLFWwindow* m_WindowHandle;
	bool m_Initialized = false;
};