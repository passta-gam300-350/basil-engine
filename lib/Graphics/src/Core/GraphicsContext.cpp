#include "../../include/Core/GraphicsContext.h"

#include <glad/gl.h>

GraphicsContext::GraphicsContext(GLFWwindow* windowHandle)
	: m_WindowHandle(windowHandle), m_Initialized(false)
{
}

GraphicsContext::~GraphicsContext()
{
}

void GraphicsContext::Initialize()
{
	if (m_Initialized) return;

    if (!m_WindowHandle)
    {
        std::cerr << "No window handle provided to GraphicsContext" << std::endl;
        return;
    }

    // Context should already be current from Window creation
    // Just verify GLAD is loaded
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD in GraphicsContext" << std::endl;
        return;
    }
	
    // Print OpenGL info
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Set default OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Initialized = true;
}

void GraphicsContext::SwapBuffers()
{
    if (!m_Initialized || !m_WindowHandle) return;
    glfwSwapBuffers(m_WindowHandle);
}

void GraphicsContext::SetVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
}

void GraphicsContext::SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void GraphicsContext::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}