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
	case GL_INT: return sizeof(GLint);
	case GL_UNSIGNED_INT: return sizeof(GLuint);
	case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
	default: return 0;
	}
}

VertexArray::VertexArray()
	: m_VAOHandle(0)
{
	// DSA: Create VAO without binding
	glCreateVertexArrays(1, &m_VAOHandle);
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
	// DSA: No binding required!
	const uint32_t bindingIndex = static_cast<uint32_t>(m_VertexBuffers.size());
	const uint32_t vboHandle = vertexBuffer->GetHandle();
	const auto &elements = layout.GetElements();

	// Attach VBO to VAO at this binding index
	glVertexArrayVertexBuffer(m_VAOHandle, bindingIndex, vboHandle, 0, static_cast<GLsizei>(layout.GetStride()));

	uint32_t offset = 0;

	for (uint32_t i = 0; i < elements.size(); ++i)
	{
		const auto &element = elements[i];
		const uint32_t attribIndex = m_AttributeIndex + i;

		// DSA: Enable attribute without binding VAO
		glEnableVertexArrayAttrib(m_VAOHandle, attribIndex);

		// Integer types (GL_INT, GL_UNSIGNED_INT, GL_BYTE, GL_SHORT, etc.) must use
		// glVertexArrayAttribIFormat to preserve integer data in the shader.
		// Using glVertexArrayAttribFormat would convert them to float, corrupting ivec4 attributes
		// like bone IDs.
		if (element.type == GL_INT || element.type == GL_UNSIGNED_INT ||
			element.type == GL_BYTE || element.type == GL_UNSIGNED_BYTE ||
			element.type == GL_SHORT || element.type == GL_UNSIGNED_SHORT)
		{
			// DSA: Set integer attribute format
			glVertexArrayAttribIFormat(
				m_VAOHandle, attribIndex,
				static_cast<GLint>(element.count),
				element.type,
				offset
			);
		}
		else
		{
			// DSA: Set float attribute format
			glVertexArrayAttribFormat(
				m_VAOHandle, attribIndex,
				static_cast<GLint>(element.count),
				element.type,
				element.normalised != 0 ? GL_TRUE : GL_FALSE,
				offset
			);
		}

		// DSA: Associate attribute with binding index
		glVertexArrayAttribBinding(m_VAOHandle, attribIndex, bindingIndex);

		offset += element.count * VertexBufferLayout::Element::GetSizeOfType(element.type);
	}

	// Update attribute index for next VBO
	m_AttributeIndex += static_cast<uint32_t>(elements.size());
	m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer> const &indexBuffer)
{
	// DSA: Attach index buffer to VAO without binding
	glVertexArrayElementBuffer(m_VAOHandle, indexBuffer->GetHandle());
	m_IndexBuffer = indexBuffer;
}