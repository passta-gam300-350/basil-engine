/******************************************************************************/
/*!
\file   VertexArray.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of OpenGL vertex array object for vertex attribute configuration

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Buffer/VertexArray.h>
#include <glad/glad.h>

uint32_t VertexBufferLayout::Element::GetSizeOfType(uint32_t type)
{
	switch (type)
	{
	case GL_FLOAT: return sizeof(GLfloat);
	case GL_UNSIGNED_INT: return sizeof(GLuint);
	case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
	default: return 0;
	}
}

VertexArray::VertexArray()
	: m_VAOHandle(0)
{
	glGenVertexArrays(1, &m_VAOHandle);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &m_VAOHandle);
}

void VertexArray::Bind() const
{
	glBindVertexArray(m_VAOHandle);
}

void VertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(std::shared_ptr<VertexBuffer> const &vertexBuffer, VertexBufferLayout const &layout)
{
	Bind();
	vertexBuffer->Bind();

	const auto &elements = layout.GetElements();
	uint32_t offset = 0;

	for (uint32_t i = 0; i < elements.size(); ++i)
	{
		const auto &element = elements[i];

		glEnableVertexAttribArray(i);
		glVertexAttribPointer(
			i, static_cast<GLint>(element.count),
			element.type, element.normalised != 0 ? GL_TRUE : GL_FALSE,
			static_cast<GLsizei>(layout.GetStride()), reinterpret_cast<const void *>(static_cast<intptr_t>(offset))
		);

		offset += element.count * VertexBufferLayout::Element::GetSizeOfType(element.type);
	}

	m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer> const &indexBuffer)
{
	Bind();
	indexBuffer->Bind();
	m_IndexBuffer = indexBuffer;

	// Properly clean up OpenGL state to prevent corruption
	Unbind();
}