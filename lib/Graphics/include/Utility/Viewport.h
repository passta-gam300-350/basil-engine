/******************************************************************************/
/*!
\file   Viewport.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the Viewport class for managing OpenGL viewport dimensions

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

class Viewport
{
public:
    Viewport(uint32_t x = 0, uint32_t y = 0, uint32_t width = 0, uint32_t height = 0);
    ~Viewport() = default;

    void Set(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void Apply() const;

    uint32_t GetX() const { return m_X; }
    uint32_t GetY() const { return m_Y; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    float GetAspectRatio() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height); }
    glm::vec2 GetSize() const { return { static_cast<float>(m_Width), static_cast<float>(m_Height) }; }

private:
    uint32_t m_X, m_Y;
    uint32_t m_Width, m_Height;
};