#include <Resources/Mesh.h>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    // Calculate AABB from vertices
    calculateAABB();

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

void Mesh::calculateAABB()
{
    if (vertices.empty()) {
        m_AABB = AABB(); // Default empty AABB
        return;
    }

    glm::vec3 min = vertices[0].Position;
    glm::vec3 max = vertices[0].Position;

    for (const auto& vertex : vertices) {
        min = glm::min(min, vertex.Position);
        max = glm::max(max, vertex.Position);
    }

    m_AABB = AABB(min, max);
}