
/******************************************************************************/
/*!
\file   ManagedInput.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedInput class, which
is responsible for managing and getting various input-related functionalities in the
managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Input/InputManager.h"
#include "Bindings/ManagedInput.hpp"

#include <atomic>

#include "Core/Window.h"
#include "Engine.hpp"

//#include "Core/Window.h"
//#include "Engine.hpp"

namespace {
	std::atomic<bool> s_MouseOverrideEnabled{ false };
	std::atomic<float> s_MouseOverrideX{ 0.0f };
	std::atomic<float> s_MouseOverrideY{ 0.0f };
}

bool ManagedInput::GetKey(int keycode)
{
	return InputManager::Get_Instance()->Is_KeyPressed(keycode);
}
bool ManagedInput::GetKeyDown(int keycode)
{
	if (keycode <= 3)
	{
		return GetMouse(keycode);
	}

	return InputManager::Get_Instance()->Is_KeyPressed(keycode);
}

bool ManagedInput::GetKeyUp(int keycode)
{
	return InputManager::Get_Instance()->Is_KeyReleased(keycode);

}

bool ManagedInput::GetKeyPress(int keycode)
{

	if (keycode <= 3)
	{
		return GetMousePress(keycode);
	}

	return InputManager::Get_Instance()->Is_KeyTriggered(keycode);
}

void ManagedInput::GetMousePosition(float* xp, float* yp)
{
	if (s_MouseOverrideEnabled.load())
	{
		if (xp) *xp = s_MouseOverrideX.load();
		if (yp) *yp = s_MouseOverrideY.load();
		return;
	}

	double x, y;
	InputManager::Get_Instance()->Get_MousePosition(x,y);
	if (xp) *xp = static_cast<float>(x);
	if (yp) *yp = static_cast<float>(y);
}

void ManagedInput::SetMouseOverride(float x, float y, bool enabled)
{
	s_MouseOverrideEnabled.store(enabled);
	if (enabled) {
		s_MouseOverrideX.store(x);
		s_MouseOverrideY.store(y);
	}
}


bool ManagedInput::GetMouse(int mousecode)
{
	return InputManager::Get_Instance()->Is_MousePressed(mousecode);
}
bool ManagedInput::GetMousePress(int mousecode)
{
	static bool pressed = false;
	if (InputManager::Get_Instance()->Is_MousePressed(mousecode))
	{
		if (!pressed)
		{
			pressed = true;
			return true;
		}
	}
	else
	{
		pressed = false;
	}

	return false;
}

void ManagedInput::LockCursor(bool locked)
{
	Engine::GetWindowInstance().SetCursorEnabled(!locked);
}


