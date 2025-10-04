/******************************************************************************/
/*!
\file   Viewport.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of viewport management for configuring OpenGL rendering regions

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Utility/Viewport.h"
#include <glad/glad.h>

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