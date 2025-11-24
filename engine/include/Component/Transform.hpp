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
	// decompose matrix to scale, rotate(degrees) and translate components. put nullptr if u dont want the components
	void Decompose(glm::vec3* world_scale, glm::vec3* world_rotate, glm::vec3* world_translate) const {
		glm::vec3 w_scale{ glm::length(glm::vec3(m_Mtx[0])), glm::length(glm::vec3(m_Mtx[1])), glm::length(glm::vec3(m_Mtx[2])) };
		world_scale ? *world_scale = w_scale : w_scale;
		if (world_rotate) {
			glm::mat3 rotMat {
				glm::vec3(m_Mtx[0]) / w_scale.x,
				glm::vec3(m_Mtx[1]) / w_scale.y,
				glm::vec3(m_Mtx[2]) / w_scale.z
			};
			*world_rotate = glm::degrees(glm::eulerAngles(glm::quat(rotMat)));
		}

		if (world_translate) {
			*world_translate = glm::vec3(m_Mtx[3]);
		}
	}
};

#endif