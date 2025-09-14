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

// Draw() method removed - rendering is now handled by DrawMeshCommand in the RenderCommand system
// This keeps Mesh as pure data without OpenGL calls

void Mesh::setupMesh()
{
    // create buffers/arrays using existing buffer classes
    m_VertexArray = std::make_shared<VertexArray>();

    uint32_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    m_VertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertexBufferSize);
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