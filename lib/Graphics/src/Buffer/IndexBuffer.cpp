#include <Buffer/IndexBuffer.h>
#include <glad/glad.h>

IndexBuffer::IndexBuffer(const uint32_t *indices, uint32_t count) 
	: m_IBOHandle(0), m_Count(count)
{
	glGenBuffers(1, &m_IBOHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBOHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(uint32_t)), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_IBOHandle);
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBOHandle);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}