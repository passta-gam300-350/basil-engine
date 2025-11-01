#include "Screens/Screen.hpp"

void Screen::Show()
{
	active = true;
}

void Screen::Close()
{
	active = false;
}
