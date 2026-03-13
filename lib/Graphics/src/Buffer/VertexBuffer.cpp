/******************************************************************************/
/*!
\file   VertexBuffer.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of OpenGL vertex buffer object for storing vertex data

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Buffer/VertexBuffer.h>
#include <glad/glad.h>

VertexBuffer::VertexBuffer(const void *data, uint32_t size)
	: m_VBOHandle(0)
{
	// DSA: Create and initialize buffer in one step, no binding required
	glCreateBuffers(1, &m_VBOHandle);
	glNamedBufferData(m_VBOHandle, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_VBOHandle);
}

void VertexBuffer::SetData(const void *data, uint32_t size) const
{
	// DSA: Update buffer data without binding
	glNamedBufferSubData(m_VBOHandle, 0, size, data);
}