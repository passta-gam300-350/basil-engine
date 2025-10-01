#include <Buffer/VertexBuffer.h>
#include <glad/glad.h>

VertexBuffer::VertexBuffer(const void *data, uint32_t size)
	: m_VBOHandle(0)
{
	glGenBuffers(1, &m_VBOHandle);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOHandle);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_VBOHandle);
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOHandle);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void *data, uint32_t size) const
{
	Bind();
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}