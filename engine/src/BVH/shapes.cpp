/******************************************************************************/
/*!
\file   shapes.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/01
\brief  Implementation of geometric primitive operations for BVH

Implements constructors and methods for geometric shapes used in BVH spatial calculations:
AABB construction, expansion, intersection tests, containment checks, plane distance calculations,
frustum-AABB tests, and surface area computations.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "BVH/shapes.h"
#include "BVH/bvh_math.h"
#include <random>
#include <algorithm>

namespace
{
    constexpr float cEpsilon = 1e-1f;
}


/**
   * @brief
   *  This is a Segment operator[] function to
   *  let user access one of the point in the struct
   *  Segment
   * @param int i
   *  The index number, which is the point wanna be accessed,
   *  start from 0 to 1, 0 for first point, 1 for second point
   * @return
   *  The point according to the index
   */
glm::vec3& Segment::operator[](int i)
{
    assert(i >= 0 && i < 2);
    return i == 0 ? startPoint : endPoint;
}

/**
* @brief
*  This is a const Segment operator[] function to
*  let user access one of the point in the struct
*  Segment
* @param int i
*  The index number, which is the point wanna be accessed,
*  start from 0 to 1, 0 for first point, 1 for second point
* @return
*  The point according to the index
*/
const glm::vec3& Segment::operator[](int i) const
{
    assert(i >= 0 && i < 2);
    return i == 0 ? startPoint : endPoint;
}

/**
 * @brief
 *  This is a constructor for Segment
 *  that take in two point and will be assigned
 *  to the member variable
 * @param glm::vec3 const& point1
 *  First point
 * @param glm::vec3 const& point2
 *  Second point
 * @return
 */
Segment::Segment(glm::vec3 const& point1, glm::vec3 const& point2) : startPoint(point1), endPoint(point2)
{

}

/**
* @brief
*  This is a triangle operator[] function to
*  let user access one of the point in the struct
*  Triangle
* @param int i
*  The index number, which is the point wanna be accessed,
*  start from 0 to 2, 0 for first point, 1 for second point
*  2 for third point
* @return
*  The point according to the index
*/
glm::vec3& Triangle::operator[](int i)
{
    assert(i >= 0 && i < 3);
    return points[i];
}

/**
* @brief
*  This is a const triangle operator[] function to
*  let user access one of the point in the struct
*  Triangle
* @param int i
*  The index number, which is the point wanna be accessed,
*  start from 0 to 2, 0 for first point, 1 for second point
*  2 for third point
* @return
*  The point according to the index
*/
const glm::vec3& Triangle::operator[](int i) const
{
    assert(i >= 0 && i < 3);
    return points[i];
}

/**
 * @brief
 *  This is a Triangle constructor that take in 3 points
 *  and will be assigned to the member variable
 * @param glm::vec3 const& point1
 *  Variable to construct point 1
 * @param glm::vec3 const& point2
 *  Variable to construct point 2
 * @param glm::vec3 const& point3
 *  Variable to construct point 3
 * @return
 *  void
 */
Triangle::Triangle(glm::vec3 const& point1, glm::vec3 const& point2, glm::vec3 const& point3) : points{ point1, point2, point3 }
{
}

/**
 * @brief
 *  This is a constructor for struct Plane
 *  that take in a point and a normal and will
 *  be assigned to the member variable
 * @param glm::vec3 const& cPoint
 *  Variable to construct the constant D by
 *  calculate the dot product of the normal and the point
 * @param glm::vec3 const& cNormal
 *  Variable to construct normal
 * @return
 */
Plane::Plane(glm::vec3 const& cPoint, glm::vec3 const& cNormal) : normal(cNormal), constantD(glm::dot(normal, cPoint))
{
}

/**
 * @brief
 *  This is a constructor for struct Plane
 *  that take in a normal and a constant and
 *  will be assigned to the member variable
 * @param glm::vec3 const& cNormal
 *  Variable to construct normal
 * @param float const& cConstant
 *  Variable to construct constantD
 * @return
 */
Plane::Plane(glm::vec3 const& cNormal, float const& cConstant) : normal(cNormal), constantD(cConstant)

{
}

/**
 * @brief
 *  This is a helper function that return
 *  point on a plane
 * @param
 *  void
 * @return
 *  the point on a plane
 */
glm::vec3 Plane::get_point() const
{
    return normal * constantD;
}

/**
 * @brief
 *  This function will classify if an Aabb is
 *  outside, intersecting or inside the plane
 * @param Aabb const& AABB
 *  The Aabb to be checked
 * @return
 *  The classified result
 */
