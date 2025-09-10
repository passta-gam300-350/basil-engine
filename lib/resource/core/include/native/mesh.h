#ifndef LIB_RESOURCE_CORE_NATIVE_MESH_H
#define LIB_RESOURCE_CORE_NATIVE_MESH_H

#include <vector>
#include <fstream>
#include <string>
#include <glm/glm.hpp>

namespace Resource {

	template <typename t>
	using MeshVertexBuffer = std::vector<t>;

	struct MeshResource {
		std::uint32_t m_guid;
		MeshVertexBuffer<glm::vec3> m_positions;
		MeshVertexBuffer<std::uint32_t> m_indices;
		struct MeshVertexAttributes {
			MeshVertexBuffer<glm::vec2> m_uvs;
			MeshVertexBuffer<glm::vec3> m_norms;
			MeshVertexBuffer<glm::vec3> m_tangs;
			MeshVertexBuffer<glm::vec3> m_bitangents;
		} m_attributes;

		MeshResource& operator>>(std::ofstream& outp);
	};

	//guid not supported
	MeshResource load_mesh(std::uint32_t guid);
	MeshResource load_mesh(std::string const& mesh_name);

}

#endif