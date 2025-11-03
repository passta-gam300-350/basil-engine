#ifndef ENGINE_TRANSFORM_HPP
#define ENGINE_TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent {
	// Core transform fields
	// NOTE: These represent LOCAL space if entity has a parent, WORLD space if no parent
	// This follows Unity's convention where Transform values are relative to parent
	glm::vec3 m_Scale{1.0f, 1.0f, 1.0f};
	glm::vec3 m_Rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
	glm::vec3 m_Translation{0.0f, 0.0f, 0.0f};

	// Dirty flag for hierarchy updates
	bool isDirty{true};

	/**
	 * @brief Get the local transformation matrix from m_Translation, m_Rotation, m_Scale
	 * @return Local space transformation matrix (TRS)
	 */
	glm::mat4 getLocalMatrix() const {
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_Translation);
		glm::mat4 rotation = glm::mat4_cast(glm::quat(glm::radians(m_Rotation)));
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		return translation * rotation * scale;
	}
};

//DO NOT SERIALISE THIS (maybe reflect for inspection)
struct TransformMtxComponent {
	glm::mat4 m_Mtx;
};

#endif