SideResult Plane::classify(Aabb const& AABB) const
{
    glm::vec3 centerOfAABB = (AABB.min + AABB.max) / 2.0f;
    float distanceFromPlane = glm::dot(normal, centerOfAABB) - constantD;
    float boxWidth = AABB.max.x - AABB.min.x;
    float boxHeight = AABB.max.y - AABB.min.y;
    float boxDepth = AABB.max.z - AABB.min.z; // need remember z axis
    float radius = std::abs((boxWidth / 2.0f * normal.x)) + std::abs((boxHeight / 2.0f * normal.y)) + std::abs((boxDepth / 2.0f * normal.z));
    return (distanceFromPlane > radius + cEpsilon) ? SideResult::eOUTSIDE : (distanceFromPlane < -radius - cEpsilon) ? SideResult::eINSIDE : SideResult::eINTERSECTING;
}

/**
 * @brief
 *  This is a default constructor
 *  of struct Aabb
 * @param
 *  void
 * @return
 *  void
 */
Aabb::Aabb() : min(vec3(0.0f)), max(vec3(0.0f))
{
}

/**
 * @brief
 *  This is an Aabb constructor that
 *  take in a min and max and it will be
 *  assigned to the member variable
 * @param glm::vec3 const& cMin
 *  the min of Aabb to construct min
 * @param glm::vec3 const& cMax
 *  the max of Aabb to construct max
 * @return
 */
Aabb::Aabb(glm::vec3 const& cMin, glm::vec3 const& cMax) : min(cMin), max(cMax)
{
}

/**
 * @brief
 *  This is an Aabb constructor that
 *  take in a pointer to points and the count
 *  of the point and use it to construct an
 *  Aabb
 * @param glm::vec3 const* points
 *  the pointer to the points
 * @param size_t const& count
 *  the count of points
 * @return
 */
Aabb::Aabb(glm::vec3 const* points, size_t const& count) : min(FLT_MAX, FLT_MAX, FLT_MAX), max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
{
    for (size_t i = 0; i < count; i++)
    {
        min = glm::min(min, points[i]);
        max = glm::max(max, points[i]);
    }
}

/**
 * @brief
 *  This is an Aabb constructor that
 *  take in a pointer to points and the count
 *  of the point and a transformation matrix
 *  to construct the Aabb and transform it
 * @param glm::vec3 const* points
 *  the pointer to the points
 * @param size_t const& count
 *  the count of points
 * @param mat4 const& matrixTransform
 *  the transform matrix
 * @return
 */
Aabb::Aabb(glm::vec3 const* points, size_t const& count, mat4 const& matrixTransform) : min(FLT_MAX, FLT_MAX, FLT_MAX), max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
{
    for (size_t i = 0; i < count; i++)
    {
        glm::vec3 transformedPoints = glm::vec3(matrixTransform * glm::vec4(points[i], 1.0f));
        min = glm::min(min, transformedPoints);
        max = glm::max(max, transformedPoints);
    }
}

/**
 * @brief
 *  This is a helper function that
 *  return the center of Aabb
 * @param
 *  void
 * @return
 *  void
 */
glm::vec3 Aabb::get_center() const
{
    return (min + max) / 2.0f;
}

/**
 * @brief
 *  This is a helper function that
 *  return the extent of Aabb
 * @param
 *  void
 * @return
 *  void
 */
glm::vec3 Aabb::get_extents() const
{
    return max - min;
}

/**
 * @brief
 *  This is a helper function that
 *  return the longest axis of Aabb
 * @param
 *  void
 * @return int
 *  longest axis, 0 for x, 1 for y, 2 for z
 */
int Aabb::get_longest_axis() const
{
    glm::vec3 extent = get_extents();
    int axis = 0;
    if (extent.y > extent.x)
    {
        axis = 1;
    }
    if (extent.z > extent[axis])
    {
        axis = 2;
    }
    return axis;
}

/**
 * @brief
 *  This is a helper function that
 *  return the volume of aabb
 * @param
 *  void
 * @return float
 *  the volume of aabb
 */
float Aabb::volume() const
{
    float width = max.x - min.x;
    float height = max.y - min.y;
    float depth = max.z - min.z;
    return width * height * depth;
}

/**
 * @brief
 *  This is a helper function that
 *  return the surface area of aabb
 * @param
 *  void
 * @return float
 *  surface area of aabb
 */
