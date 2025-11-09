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





