#pragma once

#include <Buffer/VertexArray.h>
#include <Buffer/VertexBuffer.h>
#include <Buffer/IndexBuffer.h>
#include <Resources/Material.h>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#include <assimp/mesh.h>
#include <assimp/scene.h>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;

    // Bone data for animation (future use)
    int BoneIDs[MAX_BONE_INFLUENCE];
    float Weights[MAX_BONE_INFLUENCE];

    Vertex()
    {
        // Initialize bone data
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            BoneIDs[i] = -1;
            Weights[i] = 0.0f;
        }
    }
};

class Mesh
{
public:
    // Constructors
    Mesh(std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices,
        std::shared_ptr<Material> const &material = nullptr);
    ~Mesh() = default;

    // Core functions
    void Bind() const;
    void Unbind() const;
    void Draw() const;

    //Getters
    uint32_t GetVertexCount() const
    {
        return static_cast<uint32_t>(m_Vertices.size());
    }
    uint32_t GetIndexCount() const
    {
        return static_cast<uint32_t>(m_Indices.size());
    }
    std::vector<Vertex> const &GetVertices() const
    {
        return m_Vertices;
    }
    std::vector<uint32_t> const &GetIndices() const
    {
        return m_Indices;
    }

    std::shared_ptr<VertexArray> &GetVertexArray()
    {
        return m_VertexArray;
    }


    // Material management
    void SetMaterial(std::shared_ptr<Material> const &material)
    {
        m_Material = material;
    }
    std::shared_ptr<Material> GetMaterial() const
    {
        return m_Material;
    }
    bool HasMaterial() const
    {
        return m_Material != nullptr;
    }

    // Static factory methods
    static std::shared_ptr<Mesh> CreateFromAssimp(aiMesh *assimpMesh, const aiScene *scene);

    // Utility functions
    void RecalculateNormals();
    void RecalculateTangents();
    glm::vec3 GetCenter() const;
    float GetBoundingRadius() const;


private:
    void SetupMesh();
    void SetupVertexAttributes();

    // Static helper for tangent calculation
    static void CalculateTangents(std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

    // Mesh data
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::shared_ptr<Material> m_Material;

    // OpenGL objects
    std::shared_ptr<VertexArray> m_VertexArray;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;

    // Bounding info (for culling)
    mutable glm::vec3 m_Center = glm::vec3(0.0f);
    mutable float m_BoundingRadius = 0.0f;
    mutable bool m_BoundingInfoValid = false;
};