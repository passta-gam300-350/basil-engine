#pragma once
#ifndef __BVH_HELPER_H__
#define __BVH_HELPER_H__
#include "BVH/shapes.h"
#include "Render/Camera.h"
#include <glm/glm.hpp>

Frustum CameraToFrustum(CameraSystem::Camera const& camera);
Aabb TransformAABB(Aabb const& localAABB, glm::mat4 const& matrix);
glm::mat4 BuildTransformMatrix(glm::vec3 const& position, glm::vec3 const& rotation, glm::vec3 const& scale);
#endif
