#include <Resources/Mesh.h>
#include <glad/gl.h>
#include <iostream>
#include <algorithm>
#include <cmath>

Mesh::Mesh(std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices,
    std::shared_ptr<Material> const &material = nullptr)
    : m_Vertices(vertices), m_Indices(indices), m_Material(material)
{
    SetupMesh();
}

void Mesh::Bind() const
{
    m_VertexArray->Bind();
}

void Mesh::Unbind() const
{
    m_VertexArray->Unbind();
}

void Mesh::Draw() const
{
    // Bind material if available
    if (m_Material)
    {
        m_Material->Bind();
    }

    // Bind and draw mesh
    Bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()), GL_UNSIGNED_INT, 0);
    Unbind();

    // Unbind material
    if (m_Material)
    {
        m_Material->Unbind();
    }
}

std::shared_ptr<Mesh> Mesh::CreateFromAssimp(aiMesh *assimpMesh, const aiScene *scene)
{
    if (!assimpMesh)
    {
        std::cerr << "Error: Null aiMesh passed to CreateFromAssimp" << std::endl;
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::cout << "Processing Assimp mesh: " << assimpMesh->mName.C_Str()
        << " (" << assimpMesh->mNumVertices << " vertices)" << std::endl;

    // Process vertices
    vertices.reserve(assimpMesh->mNumVertices);
    for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Position
        vertex.Position.x = assimpMesh->mVertices[i].x;
        vertex.Position.y = assimpMesh->mVertices[i].y;
        vertex.Position.z = assimpMesh->mVertices[i].z;

        // Normals
        if (assimpMesh->HasNormals())
        {
            vertex.Normal.x = assimpMesh->mNormals[i].x;
            vertex.Normal.y = assimpMesh->mNormals[i].y;
            vertex.Normal.z = assimpMesh->mNormals[i].z;
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up
        }

        // Texture coordinates (use first UV set if available)
        if (assimpMesh->mTextureCoords[0])
        {
            vertex.TexCoords.x = assimpMesh->mTextureCoords[0][i].x;
            vertex.TexCoords.y = assimpMesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        // Tangents
        if (assimpMesh->HasTangentsAndBitangents())
        {
            vertex.Tangent.x = assimpMesh->mTangents[i].x;
            vertex.Tangent.y = assimpMesh->mTangents[i].y;
            vertex.Tangent.z = assimpMesh->mTangents[i].z;

            vertex.Bitangent.x = assimpMesh->mBitangents[i].x;
            vertex.Bitangent.y = assimpMesh->mBitangents[i].y;
            vertex.Bitangent.z = assimpMesh->mBitangents[i].z;
        }
        else
        {
            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);   // Default tangent
            vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f); // Default bitangent
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
    {
        aiFace face = assimpMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Calculate tangents if not provided by Assimp
    if (!assimpMesh->HasTangentsAndBitangents())
    {
        CalculateTangents(vertices, indices);
    }

    // Create the mesh
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    return mesh;
}

void Mesh::RecalculateNormals()
{
    // Reset all normals
    for (auto &vertex : m_Vertices)
    {
        vertex.Normal = glm::vec3(0.0f);
    }

    // Calculate face normals and accumulate
    for (size_t i = 0; i < m_Indices.size(); i += 3)
    {
        uint32_t i0 = m_Indices[i];
        uint32_t i1 = m_Indices[i + 1];
        uint32_t i2 = m_Indices[i + 2];

        glm::vec3 v0 = m_Vertices[i0].Position;
        glm::vec3 v1 = m_Vertices[i1].Position;
        glm::vec3 v2 = m_Vertices[i2].Position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        m_Vertices[i0].Normal += normal;
        m_Vertices[i1].Normal += normal;
        m_Vertices[i2].Normal += normal;
    }

    // Normalize all normals
    for (auto &vertex : m_Vertices)
    {
        vertex.Normal = glm::normalize(vertex.Normal);
    }

    // Update the vertex buffer
    m_VertexBuffer->SetData(m_Vertices.data(), static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex)));
}

void Mesh::RecalculateTangents()
{
    CalculateTangents(m_Vertices, m_Indices);

    // Update the vertex buffer
    m_VertexBuffer->SetData(m_Vertices.data(), static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex)));
}

glm::vec3 Mesh::GetCenter() const
{
    if (!m_BoundingInfoValid)
    {
        // Calculate center
        glm::vec3 min(FLT_MAX);
        glm::vec3 max(-FLT_MAX);

        for (const auto &vertex : m_Vertices)
        {
            min = glm::min(min, vertex.Position);
            max = glm::max(max, vertex.Position);
        }

        m_Center = (min + max) * 0.5f;

        // Calculate bounding radius
        m_BoundingRadius = 0.0f;
        for (const auto &vertex : m_Vertices)
        {
            float distance = glm::length(vertex.Position - m_Center);
            m_BoundingRadius = std::max(m_BoundingRadius, distance);
        }

        m_BoundingInfoValid = true;
    }

    return m_Center;
}

float Mesh::GetBoundingRadius() const
{
    if (!m_BoundingInfoValid)
    {
        GetCenter(); // This will calculate both center and radius
    }
    return m_BoundingRadius;
}

void Mesh::SetupMesh()
{
    // Create VAO
    m_VertexArray = std::make_shared<VertexArray>();

    // Create VBO
    m_VertexBuffer = std::make_shared<VertexBuffer>(
        m_Vertices.data(),
        static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex))
    );

    // Create IBO
    m_IndexBuffer = std::make_shared<IndexBuffer>(
        m_Indices.data(),
        static_cast<uint32_t>(m_Indices.size())
    );

    // Setup vertex attributes
    SetupVertexAttributes();

    std::cout << "Created mesh with " << m_Vertices.size() << " vertices and "
        << m_Indices.size() << " indices" << std::endl;
}

void Mesh::SetupVertexAttributes()
{
    // Define vertex layout
    VertexBufferLayout layout;
    layout.Push<float>(3); // Position
    layout.Push<float>(3); // Normal
    layout.Push<float>(2); // TexCoords
    layout.Push<float>(3); // Tangent
    layout.Push<float>(3); // Bitangent
    layout.Push<int>(4);   // Bone IDs
    layout.Push<float>(4); // Bone Weights

    // Set up vertex array
    m_VertexArray->AddVertexBuffer(m_VertexBuffer, layout);
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);
}

void Mesh::CalculateTangents(std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
{
    // Reset tangents and bitangents
    for (auto &vertex : vertices)
    {
        vertex.Tangent = glm::vec3(0.0f);
        vertex.Bitangent = glm::vec3(0.0f);
    }

    // Calculate tangents for each triangle
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        Vertex &v0 = vertices[i0];
        Vertex &v1 = vertices[i1];
        Vertex &v2 = vertices[i2];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;

        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;

        v0.Bitangent += bitangent;
        v1.Bitangent += bitangent;
        v2.Bitangent += bitangent;
    }

    // Normalize and orthogonalize tangents
    for (auto &vertex : vertices)
    {
        // Gram-Schmidt orthogonalize
        vertex.Tangent = glm::normalize(vertex.Tangent - vertex.Normal * glm::dot(vertex.Normal, vertex.Tangent));

        // Calculate handedness
        if (glm::dot(glm::cross(vertex.Normal, vertex.Tangent), vertex.Bitangent) < 0.0f)
        {
            vertex.Tangent = -vertex.Tangent;
        }

        vertex.Bitangent = glm::normalize(vertex.Bitangent);
    }
}