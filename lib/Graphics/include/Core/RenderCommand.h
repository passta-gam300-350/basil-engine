#pragma once

#include <glm/glm.hpp>
#include <memory>

class RenderCommand
{
public:
	virtual ~RenderCommand() = default;
	virtual void Execute() = 0;

	// Clone method for copying commands
	virtual RenderCommand* Clone() const = 0;
};

class ClearCommand : public RenderCommand
{
public:
	ClearCommand(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
	virtual void Execute() override;
	virtual RenderCommand* Clone() const override { return new ClearCommand(m_ClearColor); }

private:
	glm::vec4 m_ClearColor;
}

class DrawCommand : public RenderCommand
{
public:
	DrawCommand(uint32_t vao, uint32_t indexCount);
	virtual void Execute() override;
	virtual RenderCommand* Clone() const override { return new DrawCommand(m_VertexArrayID, m_IndexCount); }

private:
	uint32_t m_VertexArray;
	uint32_t m_IndexCount;
}