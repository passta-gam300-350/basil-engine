#include <Resources/Mesh.h>
#include <iostream>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    // now that we have all the required data, set the vertex buffers and its attribute pointers.
    setupMesh();
}

void Mesh::Draw(Shader &shader) 
{
    // bind appropriate textures
    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr   = 1;
    unsigned int heightNr   = 1;
    
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
        
        // retrieve texture number (the N in diffuse_textureN)
        std::string number;
        std::string name = textures[i].type;
        if(name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if(name == "texture_specular")
            number = std::to_string(specularNr++);
        else if(name == "texture_normal")
            number = std::to_string(normalNr++);
         else if(name == "texture_height")
            number = std::to_string(heightNr++);

        // now set the sampler to the correct texture unit
        shader.setInt((name + number).c_str(), i);
        
        // and finally bind the texture
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    
    // draw mesh
    m_VertexArray->Bind();
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    m_VertexArray->Unbind();

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::setupMesh()
{
    // create buffers/arrays using existing buffer classes
    m_VertexArray = std::make_shared<VertexArray>();
    m_VertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex));
    m_IndexBuffer = std::make_shared<IndexBuffer>(indices.data(), indices.size());

    // Set up vertex buffer layout
    VertexBufferLayout layout;
    layout.Push<float>(3); // Position
    layout.Push<float>(3); // Normal
    layout.Push<float>(2); // TexCoords
    layout.Push<float>(3); // Tangent
    layout.Push<float>(3); // Bitangent
    layout.Push<int>(4);   // BoneIDs
    layout.Push<float>(4); // Weights

    m_VertexArray->AddVertexBuffer(m_VertexBuffer, layout);
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);
}