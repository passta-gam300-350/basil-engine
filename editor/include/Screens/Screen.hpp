/******************************************************************************/
/*!
\file   Screen.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration of the Screen class, which is
an abstract base class for different screens of the editor application.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SCREEN_HPP
#define SCREEN_HPP
#include <cstdint>

struct GLFWwindow;
class Screen {
protected:

	GLFWwindow* window; // Associated window
	int width, height; // Dimensions of the window
	int posx, posy; // Position of the window

	uint32_t WIN_DECORATOR; // Window decorator character

	bool active = true; // Is the screen active and should be rendered?

public:
	Screen() = default;
	Screen(GLFWwindow* window) { this->window = window; }
	virtual ~Screen() = default;
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void cleanup() = 0;
	virtual void Show();
	virtual void Close();


	virtual bool isWindowClosed() = 0;


	virtual bool Activate() = 0;


	



};

#endif // SCREEN_HPP