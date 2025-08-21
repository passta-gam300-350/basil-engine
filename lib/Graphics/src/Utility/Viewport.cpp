#include "../../include/Utility/Viewport.h"
#include <glad/gl.h>

Viewport::Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    : m_X(x), m_Y(y), m_Width(width), m_Height(height)
{
}

void Viewport::Set(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    m_X = x;
    m_Y = y;
    m_Width = width;
    m_Height = height;
}

void Viewport::Apply() const
{
    glViewport(m_X, m_Y, m_Width, m_Height);
}