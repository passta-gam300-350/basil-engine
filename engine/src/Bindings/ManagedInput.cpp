
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
#include "Bindings/ManagedInput.hpp"
#include "Input/InputManager.h"

bool ManagedInput::GetKey(int keycode)
{
	return InputManager::Get_Instance()->Is_KeyPressed(keycode);
}
bool ManagedInput::GetKeyDown(int keycode)
{
	return InputManager::Get_Instance()->Is_KeyPressed(keycode);
}

bool ManagedInput::GetKeyUp(int keycode)
{
	return InputManager::Get_Instance()->Is_KeyReleased(keycode);

}

bool ManagedInput::GetKeyPress(int keycode)
{
	static bool pressed = false;
	if (InputManager::Get_Instance()->Is_KeyPressed(keycode))
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





