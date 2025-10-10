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




struct TransformComponent {
	glm::mat4 m_trans = glm::mat4(1.0f); // Kept for backward compatibility

	// Local space (relative to parent)
	glm::vec3 localPosition = glm::vec3(0.0f);
	glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
	glm::vec3 localScale = glm::vec3(1.0f);

	// World space (absolute)
	glm::mat4 worldMatrix = glm::mat4(1.0f);

	// Dirty flag - true when local transform changed and world needs update
	bool isDirty = true;

	// Helper to get local transform matrix
	glm::mat4 getLocalMatrix() const {
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), localPosition);
		glm::mat4 rotation = glm::mat4_cast(localRotation);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), localScale);
		return translation * rotation * scale;
	}

	// Helper to set from euler angles (in radians)
	void setRotationFromEuler(const glm::vec3& eulerRadians) {
		localRotation = glm::quat(eulerRadians);
		isDirty = true;
	}

	// Helper to get euler angles (in radians)
	glm::vec3 getEulerRotation() const {
		return glm::eulerAngles(localRotation);
	}
};

// Legacy components - kept for backward compatibility
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