float Aabb::surface_area() const
{
    float width = max.x - min.x;
    float height = max.y - min.y;
    float depth = max.z - min.z;
    return 2.0f * ((width * height) + (width * depth) + (height * depth));
}
/**
 * @brief
 *  This is an Aabb helper function
 *  that take in a transformation matrix
 *  to transform the Aabb
 * @param mat4 const& matrix
 *  the transform matrix
 * @return
 *  the transformed Aabb
 */
Aabb Aabb::transform(mat4 const& matrix)
{
    glm::vec3 AabbCorners[8] =
    {
        // bottom four points
        {min.x, min.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, min.y, min.z},
        // top four points
        {min.x, max.y, min.z},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z},
        {max.x, max.y, min.z}
    };
    glm::vec3 newAabbMin = { FLT_MAX, FLT_MAX, FLT_MAX };
    glm::vec3 newAabbMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 transformedCorner = glm::vec3(matrix * glm::vec4(AabbCorners[i], 1.0f));
        newAabbMin = glm::min(newAabbMin, transformedCorner);
        newAabbMax = glm::max(newAabbMax, transformedCorner);
    }
    Aabb transformedAabb = { newAabbMin, newAabbMax };
    return transformedAabb;
}

/**
 * @brief
 *  This is an Aabb helper function
 *  that take in two Aabb and merge
 *  them together to a new Aabb
 * @param Aabb const& lhs
 *  first Aabb
 * @param Aabb const& rhs
 *  second Aabb
 * @return
 *  the merged Aabb
 */
Aabb Aabb::Merge(Aabb const& rhs)
{
    Aabb merged;
    merged.min.x = std::min(min.x, rhs.min.x);
    merged.min.y = std::min(min.y, rhs.min.y);
    merged.min.z = std::min(min.z, rhs.min.z);
    merged.max.x = std::max(max.x, rhs.max.x);
    merged.max.y = std::max(max.y, rhs.max.y);
    merged.max.z = std::max(max.z, rhs.max.z);
    return merged;
}

/**
 * @brief
 *  This is an frustum constructor that take in a
 *  view projection matrix and use it to construct a
 *  Frustum
 * @param mat4 const& vpMatrix
 *  the view projection matrix
 * @return
 *  void
 */
Frustum::Frustum(mat4 const& vpMatrix) : normals{}, dists{}
{
    // column major matrix
    glm::vec4 row1 = { vpMatrix[0][0], vpMatrix[1][0], vpMatrix[2][0], vpMatrix[3][0] };
    glm::vec4 row2 = { vpMatrix[0][1], vpMatrix[1][1], vpMatrix[2][1], vpMatrix[3][1] };
    glm::vec4 row3 = { vpMatrix[0][2], vpMatrix[1][2], vpMatrix[2][2], vpMatrix[3][2] };
    glm::vec4 row4 = { vpMatrix[0][3], vpMatrix[1][3], vpMatrix[2][3], vpMatrix[3][3] };
    glm::vec4 eachPlaneExtracted[6];
    // left clipping plane
    eachPlaneExtracted[0] = row4 + row1;
    // right clipping plane
    eachPlaneExtracted[1] = row4 - row1;
    // top clipping plane
    eachPlaneExtracted[2] = row4 - row2;
    // bottom clipping plane 
    eachPlaneExtracted[3] = row4 + row2;
    // near clipping plane
    eachPlaneExtracted[4] = row4 + row3;
    // far clipping plane
    eachPlaneExtracted[5] = row4 - row3;

    for (int i = 0; i < 6; i++)
    {
        glm::vec3 normal = { eachPlaneExtracted[i].x, eachPlaneExtracted[i].y, eachPlaneExtracted[i].z };
        float lengthNormal = glm::length(normal);
        normals[i] = -normal / lengthNormal; // negate it to be outward normal
        dists[i] = eachPlaneExtracted[i].w / lengthNormal;
    }
}

/**
 * @brief
 *  This function will classify if an Aabb is inside the
 *  frustum
 * @param Aabb const& AABB
 *  The aabb to be checked
 * @return
 *  The classified result
 */
SideResult Frustum::classify(Aabb const& AABB) const
{
    int insideCount = 0;
    int intersectCount = 0;
    int numberOfPlane = 6;
    for (int i = 0; i < numberOfPlane; i++)
    {
        // if the aabb is outside any plane, then we can safely say it is not within the Frustum. 
        Plane planeCheck(normals[i], dists[i]);
        SideResult result = planeCheck.classify(AABB);
        if (result == SideResult::eOUTSIDE)
        {
            return SideResult::eOUTSIDE;
        }
        result == SideResult::eINSIDE ? ++insideCount : ++intersectCount;
    }
    if (intersectCount > 0)
    {
        return SideResult::eINTERSECTING;
    }
    return SideResult::eINSIDE;
}

