#include "../../include/Core/RenderCommand.h"
#include <glad/gl.h>

// ClearCommand implementation
ClearCommand::ClearCommand(const glm::vec4& color)
    : m_ClearColor(color)
{
}

void ClearCommand::Execute()
{
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// DrawCommand implementation
DrawCommand::DrawCommand(uint32_t vao, uint32_t indexCount)
    : m_VertexArrayID(vao), m_IndexCount(indexCount)
{
}

void DrawCommand::Execute()
{
    glBindVertexArray(m_VertexArrayID);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}