/******************************************************************************/
/*!
\file   SplashScreen.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration of the SplashScreen class, which
is an IMGUI-based screen displayed during the application startup.
It provides a visual loading indicator while the editor initializes.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SPLASHSCREEN_HPP
#define SPLASHSCREEN_HPP


#include "Screen.hpp"

class SplashScreen : public Screen
{
	float timer = 1.5f; // 5 seconds
	
public:
	SplashScreen(GLFWwindow* window);
	void init() override;
	void update() override;
	void render() override;
	void cleanup() override;

	void Show() override;
	void Close() override;

	virtual bool isWindowClosed() override;
	bool Activate() override;

	~SplashScreen() override = default;

};

#endif // SPLASHSCREEN_HPP