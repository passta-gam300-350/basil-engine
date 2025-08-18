#include "VertexBuffer.h"
#include <glad/gl.h>

VertexBuffer::VertexBuffer(const void *data, uint32_t size)
{
	glGenBuffers(1, &m_VBOHandle);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOHandle);
	glBufferData(m_VBOHandle, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_VBOHandle);
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOHandle)
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void *data, uint32_t size)
{
	Bind();
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}