/**
 * @brief
 *  This is a default constructor for struct Ray
 * @return
 */
Ray::Ray() : start(glm::vec3(0, 0, 0)), dir(glm::vec3(1, 0, 0))
{

}
/**
 * @brief
 *  This is a constructor for struct Ray that will
 *  take in a point and a dir vector that will
 *  be assigned to the member variable
 * @param const glm::vec3& cstart
 *  The variable that use to construct the start
 * @param const glm::vec3& cdir
 *  The variable that use to construct the dir
 * @return
 */
Ray::Ray(glm::vec3 const& cstart, glm::vec3 const& cdir) : start(cstart), dir(cdir)
{

}

/**
 * @brief
 *  This function will find the intersection time
 *  of a ray and Aabb
 * @param const Aabb& AABB
 *  The Aabb to be checked
 * @return
 *  the intersection time
 */
float Ray::intersect(Aabb const& AABB) const
{
    // if ray in the AABB
    if (start.x >= AABB.min.x && start.x <= AABB.max.x &&
        start.y >= AABB.min.y && start.y <= AABB.max.y &&
        start.z >= AABB.min.z && start.z <= AABB.max.z)
    {
        return 0.0f;
    }

    // if the dir coordinate is close to 0, that means that it is parallel to the axis
    // it must be within the min and max of AABB, if not is not intersecting
    // everything else is same for other two axis
    if (std::abs(dir.x) <= cEpsilon)
    {
        if (start.x < AABB.min.x || start.x > AABB.max.x)
        {
            return -1.0f;
        }
    }

    if (std::abs(dir.y) <= cEpsilon)
    {
        if (start.y < AABB.min.y || start.y > AABB.max.y)
        {
            return -1.0f;
        }
    }

    if (std::abs(dir.z) <= cEpsilon)
    {
        if (start.z < AABB.min.z || start.z > AABB.max.z)
        {
            return -1.0f;
        }
    }

    float txMin = (AABB.min.x - start.x) / dir.x;
    float txMax = (AABB.max.x - start.x) / dir.x;
    // negative dir, always ensure min is the minimum, same for other axis
    if (txMin > txMax)
    {
        std::swap(txMin, txMax);
    }
    float tyMin = (AABB.min.y - start.y) / dir.y;
    float tyMax = (AABB.max.y - start.y) / dir.y;
    if (tyMin > tyMax)
    {
        std::swap(tyMin, tyMax);
    }
    float tzMin = (AABB.min.z - start.z) / dir.z;
    float tzMax = (AABB.max.z - start.z) / dir.z;
    if (tzMin > tzMax)
    {
        std::swap(tzMin, tzMax);
    }

    float tMin = glm::max(txMin, tyMin, tzMin);
    float tMax = glm::min(txMax, tyMax, tzMax);

    // means there's no overlap, the ray missed the aabb if tmin > tmax
    // tMax < 0.0f, means box is behind the ray origin
    if (tMin > tMax || tMax < 0.0f)
    {
        return -1.0f;
    }
    return tMin;
}

/**
 * @brief
 *  this function returns the position along the ray at parameter t
 * @param float t
 *  the t value along the ray
 * @return
 *  the position at parameter t along the ray
 */
glm::vec3 Ray::at(float const t) const
{
    return start + t * dir;
}

/**
 * @brief
 *  This is a Sphere constructor that take in
 *  a center point and a radius and will
 *  be assigned to the member variable
 * @param glm::vec3 const& cCenter
 *  Variable to construct center point
 * @param float const& cRadius
 *  Variable to construct radius
 * @return
 *  void
 */
Sphere::Sphere(glm::vec3 const& cCenter, float const& cRadius) : center(cCenter), radius(cRadius)
{

}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points and the count of points
 *  and to construct a bounding
 *  sphere from centorid
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @return
 *  the bouding sphere that constructed from centroid
 */
