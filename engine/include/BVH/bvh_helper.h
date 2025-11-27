/******************************************************************************/
/*!
\file   bvh_helper.h
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/01
\brief  Helper utilities for BVH integration with rendering system

Provides utility functions for converting between camera frustums and BVH spatial queries,
transforming AABBs by matrices, and building transformation matrices for culling tests.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#ifndef __BVH_HELPER_H__
#define __BVH_HELPER_H__
#include "BVH/shapes.h"
#include <glm/glm.hpp>

// Convert camera parameters to frustum for BVH culling
Frustum CameraToFrustum(glm::vec3 const& position, glm::vec3 const& front, glm::vec3 const& up,
                        float fov, float aspectRatio, float nearPlane, float farPlane);
Aabb TransformAABB(Aabb const& localAABB, glm::mat4 const& matrix);
glm::mat4 BuildTransformMatrix(glm::vec3 const& position, glm::vec3 const& rotation, glm::vec3 const& scale);
#endif
