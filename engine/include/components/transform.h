/******************************************************************************/
/*!
\file   transform.h
\author Team PASSTA
		Steven Bangxin Chew (bangxinsteven.chew\@digipen.edu)
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the TransformComponent struct, which
represents the transformation (position, rotation, scale) of an entity in the ECS.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


struct PositionComponent {
	glm::vec3 m_WorldPos;
};

struct ScaleComponent {
	glm::vec3 m_Scale;
};

struct RotationComponent {
	glm::vec3 m_Rotation; // Euler angles in degrees
};


#endif