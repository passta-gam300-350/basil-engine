#include "Utility/AABB.h"
#include "Resources/Mesh.h"
#include <cassert>
#include <algorithm>

std::vector<glm::vec3> AABB::GetCorners() const
{
    assert(IsValid() && "AABB must be valid to get corners");

    std::vector<glm::vec3> corners(8);

    // Generate all 8 combinations of min/max for x, y, z
    corners[0] = glm::vec3(min.x, min.y, min.z);  // min corner
    corners[1] = glm::vec3(max.x, min.y, min.z);
    corners[2] = glm::vec3(min.x, max.y, min.z);
    corners[3] = glm::vec3(max.x, max.y, min.z);
    corners[4] = glm::vec3(min.x, min.y, max.z);
    corners[5] = glm::vec3(max.x, min.y, max.z);
    corners[6] = glm::vec3(min.x, max.y, max.z);
    corners[7] = glm::vec3(max.x, max.y, max.z);  // max corner

    return corners;
}

AABB AABB::Transform(const glm::mat4& transformMatrix) const
{
    assert(IsValid() && "AABB must be valid to transform");

    // Get all 8 corners of the original AABB
    auto corners = GetCorners();

    // Transform all corners and find new min/max
    AABB transformedAABB;
    for (const auto& corner : corners)
    {
        glm::vec4 transformedCorner = transformMatrix * glm::vec4(corner, 1.0f);
        glm::vec3 worldCorner = glm::vec3(transformedCorner);
        transformedAABB.ExpandToInclude(worldCorner);
    }

    return transformedAABB;
}

void AABB::ExpandToInclude(const glm::vec3& point)
{
    min = glm::min(min, point);
    max = glm::max(max, point);
}

void AABB::ExpandToInclude(const AABB& other)
{
    assert(other.IsValid() && "Other AABB must be valid to expand");

    if (!IsValid())
    {
        // If this AABB is invalid, just copy the other
        *this = other;
        return;
    }

    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

void AABB::Reset()
{
    min = glm::vec3(FLT_MAX);
    max = glm::vec3(-FLT_MAX);
}

AABB AABB::CreateFromMesh(const std::shared_ptr<Mesh>& mesh)
{
    assert(mesh && "Mesh cannot be null for AABB creation");

    // Get vertex data from mesh
    const auto& vertices = mesh->vertices;
    assert(!vertices.empty() && "Mesh must have vertices for AABB creation");

    // Extract positions from Vertex structs
    std::vector<glm::vec3> positions;
    positions.reserve(vertices.size());
    for (const auto& vertex : vertices)
    {
        positions.push_back(vertex.Position);
    }

    return CreateFromPoints(positions);
}

AABB AABB::CreateFromPoints(const std::vector<glm::vec3>& points)
{
    assert(!points.empty() && "Points vector cannot be empty for AABB creation");

    AABB aabb;

    for (const auto& point : points)
    {
        aabb.ExpandToInclude(point);
    }

    assert(aabb.IsValid() && "Created AABB must be valid");
    return aabb;
}