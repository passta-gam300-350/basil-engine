/******************************************************************************/
/*!
\file   IndexBuffer.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of OpenGL index buffer object for indexed rendering

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Buffer/IndexBuffer.h>
#include <glad/glad.h>

IndexBuffer::IndexBuffer(const uint32_t *indices, uint32_t count)
	: m_IBOHandle(0), m_Count(count)
{
	// DSA: Create and initialize buffer in one step, no binding required
	glCreateBuffers(1, &m_IBOHandle);
	glNamedBufferData(m_IBOHandle, static_cast<GLsizeiptr>(count * sizeof(uint32_t)), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_IBOHandle);
}