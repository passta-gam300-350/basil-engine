/*!************************************************************************
\file:      InputGraphics_Helper.h
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file contains the declaration of namespace GLHelper that encapsulates the functionality required to create an OpenGL context using GLFW and GLEW to load OpenGL extensions,
			initialise OpenGL states and finally initialise the OpenGL application by calling initalisation functions associated with objects participating in the application.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/
#ifndef GLHELPER_H
#define GLHELPER_H

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

/**
* @struct GLHelper
* @brief A struct that contains useful variables and functions.
*
* This struct provides variables and functions that may be applied throughout the
* entire Graphics folder.
*/
struct GLHelper
{
	static GLint width, height;     // Last known resolution size of the window in windowed mode
	static GLint windowX, windowY;  // Last known position of the window's top-left corner on the screen in windowed mode
	static GLdouble fps;
	static GLdouble delta_time;     // Time taken to complete most recent game loop
	static std::string title;
	static GLFWwindow* ptr_window;
	static GLFWmonitor* monitor;
	static const GLFWvidmode* mode;

	/**
	* @brief Initialises the OpenGL context and creates a window.
	*
	* This function sets up an OpenGL context using GLFW and configures the window with the specified dimensions and title.
	* The window is created with a core profile that supports OpenGL 4.5 and disables support for older versions of OpenGL.
	* The buffers include a 32-bit RGBA color buffer, 24-bit depth buffer, 8-bit stencil buffer, and are double-buffered.
    *
	* @param[in] widthInit The width of the window in pixels.
	* @param[in] heightInit The height of the window in pixels.
	* @param[in] titleInit A string to be displayed on the window's title bar.
	*/
	static bool Initialise_OpenGL(GLint widthInit, GLint heightInit, std::string titleInit);

	/**
	* @brief Updates the OpenGL state and prepares it for rendering.
	*
	* This function ensures that the OpenGL state is updated as required for rendering.
	* It is typically called once per frame to handle any state transitions, updates to uniforms, and preparation steps necessary for the current frame's rendering operations.
	*/
	static void Update_OpenGL();

	/**
	* @brief Updates the screen mode between fullscreen and windowed mode.
	*
	* This function transitions the window between fullscreen and windowed modes.
	* When switching to fullscreen, it saves the current windowed position and size for later restoration.
	* When switching to windowed mode, it restores the window to its previously saved position and size.
	* The function ensures that the OpenGL context remains current after the transition through glfwMakeContextCurrent.
	*
	* @param[in] isFullscreen A boolean indicating whether to switch to fullscreen mode (true) or windowed mode (false).
	* @param[in,out] windowedX A reference to an integer storing the x-coordinate of the window's position in windowed mode. Updated when transitioning to fullscreen mode.
	* @param[in,out] windowedY A reference to an integer storing the y-coordinate of the window's position in windowed mode. Updated when transitioning to fullscreen mode.
	* @param[in,out] windowedWidth A reference to an integer storing the width of the window in windowed mode. Updated when transitioning to fullscreen mode.
	* @param[in,out] windowedHeight A reference to an integer storing the height of the window in windowed mode. Updated when transitioning to fullscreen mode.
	*
	* @details When switching to fullscreen mode, the window takes the primary monitor's resolution and refresh rate.
	*		   When switching back to windowed mode, the stored window position and size are restored, and the window decorations are re-enabled.
	*
	* @note The OpenGL context is preserved after the transition, and the window is restored from a minimised or hidden state as necessary.
	*		glfwRestoreWindow is a no-op because the window context is already in the normal (non-minimised, non-maximised) state after changing from fullscreen mode.
	*/
	static void Update_Screen_Mode(bool isFullscreen, GLint& windowedX, GLint& windowedY, GLint& windowedWidth, GLint& windowedHeight);

	/**
	* @brief Cleans up and terminates GLFW.
	*
	* This function handles the cleanup of resources allocated by GLFW and gracefully terminates the program.
	* It is used to release any system resources acquired during initialisation or rendering.
	*/
	static void Cleanup();
	
	/**
	* @brief Updates the time and calculates the frame time and FPS.
	*
	* This function calculates tdelta time between frames using GLFW's time functions.
	* It also computes the FPS at a specified interval, allowing it to update every "fps_calc_interval" seconds.
	* This function must be called once per game loop to ensure accurate time tracking and FPS measurement.
	*
	* @param[in] fps_calc_interval The interval (in seconds) at which the FPS should be calculated. The value is clamped between 0 and 10 seconds.
	*
	* The function performs two primary calculations:
	* 1. Delta time between the current and previous frame.
	* 2. FPS, which is recalculated at least every "fps_calc_interval" seconds.
	*
	* The calculated FPS value is stored in GLHelper::fps.
	*/
	static void Update_Time(double fpsCalcInt = 1.0);

	/**
	* @brief Loads the window icon from the specified file path.
	* This is called inside Initialise_OpenGL after the window is created.
	*
	* @param[in] window   The pointer to the window.
	* @param[in] iconPath The path to the texture file to load.
	*/
	// static void Set_Window_Icon(GLFWwindow* window, const char* iconPath);

    /**
	* @brief Prints graphics driver and hardware specifications.
	*
	* This function outputs information about the current graphics hardware and its capabilities.
	* The specifications include details about the OpenGL version, supported extensions and other hardware details.
	*/
	static void Print_Specifications();

	/**
	* @brief Interpolates between two colours.
	*
	* This function calculates the interpolation between two specified RGB colors based on the value of `t`.
	* The result is stored in the provided result array.
	*
	* @param[in] t A float value between 0.0 and 1.0 representing the interpolation factor.
	* @param[in] color1 A float array representing the first colour (RGB format).
	* @param[in] color2 A float array representing the second colour (RGB format).
	* @param[out] result A float array where the interpolated colour will be stored (RGB format).
    */
	static void Interpolate_Color(float t, const float color1[3], const float color2[3], float result[3]);
};

#endif /* GLHELPER_H */
