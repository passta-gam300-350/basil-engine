/******************************************************************************/
/*!
\file   Window.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the Window class for GLFW window and OpenGL context management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <string>
#include <functional>
struct GLFWwindow;
class Window
{
public:
	Window(const std::string& title = "Engine", uint32_t width = 1280, uint32_t height = 720);
	Window(GLFWwindow*);
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

	void SetCursorEnabled(bool enabled);


	void SwapBuffers();
	void SetVSync(bool enabled);
	void SetTitle(const std::string& title);
	void SetClearColor(float r, float g, float b, float a = 1.0f);
	void Clear();

	bool IsInitialized() const { return m_Initialized; }

	// Resize callback - called whenever window is resized (including during drag operations)
	void SetResizeCallback(std::function<void()> callback) { m_ResizeCallback = callback; }

private:
	GLFWwindow* m_Window;
	uint32_t m_Width, m_Height;
	bool m_Initialized = false;
	bool m_cursorEnabled = true;

	// Callback invoked on window resize to trigger re-rendering
	std::function<void()> m_ResizeCallback;

	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};