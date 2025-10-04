/******************************************************************************/
/*!
\file   AABB.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares axis-aligned bounding box structures and utilities for culling and debugging

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

// Forward declarations
class Mesh;

/**
 * Axis-Aligned Bounding Box utility class
 *
 * Provides AABB calculation, transformation, and rendering utilities
 * for debugging and culling operations.
 */
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    // Default constructor - creates invalid AABB
    AABB() : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {}

    // Constructor with min/max points
    AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint)
        : min(minPoint), max(maxPoint) {}

    // Check if AABB is valid (min <= max for all components)
    bool IsValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Get center point of AABB
    glm::vec3 GetCenter() const
    {
        return (min + max) * 0.5f;
    }

    // Get size (extent) of AABB
    glm::vec3 GetSize() const
    {
        return max - min;
    }

    // Get the 8 corner points of the AABB
    std::vector<glm::vec3> GetCorners() const;

    // Transform AABB by a transformation matrix
    AABB Transform(const glm::mat4& transformMatrix) const;

    // Expand AABB to include a point
    void ExpandToInclude(const glm::vec3& point);

    // Expand AABB to include another AABB
    void ExpandToInclude(const AABB& other);

    // Reset AABB to invalid state
    void Reset();

    // Static factory methods
    static AABB CreateFromMesh(const std::shared_ptr<Mesh>& mesh);
    static AABB CreateFromPoints(const std::vector<glm::vec3>& points);
};

/**
 * Debug AABB data for rendering
 * Contains AABB and associated rendering properties
 */
struct DebugAABB
{
    AABB boundingBox;
    glm::vec3 color;
    bool visible;
    bool isLocalAABB;  // Flag to distinguish local vs world AABBs
    glm::mat4 objectTransform;  // Transform matrix for local AABBs

    // Constructor for world AABB
    DebugAABB(const AABB& aabb, const glm::vec3& debugColor = glm::vec3(1.0f, 1.0f, 0.0f))
        : boundingBox(aabb), color(debugColor), visible(true), isLocalAABB(false), objectTransform(glm::mat4(1.0f)) {}

    // Constructor for local AABB
    DebugAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec3& debugColor = glm::vec3(0.0f, 1.0f, 1.0f))
        : boundingBox(aabb), color(debugColor), visible(true), isLocalAABB(true), objectTransform(transform) {}
};