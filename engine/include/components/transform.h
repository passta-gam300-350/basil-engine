#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include <glm/glm.hpp>

struct TransformComponent {
	glm::mat4 m_trans;
};

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