#include <Buffer/UniformBuffer.h>
#include <glad/glad.h>

UniformBuffer::UniformBuffer(uint32_t size, uint32_t binding)
	: m_Binding(binding), m_UBOHandle(0)
{
	glGenBuffers(1, &m_UBOHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, m_UBOHandle);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_UBOHandle);
}

UniformBuffer::~UniformBuffer()
{
	glDeleteBuffers(1, &m_UBOHandle);
}

void UniformBuffer::SetData(const void *data, uint32_t size, uint32_t offset) const
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_UBOHandle);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}

void UniformBuffer::Bind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_UBOHandle);
}

void UniformBuffer::Unbind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}