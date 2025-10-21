#ifndef ENGINE_TRANSFORM_HPP
#define ENGINE_TRANSFORM_HPP

#include <glm/glm.hpp>

struct TransformComponent {
	glm::vec3 m_Scale;
	glm::vec3 m_Rotation;
	glm::vec3 m_Translation;
};

//DO NOT SERIALISE THIS (maybe reflect for inspection)
struct TransformMtxComponent {
	glm::mat4 m_Mtx;
};

#endif