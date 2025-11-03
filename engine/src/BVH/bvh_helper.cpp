#include "BVH/bvh_helper.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cfloat>

Frustum CameraToFrustum(CameraSystem::Camera const& camera)
{
    Frustum frustum;

    // extract camera data
    glm::vec3 pos = camera.m_Pos;
    glm::vec3 front = camera.m_Front;
    glm::vec3 right = camera.m_Right;
    glm::vec3 up = camera.m_Up;

    float nearDist = camera.m_Near;
    float farDist = camera.m_Far;
    float fov = camera.m_Fov;          // in degrees
    float aspect = camera.m_AspectRatio;

    // calculate frustum dimensions
    float tanHalfFov = tan(glm::radians(fov / 2.0f));
    float nearHeight = 2.0f * tanHalfFov * nearDist;
    float nearWidth = nearHeight * aspect;
    float farHeight = 2.0f * tanHalfFov * farDist;
    float farWidth = farHeight * aspect;

    // centers of near and far planes
    glm::vec3 nearCenter = pos + front * nearDist;
    glm::vec3 farCenter = pos + front * farDist;

    // === construct 6 planes (normals point INWARD) ===

    // near plane (points forward, towards far)
    frustum.planes[0] = Plane(front, glm::dot(front, nearCenter));

    // far plane (points backward, towards near)
    frustum.planes[1] = Plane(-front, glm::dot(-front, farCenter));

    // left plane
    glm::vec3 leftNormal = glm::normalize(glm::cross(up, front * farDist - right * (farWidth / 2.0f)));
    frustum.planes[2] = Plane(leftNormal, glm::dot(leftNormal, pos));

    // rright plane
    glm::vec3 rightNormal = glm::normalize(glm::cross(front * farDist + right * (farWidth / 2.0f), up));
    frustum.planes[3] = Plane(rightNormal, glm::dot(rightNormal, pos));

    // top plane
    glm::vec3 topNormal = glm::normalize(glm::cross(right, front * farDist + up * (farHeight / 2.0f)));
    frustum.planes[4] = Plane(topNormal, glm::dot(topNormal, pos));

    // bottom plane
    glm::vec3 bottomNormal = glm::normalize(glm::cross(front * farDist - up * (farHeight / 2.0f), right));
    frustum.planes[5] = Plane(bottomNormal, glm::dot(bottomNormal, pos));

    return frustum;
}

Aabb TransformAABB(Aabb const& localAABB, glm::mat4 const& matrix)
{
    glm::vec3 AabbCorners[8] =
    {
        // bottom four points
        {localAABB.min.x, localAABB.min.y, localAABB.min.z},
        {localAABB.min.x, localAABB.min.y, localAABB.max.z},
        {localAABB.max.x, localAABB.min.y, localAABB.max.z},
        {localAABB.max.x, localAABB.min.y, localAABB.min.z},
        // top four points
        {localAABB.min.x, localAABB.max.y, localAABB.min.z},
        {localAABB.min.x, localAABB.max.y, localAABB.max.z},
        {localAABB.max.x, localAABB.max.y, localAABB.max.z},
        {localAABB.max.x, localAABB.max.y, localAABB.min.z}
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

glm::mat4 BuildTransformMatrix(glm::vec3 const& position, glm::vec3 const& rotation, glm::vec3 const& scale)
{
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)); 
    transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)); 
    transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)); 
    transform = glm::scale(transform, scale);
    return transform;
}
