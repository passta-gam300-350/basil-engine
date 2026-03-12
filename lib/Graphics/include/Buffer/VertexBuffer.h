/******************************************************************************/
/*!
\file   VertexBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the VertexBuffer class for OpenGL vertex buffer object (VBO) management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <cstdint>

class VertexBuffer
{
public:
	VertexBuffer(const void *data, uint32_t size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;

    void SetData(const void *data, uint32_t size) const;

    uint32_t GetHandle() const
    {
        return m_VBOHandle;
    }

    uint32_t GetVAOHandle() const  // Legacy: Misnamed, use GetHandle() instead
    {
        return m_VBOHandle;
    }

private:
    uint32_t m_VBOHandle;
};