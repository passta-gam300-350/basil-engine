#include "Screens/SplashScreen.hpp"
#include "glad/glad.h"
#include "GLFW/glfw3.h"


SplashScreen::SplashScreen(GLFWwindow* _window) : Screen(_window)
{
	
}

void SplashScreen::init()
{
	// Create a windowed mode window and its OpenGL context
	//window = glfwCreateWindow(800, 600, "Splash Screen", NULL, NULL);
	// Set decorator to false

	// Remove window borders and title bar
	//glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);

}


void SplashScreen::render()
{
	
}


void SplashScreen::cleanup()
{
	
}


void SplashScreen::Show()
{
	active = true;
}

void SplashScreen::Close()
{
	active = false;
}

bool SplashScreen::isWindowClosed()
{
	return glfwWindowShouldClose(window);
}

bool SplashScreen::Activate()
{
	return active;
}
