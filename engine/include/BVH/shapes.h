#pragma once
/**
 * @file
 *  shapes.hpp
 * @author
 *  Jia Zen Cheong, 670004724, jiazen.c@digipen.edu
 * @date
 *  2025/05/24
 * @brief
 *  This is the header file that implemented the declaration of various geometry struct,
    and their functions and constructor needed
 * @copyright
 *  Copyright (C) 2025 DigiPen Institute of Technology.
 */
#ifndef __SHAPES_H__
#define __SHAPES_H__

#include "BVH/bvh_math.h"
#include <vector>
#include <array>


enum SideResult
{
    eINSIDE = -1,
    eINTERSECTING = 0,
    eOUTSIDE = 1
};


struct Line
{
    glm::vec3 start;
    glm::vec3 dir;
};

struct Segment
{
    glm::vec3 startPoint;
    glm::vec3 endPoint;
    glm::vec3& operator[](int i);
    const glm::vec3& operator[](int i) const;
    Segment(glm::vec3 const& point1, glm::vec3 const& point2);
};

struct Triangle
{
    std::array<glm::vec3, 3> points;
    glm::vec3& operator[](int i);
    const glm::vec3& operator[](int i) const;
    Triangle(glm::vec3 const& point1, glm::vec3 const& point2, glm::vec3 const& point3);
};

struct Aabb
{
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 get_center() const;
    glm::vec3 get_extents() const;
    int get_longest_axis() const;
    float volume() const;
    float surface_area() const;
    Aabb();
    Aabb(glm::vec3 const& cMin, glm::vec3 const& cMax);
    Aabb(glm::vec3 const* points, size_t const& count);
    Aabb(glm::vec3 const* points, size_t const& count, mat4 const& matrixTransform);
    Aabb transform(mat4 const& matrix);
    Aabb Merge(Aabb const& rhs);
};

// helper plane
struct Plane
{
    glm::vec3 normal;
    float constantD;
    Plane(glm::vec3 const& cPoint, glm::vec3 const& cNormal);
    Plane(glm::vec3 const& cNormal, float const& cConstant);
    glm::vec3 get_point() const;
    SideResult classify(Aabb const& AABB) const;
};

struct Frustum
{
    std::array<glm::vec3, 6> normals;
    std::array<float, 6> dists;
    Frustum(mat4 const& vpMatrix);
    SideResult classify(Aabb const& AABB) const;
};

struct Ray
{
    glm::vec3 start;
    glm::vec3 dir;
    Ray();
    Ray(glm::vec3 const& cOriginPoint, glm::vec3 const& cDirection);
    float intersect(Aabb const& AABB) const;
    glm::vec3 at(float const t) const;
};

struct Sphere
{
    glm::vec3 center;
    float radius;
    Sphere(glm::vec3 const& cCenter, float const& cRadius);
    static Sphere centroid(glm::vec3 const* points, size_t const& count);
    static Sphere centroid(glm::vec3 const* points, size_t const& count, mat4 const& matrixTransform);
    static Sphere ritters(vec3 const* points, size_t const& count);
    static Sphere ritters(vec3 const* points, size_t const& count, mat4 const& matrixTransform);
    static Sphere iterative(vec3 const* points, size_t const& count, int const& iterations, float const& shrinkRatio);
    static Sphere iterative(vec3 const* points, size_t const& count, int const& iterations, float const& shrinkRatio, mat4 const& matrixTransform);
};

#endif // __SHAPES_H__
