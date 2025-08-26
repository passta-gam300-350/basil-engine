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

// DrawMeshCommand implementation
DrawMeshCommand::DrawMeshCommand(uint32_t vao, uint32_t indexCount,
                                const std::vector<Texture>& textures,
                                std::shared_ptr<Shader> shader,
                                const glm::mat4& modelMatrix,
                                const glm::mat4& viewMatrix,
                                const glm::mat4& projectionMatrix,
                                const glm::vec3& cameraPosition)
    : m_VertexArrayID(vao), m_IndexCount(indexCount), m_Textures(textures), 
      m_Shader(shader), m_ModelMatrix(modelMatrix), m_ViewMatrix(viewMatrix),
      m_ProjectionMatrix(projectionMatrix), m_CameraPosition(cameraPosition)
{
}

void DrawMeshCommand::Execute()
{
    if (!m_Shader) {
        std::cerr << "DrawMeshCommand: No shader provided" << std::endl;
        return;
    }

    // Use the shader
    m_Shader->use();

    // Set transform matrices
    m_Shader->setMat4("u_Model", m_ModelMatrix);
    m_Shader->setMat4("u_View", m_ViewMatrix);
    m_Shader->setMat4("u_Projection", m_ProjectionMatrix);
    
    // Set camera position for lighting
    m_Shader->setVec3("u_ViewPos", m_CameraPosition);

    // Analyze textures and set availability flags
    bool hasDiffuse = false, hasNormal = false, hasMetallic = false, 
         hasRoughness = false, hasAO = false, hasEmissive = false;
    
    for (const auto& texture : m_Textures) {
        if (texture.type == "texture_diffuse") hasDiffuse = true;
        else if (texture.type == "texture_normal") hasNormal = true;
        else if (texture.type == "texture_metallic") hasMetallic = true;
        else if (texture.type == "texture_roughness") hasRoughness = true;
        else if (texture.type == "texture_ao") hasAO = true;
        else if (texture.type == "texture_emissive") hasEmissive = true;
    }
    
    // Set texture availability flags in shader
    m_Shader->setBool("u_HasDiffuseMap", hasDiffuse);
    m_Shader->setBool("u_HasNormalMap", hasNormal);
    m_Shader->setBool("u_HasMetallicMap", hasMetallic);
    m_Shader->setBool("u_HasRoughnessMap", hasRoughness);
    m_Shader->setBool("u_HasAOMap", hasAO);
    m_Shader->setBool("u_HasEmissiveMap", hasEmissive);

    // Bind textures using the same logic as the original DrawWithTexturesCommand
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    
    for (unsigned int i = 0; i < m_Textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        
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

        m_Shader->setInt((name + number).c_str(), i);
        glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
    }
    
    // Draw the geometry
    glBindVertexArray(m_VertexArrayID);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset texture state
    glActiveTexture(GL_TEXTURE0);
}

RenderCommand* DrawMeshCommand::Clone() const
{
    return new DrawMeshCommand(m_VertexArrayID, m_IndexCount, m_Textures, m_Shader,
                              m_ModelMatrix, m_ViewMatrix, m_ProjectionMatrix, m_CameraPosition);
}