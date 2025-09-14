#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <Resources/Texture.h>
#include <Resources/Shader.h>

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
};

class DrawCommand : public RenderCommand
{
public:
	DrawCommand(uint32_t vao, uint32_t indexCount);
	virtual void Execute() override;
	virtual RenderCommand* Clone() const override { return new DrawCommand(m_VertexArrayID, m_IndexCount); }

private:
	uint32_t m_VertexArrayID;
	uint32_t m_IndexCount;
};

class DrawWithTexturesCommand : public RenderCommand
{
public:
	DrawWithTexturesCommand(uint32_t vao, uint32_t indexCount, 
	                       const std::vector<Texture>& textures, 
	                       std::shared_ptr<Shader> shader);
	virtual void Execute() override;
	virtual RenderCommand* Clone() const override;

private:
	uint32_t m_VertexArrayID;
	uint32_t m_IndexCount;
	std::vector<Texture> m_Textures;
	std::shared_ptr<Shader> m_Shader;
};