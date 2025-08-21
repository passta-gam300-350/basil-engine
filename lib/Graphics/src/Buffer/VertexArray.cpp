#include <Buffer/VertexArray.h>
#include <glad/gl.h>

uint32_t VertexBufferLayout::Element::GetSizeOfType(uint32_t type)
{
	switch (type)
	{
	case GL_FLOAT: return sizeof(GLfloat);
	case GL_UNSIGNED_INT: return sizeof(GLuint);
	case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
	}
}

VertexArray::VertexArray()
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
			i, element.count,
			element.type, element.normalised ? GL_TRUE : GL_FALSE,
			layout.GetStride(), (const void *)(intptr_t)offset
		);

		offset += element.count * element.GetSizeOfType(element.type);
	}

	m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer> const &indexBuffer)
{
	Bind();
	indexBuffer->Bind();

	m_IndexBuffer = indexBuffer;
}