#ifndef LIB_RESOURCE_CORE_NATIVE_MESH_H
#define LIB_RESOURCE_CORE_NATIVE_MESH_H

#include <vector>
#include <fstream>
#include <string>
#include "serialisation/guid.h"
#include <glm/glm.hpp>

namespace Resource {

	template <typename t>
	using MeshVertexBuffer = std::vector<t>;

	struct MeshResource {
		Guid m_guid;
		MeshVertexBuffer<glm::vec3> m_positions;
		MeshVertexBuffer<std::uint32_t> m_indices;
		struct MeshVertexAttributes {
			MeshVertexBuffer<glm::vec2> m_uvs;
			MeshVertexBuffer<glm::vec4> m_color;
			MeshVertexBuffer<glm::vec3> m_norms;
			MeshVertexBuffer<glm::vec3> m_tangs;
			MeshVertexBuffer<glm::vec3> m_bitangents;
			MeshVertexBuffer<std::uint32_t> m_bone_indices;
			MeshVertexBuffer<float> m_bone_influence;
		} m_attributes;
		struct MeshProperties {
			std::uint32_t m_mat_idx;
		} m_properties;

		MeshResource& operator>>(std::ofstream& outp);
		MeshResource const& operator>>(std::ofstream& outp) const;
	};

	//guid not supported
	MeshResource load_native_mesh(std::uint32_t guid);
	MeshResource load_native_mesh(std::string const& mesh_name);

}

#endif