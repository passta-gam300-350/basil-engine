#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <memory>
#include <vector>

class VertexBufferLayout // Manages the VBO's attributes
{
public:
	struct Element
	{
		uint32_t type;
		uint32_t count;
		uint8_t normalised;

		static uint32_t GetSizeOfType(uint32_t type);
	};

	VertexBufferLayout() = default;

	template<typename T>
	void Push(uint32_t count);

	const std::vector<Element> &GetElements() const
	{
		return m_Elements;
	}

	uint32_t GetStride() const
	{
		return m_Stride;
	}

private:
	std::vector<Element> m_Elements;
	uint32_t m_Stride = 0;
};

template<>
inline void VertexBufferLayout::Push<float>(uint32_t count)
{
	m_Elements.push_back({ 0x1406, count, false }); // GL_FLOAT
	m_Stride += count * sizeof(float);
}

template<>
inline void VertexBufferLayout::Push<uint32_t>(uint32_t count)
{
	m_Elements.push_back({ 0x1405, count, false }); // GL_UNSIGNED_INT
	m_Stride += count * sizeof(uint32_t);
}

template<>
inline void VertexBufferLayout::Push<uint8_t>(uint32_t count)
{
	m_Elements.push_back({ 0x1401, count, false }); // GL_UNSIGNED_BYTE
	m_Stride += count * sizeof(uint8_t);
}

class VertexArray // VAO Wrapper
{
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;

	void AddVertexBuffer(std::shared_ptr<VertexBuffer> const &vertexBuffer, VertexBufferLayout const &layout);
	void SetIndexBuffer(std::shared_ptr<IndexBuffer> const &indexBuffer);

	const std::vector<std::shared_ptr<VertexBuffer>> &GetVertexBuffers() const
	{
		return m_VertexBuffers;
	}

	const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}

private:
	uint32_t m_VAOHandle;
	std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers; // Vector of buffers since a buffer represent a type data like position/texCoord/normals
	std::shared_ptr<IndexBuffer> m_IndexBuffer;

};

