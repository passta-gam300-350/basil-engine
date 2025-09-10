#include "native/mesh.h"

namespace Resource {
	MeshResource load_mesh(std::string const& mesh_name) {
		std::ifstream ifs(mesh_name, std::ios::binary);
		assert(ifs && "file error!");

		MeshResource tmp{};

		std::uint64_t pos_ct{};
		std::uint64_t idx_ct{};
		std::uint64_t uv_ct{};
		std::uint64_t norm_ct{};
		std::uint64_t tang_ct{};
		std::uint64_t bitang_ct{};

		ifs.read(reinterpret_cast<char*>(&tmp.m_guid), sizeof(tmp.m_guid));
		ifs.read(reinterpret_cast<char*>(&pos_ct), sizeof(tmp.m_positions.size()));
		ifs.read(reinterpret_cast<char*>(&idx_ct), sizeof(tmp.m_indices.size()));
		ifs.read(reinterpret_cast<char*>(&uv_ct), sizeof(tmp.m_attributes.m_uvs.size()));
		ifs.read(reinterpret_cast<char*>(&norm_ct), sizeof(tmp.m_attributes.m_norms.size()));
		ifs.read(reinterpret_cast<char*>(&tang_ct), sizeof(tmp.m_attributes.m_uvs.size()));
		ifs.read(reinterpret_cast<char*>(&bitang_ct), sizeof(tmp.m_attributes.m_bitangents.size()));

		tmp.m_positions.resize(pos_ct);
		tmp.m_indices.resize(idx_ct);
		tmp.m_attributes.m_uvs.resize(uv_ct);
		tmp.m_attributes.m_norms.resize(norm_ct);
		tmp.m_attributes.m_tangs.resize(tang_ct);
		tmp.m_attributes.m_bitangents.resize(bitang_ct);

		ifs.read(reinterpret_cast<char*>(&tmp.m_positions.data()), pos_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(&tmp.m_indices.data()), idx_ct * sizeof(std::uint32_t));
		ifs.read(reinterpret_cast<char*>(&tmp.m_attributes.m_uvs.data()), uv_ct * sizeof(glm::vec2));
		ifs.read(reinterpret_cast<char*>(&tmp.m_attributes.m_norms.data()), norm_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(&tmp.m_attributes.m_tangs.data()), tang_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(&tmp.m_attributes.m_bitangents.data()), bitang_ct * sizeof(glm::vec3));
	}
}