/******************************************************************************/
/*!
\file   ManagedScreen.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief  Native bindings for querying screen dimensions from managed code.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedScreen.hpp"
#include "Engine.hpp"
#include "Core/Window.h"
#include "Input/InputGraphics_Helper.h"
#include <GLFW/glfw3.h>

#include "Render/Camera.h"

int ManagedScreen::GetWidth()
{
	// Prefer cached viewport from camera system if set
	{
		auto vp = CameraSystem::GetCachedViewport();
		if (vp.x > 0 && vp.y > 0) return static_cast<int>(vp.x);
	}

	// Prefer engine window dimensions if available
	try {
		if (Engine::GetWindowInstance().GetNativeWindow())
		{
			return static_cast<int>(Engine::GetWindowInstance().GetWidth());
		}
	}
	catch (...) {}

	// Fallback to GLFW framebuffer (if GLHelper has a window)
	if (GLHelper::ptr_window)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(GLHelper::ptr_window, &w, &h);
		if (w > 0)
			return w;
	}

	return GLHelper::width;
}

int ManagedScreen::GetHeight()
{
	{
		auto vp = CameraSystem::GetCachedViewport();
		if (vp.x > 0 && vp.y > 0) return static_cast<int>(vp.y);
	}

	try {
		if (Engine::GetWindowInstance().GetNativeWindow())
		{
			return static_cast<int>(Engine::GetWindowInstance().GetHeight());
		}
	}
	catch (...) {}

	if (GLHelper::ptr_window)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(GLHelper::ptr_window, &w, &h);
		if (h > 0)
			return h;
	}

	return GLHelper::height;
}
