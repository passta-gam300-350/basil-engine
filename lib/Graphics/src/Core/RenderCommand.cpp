#include "../../include/Core/RenderCommand.h"
#include <glad/gl.h>
#include <iostream>

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

// DrawWithTexturesCommand implementation
DrawWithTexturesCommand::DrawWithTexturesCommand(uint32_t vao, uint32_t indexCount, 
                                               const std::vector<Texture>& textures, 
                                               std::shared_ptr<Shader> shader)
    : m_VertexArrayID(vao), m_IndexCount(indexCount), m_Textures(textures), m_Shader(shader)
{
}

void DrawWithTexturesCommand::Execute()
{
    if (!m_Shader) {
        std::cerr << "DrawWithTexturesCommand: No shader provided" << std::endl;
        return;
    }

    // Bind textures using the same logic as Mesh::Draw()
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    
    for (unsigned int i = 0; i < m_Textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        
        // Retrieve texture number (the N in diffuse_textureN)
        std::string number;
        std::string name = m_Textures[i].type;
        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "texture_normal")
            number = std::to_string(normalNr++);
        else if (name == "texture_height")
            number = std::to_string(heightNr++);

        // Set the sampler to the correct texture unit
        m_Shader->setInt((name + number).c_str(), i);
        
        // Bind the texture
        glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
    }
    
    // Draw the geometry
    glBindVertexArray(m_VertexArrayID);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset texture state
    glActiveTexture(GL_TEXTURE0);
}

RenderCommand* DrawWithTexturesCommand::Clone() const
{
    return new DrawWithTexturesCommand(m_VertexArrayID, m_IndexCount, m_Textures, m_Shader);
}