Sphere Sphere::centroid(glm::vec3 const* points, size_t const& count)
{
    // 1st pass: Compute the centroid: Average the points, find the point that is more central.
    glm::vec3 centroid = {};
    for (size_t i = 0; i < count; i++)
    {
        centroid += points[i];
    }
    centroid /= float(count);
    // 2nd pass: Find most extreme point as radius
    float radius = 0;
    for (size_t i = 0; i < count; i++)
    {
        float squaredDistance = glm::length2(points[i] - centroid);
        if (squaredDistance > radius * radius)
        {
            radius = glm::sqrt(squaredDistance); // try to save some sqrt operation here
        }
    }
    return Sphere(centroid, radius);
}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points, the count of points
 *  and a transform matrix to construct a bounding
 *  sphere from centorid and transform it using the matrix
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @param mat4 const& matrixTransform
 *  the transformation martix
 * @return
 *  the bouding sphere that constructed from centroid and transformed
 */
Sphere Sphere::centroid(glm::vec3 const* points, size_t const& count, mat4 const& matrixTransform)
{
    // transform all point first then compute using the function defined above
    std::vector<vec3> transformedPoints(count);
    for (size_t i = 0; i < count; i++)
    {
        transformedPoints[i] = glm::vec3(matrixTransform * glm::vec4(points[i], 1.0f));
    }
    return centroid(transformedPoints.data(), count);
}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points and the count of points
 *  and to construct a bounding
 *  sphere using Ritters's method
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @return
 *  the bouding sphere that constructed using
 *  Ritters's method
 */
Sphere Sphere::ritters(vec3 const* points, size_t const& count)
{
    // record where is all the min and max point of each axis to find the most separate points
    size_t xMinidx = 0;
    size_t xMaxidx = 0;
    size_t yMinidx = 0;
    size_t yMaxidx = 0;
    size_t zMinidx = 0;
    size_t zMaxidx = 0;

    // The first step to track the most separate points in X, Y and Z. ( find the x y z min max )
    for (size_t i = 0; i < count; i++)
    {
        if (points[i].x < points[xMinidx].x)
        {
            xMinidx = i;
        }
        if (points[i].x > points[xMaxidx].x)
        {
            xMaxidx = i;
        }
        if (points[i].y < points[yMinidx].y)
        {
            yMinidx = i;
        }
        if (points[i].y > points[yMaxidx].y)
        {
            yMaxidx = i;
        }
        if (points[i].z < points[zMinidx].z)
        {
            zMinidx = i;
        }
        if (points[i].z > points[zMaxidx].z)
        {
            zMaxidx = i;
        }
    }

    // From these 3 pairs of two points, pick the pair that is more spread out.
    size_t mostSpreadmin = 0;
    size_t mostSpreadMax = 0;
    glm::vec3 xAxis = points[xMaxidx] - points[xMinidx];
    glm::vec3 yAxis = points[yMaxidx] - points[yMinidx];
    glm::vec3 zAxis = points[zMaxidx] - points[zMinidx];

    // perform less square roots
    float squaredDistanceX = (xAxis.x * xAxis.x) + (xAxis.y * xAxis.y) + (xAxis.z * xAxis.z);
    float squaredDistanceY = (yAxis.x * yAxis.x) + (yAxis.y * yAxis.y) + (yAxis.z * yAxis.z);
    float squaredDistanceZ = (zAxis.x * zAxis.x) + (zAxis.y * zAxis.y) + (zAxis.z * zAxis.z);
    if (squaredDistanceX >= squaredDistanceY && squaredDistanceX >= squaredDistanceZ)
    {
        mostSpreadmin = xMinidx;
        mostSpreadMax = xMaxidx;
    }
    else if (squaredDistanceY >= squaredDistanceX && squaredDistanceY >= squaredDistanceZ)
    {
        mostSpreadmin = yMinidx;
        mostSpreadMax = yMaxidx;
    }
    else
    {
        mostSpreadmin = zMinidx;
        mostSpreadMax = zMaxidx;
    }

    // Create initial sphere by averaging of these two points.
    glm::vec3 initialCentre = (points[mostSpreadmin] + points[mostSpreadMax]) / 2.0f;
    float initialRadius = glm::distance(points[mostSpreadmin], points[mostSpreadMax]) / 2.0f;

    // Iterate the rest of the points, for each other vertex that is outside, expand sphere in the proper dir.
    for (size_t i = 0; i < count; i++)
    {
        float squaredDistanceCompare = glm::length2(points[i] - initialCentre); // save some sqrt
        if (squaredDistanceCompare > initialRadius * initialRadius)
        {
            float distanceCompare = std::sqrt(squaredDistanceCompare);
            glm::vec3 b = initialCentre - (initialRadius * (points[i] - initialCentre) / distanceCompare);
            initialCentre = (points[i] + b) / 2.0f;
            initialRadius = glm::length(points[i] - b) / 2.0f;
        }
    }
    return Sphere(initialCentre, initialRadius);
}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points, the count of points
 *  and a transform matrix to construct a bounding
 *  sphere using Ritters's method and transform
 *  it using the matrix
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @param mat4 const& matrixTransform
 *  the transformation martix
 * @return
 *  the bouding sphere that constructed using
 *  Riters's method and transformed
 */
