#include "Resources/PrimitiveGenerator.h"
#include <vector>
#include <algorithm>

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