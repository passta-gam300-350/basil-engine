#include "BVH/bvh_helper.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cfloat>

Frustum CameraToFrustum(CameraSystem::Camera const& camera)
{
    glm::mat4 projection = glm::perspective
    (
        glm::radians(camera.m_Fov),
        camera.m_AspectRatio,
        camera.m_Near,
        camera.m_Far
    );
    glm::mat4 view = glm::lookAt
    (
        camera.m_Pos,
        camera.m_Pos + camera.m_Front,
        camera.m_Up
    );
    return Frustum(projection * view);
}

Ray CameraToRay(CameraSystem::Camera const& camera)
{
    // Create a ray from camera position in the forward direction
    return Ray(camera.m_Pos, glm::normalize(camera.m_Front));
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