Sphere Sphere::ritters(vec3 const* points, size_t const& count, mat4 const& matrixTransform)
{
    // transform all point first then compute using the function defined above
    std::vector<vec3> transformedPoints(count);
    for (size_t i = 0; i < count; i++)
    {
        transformedPoints[i] = glm::vec3(matrixTransform * glm::vec4(points[i], 1.0f));
    }
    return ritters(transformedPoints.data(), count);
}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points, the count of points,
 *  iteration count, and shrink ratio to construct a bounding
 *  sphere using iterative improvement method
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @param int const& iterations
 *  iteration count
 * @param float const& shrinkRatio
 *  shrink ratio
 * @return
 *  the bouding sphere that constructed using
 *  iterative improvement method
 */
Sphere Sphere::iterative(vec3 const* points, size_t const& count, int const& iterations, float const& shrinkRatio)
{
    // Start with the result of a previous Ritter sphere
    Sphere initialRitterSphere = ritters(points, count);
    std::vector<size_t> randomPointIndex(count);
    // sub in all the index
    for (size_t i = 0; i < count; i++)
    {
        randomPointIndex[i] = i;
    }
    // generate random seed for random shuffle the points index
    std::random_device rd;
    unsigned int seed = rd();
    std::mt19937 randomGenerator(seed);
    // Every iteration shrunk the sphere a little in order to force a better expansion
    for (int i = 0; i < iterations; i++)
    {
        Sphere newIterativeSphere = initialRitterSphere;
        newIterativeSphere.radius *= shrinkRatio; // shrink little bit
        // Iterate through points in random order and grow the sphere(if a point is outside), similar how it grows in ritter
        std::shuffle(randomPointIndex.begin(), randomPointIndex.end(), randomGenerator);
        for (size_t j = 0; j < randomPointIndex.size(); j++)
        {
            float squaredDistanceCompare = glm::length2(points[randomPointIndex[j]] - newIterativeSphere.center); // save some sqrt
            if (squaredDistanceCompare > newIterativeSphere.radius * newIterativeSphere.radius)
            {
                float distanceCompare = std::sqrt(squaredDistanceCompare);
                glm::vec3 b = newIterativeSphere.center - (newIterativeSphere.radius * (points[randomPointIndex[j]] - newIterativeSphere.center) / distanceCompare);
                newIterativeSphere.center = (points[randomPointIndex[j]] + b) / 2.0f;
                newIterativeSphere.radius = glm::length(points[randomPointIndex[j]] - b) / 2.0f;
            }
        }
        // If the sphere that we got is better than the original Ritter sphere, we got a better sphere
        // Keep track of this and continue to the next iteration. 
        // Otherwise, ignore the current sphere as we created a worse sphere.
        if (newIterativeSphere.radius < initialRitterSphere.radius)
        {
            initialRitterSphere = newIterativeSphere;
        }
    }
    return initialRitterSphere; // final better bounding sphere 
}

/**
 * @brief
 *  This is a function that takes in
 *  the pointer to points, the count of points,
 *  iteration count, shrink ratio and a
 *  transform matrix to construct a bounding
 *  sphere using iterative improvement method and transform
 *  it using the matrix
 * @param glm::vec3 const* points
 *  pointer to points
 * @param size_t const& count
 *  count of points
 * @param int const& iterations
 *  iteration count
 * @param float const& shrinkRatio
 *  shrink ratio
 * @param mat4 const& matrixTransform
 *  the transformation martix
 * @return
 *  the bouding sphere that constructed using
 *  iterative improvement method and transformed
 */
Sphere Sphere::iterative(vec3 const* points, size_t const& count, int const& iterations, float const& shrinkRatio, mat4 const& matrixTransform)
{
    // transform all point first then compute using the function defined above
    std::vector<vec3> transformedPoints(count);
    for (size_t i = 0; i < count; i++)
    {
        transformedPoints[i] = glm::vec3(matrixTransform * glm::vec4(points[i], 1.0f));
    }
    return iterative(transformedPoints.data(), count, iterations, shrinkRatio);
}