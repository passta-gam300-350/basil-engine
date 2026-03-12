/******************************************************************************/
/*!
\file   VertexArray.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the VertexArray and VertexBufferLayout classes for OpenGL VAO management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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

template<>
inline void VertexBufferLayout::Push<int>(uint32_t count)
{
	m_Elements.push_back({ 0x1404, count, false }); // GL_INT
	m_Stride += count * sizeof(int);
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

	uint32_t GetVAOHandle() const
	{
		return m_VAOHandle;
	}

private:
	uint32_t m_VAOHandle;
	std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;

};

