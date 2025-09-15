/*!************************************************************************
\file:      InputGraphics_Helper.cpp
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file implements functionality useful and necessary to build OpenGL applications including use of external APIs such as GLFW to create a window and start up an
            OpenGL context and use GLEW to extract function pointers to OpenGL implementations.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/
#pragma once

#ifndef ENGINE_INPUT_USE_HELPER_IMPL
#else
#include "./Input/InputGraphics_Helper.h"
#include "./Input/InputManager.h"

// Static data members declared in GLHelper
GLint GLHelper::width;
GLint GLHelper::height;
GLdouble GLHelper::fps;
GLdouble GLHelper::delta_time;
std::string GLHelper::title;
GLFWwindow* GLHelper::ptr_window;
GLFWmonitor* GLHelper::monitor;
const GLFWvidmode* GLHelper::mode;
GLint GLHelper::windowX, GLHelper::windowY;

bool isWindowMinimized;
bool isWindowFocused;
bool modeChanged;
bool isFullscreen;

bool GLHelper::Initialise_OpenGL(GLint widthInit, GLint heightInit, std::string titleInit)
{
    GLHelper::width = widthInit;
    GLHelper::height = heightInit;
    GLHelper::title = titleInit;
    GLHelper::windowX = 0, GLHelper::windowY = 0;

    if (!glfwInit())
    {
        return false;
    }

    // In case a GLFW function fails, an error is reported to callback function
    #ifdef _DEBUG
        glfwSetErrorCallback(InputManager::Error_Callback);
    #endif
    // Before asking GLFW to create an OpenGL context, specify the minimum constraints in that context:
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_RED_BITS, 8); glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8); glfwWindowHint(GLFW_ALPHA_BITS, 8);

    monitor = glfwGetPrimaryMonitor();                   // Get the primary monitor
    if (!monitor)
    {
        throw std::runtime_error("Failed to retrieve the primary monitor.");
    }
    mode = glfwGetVideoMode(monitor);                    //  Get the video mode
    if (!mode)
    {
        throw std::runtime_error("Failed to retrieve video mode.");
    }

    GLHelper::ptr_window = glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, NULL);
    if (!GLHelper::ptr_window)
    {
        #ifdef _DEBUG
        std::cerr << "GLFW unable to create OpenGL context - abort program\n";
        #endif  
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(GLHelper::ptr_window);
    glfwSwapInterval(1); // cap fps to refresh rate
    glfwMaximizeWindow(GLHelper::ptr_window);
    InputManager::Setup_Callbacks();
    glfwSetInputMode(GLHelper::ptr_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Default setting
    GLHelper::Update_Screen_Mode(true, GLHelper::windowX, GLHelper::windowY, GLHelper::width, GLHelper::height);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        return false;
    }
    return true;
}

void GLHelper::Update_OpenGL()
{
    isWindowMinimized = glfwGetWindowAttrib(GLHelper::ptr_window, GLFW_ICONIFIED);  // Get window minimise state
    isWindowFocused = glfwGetWindowAttrib(GLHelper::ptr_window, GLFW_FOCUSED);      // Get window focus state
    if (!isWindowFocused)                                                           // Always minimise regardless of screen mode when window loses focus
    {
        glfwIconifyWindow(GLHelper::ptr_window);
    }
    if (modeChanged)
    {
        GLHelper::Update_Screen_Mode(isFullscreen, GLHelper::windowX, GLHelper::windowY, GLHelper::width, GLHelper::height);
        modeChanged = false;
    }
}

void GLHelper::Update_Screen_Mode(bool windowMode, GLint& windowedX, GLint& windowedY, GLint& windowedWidth, GLint& windowedHeight)
{
    if (windowMode)                                                                                                                                      // Fullscreen mode
    {
        glfwGetWindowPos(GLHelper::ptr_window, &windowedX, &windowedY);                                                                                  // Store the current window top-left position
        glfwGetWindowSize(GLHelper::ptr_window, &windowedWidth, &windowedHeight);                                                                        // Store the current window resolution size
        glfwSetWindowMonitor(GLHelper::ptr_window, GLHelper::monitor, 0, 0, GLHelper::mode->width, GLHelper::mode->height, GLHelper::mode->refreshRate); // Fullscreen settings
    }
    else                                                                                                                                                 // Windowed mode
    {
        int workAreaX, workAreaY, workAreaWidth, workAreaHeight;
        glfwGetMonitorWorkarea(GLHelper::monitor, &workAreaX, &workAreaY, &workAreaWidth, &workAreaHeight);                                              // Get monitor work area
        windowedWidth = std::min(windowedWidth, workAreaWidth);                                                                                          // Ensure window width does not exceed work area
        windowedHeight = std::min(windowedHeight, workAreaHeight);                                                                                       // Ensure window height does not exceed work area
        windowedY = workAreaY + 45;                                                                                                                      // Ensure the title bar is visible and doesn't increment over toggles
        glfwSetWindowMonitor(GLHelper::ptr_window, NULL, windowedX, windowedY, windowedWidth, windowedHeight, 0);                                        // Windowed settings, sets the resolution dimensions specified in the json file. Need to account for title bar offset (y + 30) after coming back from windowed mode
        glfwSetWindowAttrib(GLHelper::ptr_window, GLFW_RESIZABLE, GLFW_FALSE);                                                                           // No resizing
        glfwSetWindowSize(GLHelper::ptr_window, windowedWidth, windowedHeight);                                                                          // Consistent resolution
    }
    glfwMakeContextCurrent(GLHelper::ptr_window);                                                                                                        // Ensure context is still active
}

void GLHelper::Cleanup()
{
    glfwDestroyWindow(GLHelper::ptr_window);
    glfwTerminate();
}

void GLHelper::Update_Time(double fps_calc_interval)
{
    // Elapsed time between previous and current frames
    static double prev_time = glfwGetTime();
    double curr_time = glfwGetTime();
    delta_time = curr_time - prev_time;
    prev_time = curr_time;

    // FPS calculations
    static double count = 0.0; // number of game loop iterations
    static double start_time = glfwGetTime();
    // Get elapsed time since very beginning
    double elapsed_time = curr_time - start_time;

    ++count;

    // Update fps at least every 10 seconds
    fps_calc_interval = (fps_calc_interval < 0.0) ? 0.0 : fps_calc_interval;
    fps_calc_interval = (fps_calc_interval > 10.0) ? 10.0 : fps_calc_interval;
    if (elapsed_time > fps_calc_interval)
    {
        GLHelper::fps = count / elapsed_time;
        start_time = curr_time;
        count = 0.0;
    }
}

void GLHelper::Print_Specifications()
{
    [[maybe_unused]] GLubyte const* glVendor = glGetString(GL_VENDOR);
    [[maybe_unused]] GLubyte const* glRenderer = glGetString(GL_RENDERER);
    [[maybe_unused]] GLubyte const* glVersion = glGetString(GL_VERSION);
    [[maybe_unused]] GLubyte const* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint glMajorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    GLint glMinorVersion;
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    GLint glIsDoubleBuffered;
    glGetIntegerv(GL_DOUBLEBUFFER, &glIsDoubleBuffered);
    GLint glMaxVerticesCount;
    glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &glMaxVerticesCount);
    GLint glMaxIndicesCount;
    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &glMaxIndicesCount);
    GLint glTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glTextureSize);
    GLint maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    GLint glMaxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, glMaxViewportDims);
    GLint glMaxVertexAttribs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &glMaxVertexAttribs);
    GLint glMaxVertexAttribsBindings;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &glMaxVertexAttribsBindings);
    GLfloat lineWidthRange[2];
    glGetFloatv(GL_LINE_WIDTH_RANGE, lineWidthRange); 
}

void GLHelper::Interpolate_Color(float t, const float color1[3], const float color2[3], float result[3])
{
    result[0] = (1.0f - t) * color1[0] + t * color2[0]; // Red
    result[1] = (1.0f - t) * color1[1] + t * color2[1]; // Green
    result[2] = (1.0f - t) * color1[2] + t * color2[2]; // Blue
}
#endif