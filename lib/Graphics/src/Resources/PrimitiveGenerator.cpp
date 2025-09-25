#include "Resources/PrimitiveGenerator.h"
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include "glm/ext/scalar_constants.hpp"

Mesh PrimitiveGenerator::CreatePlane(float width, float height,
                                     int subdivisionsX, int subdivisionsZ)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Ensure at least 1 subdivision
    subdivisionsX = std::max(1, subdivisionsX);
    subdivisionsZ = std::max(1, subdivisionsZ);

    // Calculate vertex count
    int verticesX = subdivisionsX + 1;
    int verticesZ = subdivisionsZ + 1;

    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    // Generate vertices
    for (int z = 0; z < verticesZ; ++z) {
        for (int x = 0; x < verticesX; ++x) {
            Vertex vertex;

            // Normalized coordinates [0,1]
            float u = static_cast<float>(x) / subdivisionsX;
            float v = static_cast<float>(z) / subdivisionsZ;

            // Position in XZ plane (Y = 0)
            // X: from -width/2 to width/2
            // Z: from -height/2 to height/2
            vertex.Position = glm::vec3(
                -halfWidth + width * u,
                0.0f,
                -halfHeight + height * v
            );

            // Normal pointing up (positive Y) for top face
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);

            // Texture coordinates
            // OpenGL convention: (0,0) at bottom-left corner
            // u increases left to right, v increases bottom to top
            vertex.TexCoords = glm::vec2(u, v);

            // Tangent space vectors for normal mapping
            // Tangent points along positive X axis
            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);

            // Bitangent points along positive Z axis
            vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);

            // Initialize bone data (unused for static geometry)
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                vertex.m_BoneIDs[i] = -1;
                vertex.m_Weights[i] = 0.0f;
            }

            vertices.push_back(vertex);
        }
    }

    // Generate indices using counter-clockwise winding for front faces
    // When viewed from above (positive Y), vertices should wind CCW
    for (int z = 0; z < subdivisionsZ; ++z) {
        for (int x = 0; x < subdivisionsX; ++x) {
            // Calculate the four corners of the current quad
            int topLeft = z * verticesX + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * verticesX + x;
            int bottomRight = bottomLeft + 1;

            // First triangle (CCW when viewed from above)
            // topLeft -> bottomLeft -> topRight
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle (CCW when viewed from above)
            // topRight -> bottomLeft -> bottomRight
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Return mesh with no textures (textures will be bound via material)
    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

Mesh PrimitiveGenerator::CreateCube(float size)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfSize = size * 0.5f;

    // Define the 6 faces of a cube with proper normals, texture coordinates, and tangents
    // Each face consists of 4 vertices in counter-clockwise order when viewed from outside

    // Front face (+Z)
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize,  halfSize),  // Position
        glm::vec3(0.0f, 0.0f, 1.0f),                // Normal
        glm::vec2(0.0f, 0.0f),                      // TexCoords
        glm::vec3(1.0f, 0.0f, 0.0f),                // Tangent
        glm::vec3(0.0f, 1.0f, 0.0f),                // Bitangent
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f} // Bone data
    });
    vertices.push_back({
        glm::vec3( halfSize, -halfSize,  halfSize),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize,  halfSize),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize,  halfSize),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Back face (-Z)
    vertices.push_back({
        glm::vec3( halfSize, -halfSize, -halfSize),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize, -halfSize),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize, -halfSize),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Left face (-X)
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize,  halfSize),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize,  halfSize),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize, -halfSize),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Right face (+X)
    vertices.push_back({
        glm::vec3( halfSize, -halfSize,  halfSize),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize, -halfSize, -halfSize),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize, -halfSize),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize,  halfSize),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Bottom face (-Y)
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize, -halfSize, -halfSize),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize, -halfSize,  halfSize),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize, -halfSize,  halfSize),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Top face (+Y)
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize,  halfSize),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize,  halfSize),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3( halfSize,  halfSize, -halfSize),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });
    vertices.push_back({
        glm::vec3(-halfSize,  halfSize, -halfSize),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Generate indices for 6 faces, 2 triangles per face
    // Each face starts at index: faceIndex * 4
    for (int face = 0; face < 6; ++face) {
        int baseIndex = face * 4;

        // First triangle (0, 1, 2)
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);

        // Second triangle (2, 3, 0)
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
        indices.push_back(baseIndex + 0);
    }

    // Return mesh with no textures (textures will be bound via material)
    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

Mesh PrimitiveGenerator::CreateFullscreenQuad()
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Create a quad that covers the entire screen in NDC space (-1 to 1)
    // Vertices in NDC space for direct rendering without MVP matrices

    // Bottom-left
    vertices.push_back({
        glm::vec3(-1.0f, -1.0f, 0.0f),  // Position
        glm::vec3(0.0f, 0.0f, 1.0f),    // Normal (facing forward)
        glm::vec2(0.0f, 0.0f),          // TexCoords (bottom-left)
        glm::vec3(1.0f, 0.0f, 0.0f),    // Tangent
        glm::vec3(0.0f, 1.0f, 0.0f),    // Bitangent
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f} // Bone data
    });

    // Bottom-right
    vertices.push_back({
        glm::vec3(1.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Top-right
    vertices.push_back({
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Top-left
    vertices.push_back({
        glm::vec3(-1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Two triangles forming a quad (counter-clockwise winding)
    indices = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };

    // Return mesh with no textures (textures will be bound manually)
    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

Mesh PrimitiveGenerator::CreateDirectionalRay(float length)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Start point (light position)
    vertices.push_back({
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.5f, 0.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // End point (direction along negative Z axis)
    vertices.push_back({
        glm::vec3(0.0f, 0.0f, -length),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.5f, 0.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}
    });

    // Single line from start to end point
    indices.push_back(0);
    indices.push_back(1);

    // Return mesh with single line (no textures needed for wireframe)
    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

Mesh PrimitiveGenerator::CreateWireframeCube(float size)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfSize = size * 0.5f;

    // Define 8 cube vertices
    std::vector<glm::vec3> positions = {
        // Bottom face vertices (y = -halfSize)
        glm::vec3(-halfSize, -halfSize, -halfSize),  // 0: bottom-back-left
        glm::vec3( halfSize, -halfSize, -halfSize),  // 1: bottom-back-right
        glm::vec3( halfSize, -halfSize,  halfSize),  // 2: bottom-front-right
        glm::vec3(-halfSize, -halfSize,  halfSize),  // 3: bottom-front-left

        // Top face vertices (y = halfSize)
        glm::vec3(-halfSize,  halfSize, -halfSize),  // 4: top-back-left
        glm::vec3( halfSize,  halfSize, -halfSize),  // 5: top-back-right
        glm::vec3( halfSize,  halfSize,  halfSize),  // 6: top-front-right
        glm::vec3(-halfSize,  halfSize,  halfSize)   // 7: top-front-left
    };

    // Create vertices
    for (const auto& pos : positions) {
        Vertex vertex;
        vertex.Position = pos;
        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);     // Dummy normal for wireframe
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);        // Dummy tex coords for wireframe
        vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);    // Dummy tangent for wireframe
        vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);  // Dummy bitangent for wireframe

        // Initialize bone data to zero
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            vertex.m_BoneIDs[i] = 0;
            vertex.m_Weights[i] = 0.0f;
        }

        vertices.push_back(vertex);
    }

    // Define the 12 edges of the cube using line indices
    // Each pair of indices represents one edge (line segment)
    std::vector<unsigned int> edges = {
        // Bottom face edges
        0, 1,  // back edge
        1, 2,  // right edge
        2, 3,  // front edge
        3, 0,  // left edge

        // Top face edges
        4, 5,  // back edge
        5, 6,  // right edge
        6, 7,  // front edge
        7, 4,  // left edge

        // Vertical edges connecting bottom to top
        0, 4,  // back-left vertical
        1, 5,  // back-right vertical
        2, 6,  // front-right vertical
        3, 7   // front-left vertical
    };

    indices = edges;

    // Return wireframe mesh (no textures needed)
    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

