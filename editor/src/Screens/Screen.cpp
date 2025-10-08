/******************************************************************************/
/*!
\file   Screen.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the implementation of the Screen class, it implements
a minimal interface for different screens in the editor application.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Screens/Screen.hpp"

void Screen::Show()
{
	active = true;
}

void Screen::Close()
{
	active = false;
}
