/******************************************************************************/
/*!
\file   IndexBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the IndexBuffer class for OpenGL index buffer object (IBO) management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <cstdint>

class IndexBuffer
{
public:
    IndexBuffer(const uint32_t *indices, uint32_t count);
    ~IndexBuffer();

    uint32_t GetCount() const
    {
        return m_Count;
    }

    uint32_t GetHandle() const
    {
        return m_IBOHandle;
    }

    uint32_t GetIBOHandle() const
    {
        return m_IBOHandle;
    }

private:
    uint32_t m_IBOHandle;
    uint32_t m_Count;
};