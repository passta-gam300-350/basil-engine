#pragma once

#include <Buffer/VertexArray.h>
#include <Buffer/VertexBuffer.h>
#include <Buffer/IndexBuffer.h>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

class Mesh {
public:
    // mesh Data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    // Mesh ordering for deterministic sorting (hack until proper solution)
    uint16_t meshSortIndex = 0;

    // constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    // Note: Removed Draw() method - rendering is now handled by RenderCommand system

    // Getters for compatibility
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
    std::shared_ptr<VertexArray> GetVertexArray() const { return m_VertexArray; }

private:
    // OpenGL objects using existing buffer classes
    std::shared_ptr<VertexArray> m_VertexArray;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;

    // initializes all the buffer objects/arrays
    void setupMesh();
};