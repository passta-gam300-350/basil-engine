#include "native/mesh.h"

namespace Resource {
	MeshResource& MeshResource::operator>>(std::ofstream& outp) {
		assert(outp && "file error!");
		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&m_positions.size()), sizeof(m_positions.size()));
		outp.write(reinterpret_cast<const char*>(&m_indices.size()), sizeof(m_indices.size()));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_uvs.size()), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_norms.size()), sizeof(m_attributes.m_norms.size()));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_tangs.size()), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_bitangents.size()), sizeof(m_attributes.m_bitangents.size()));

		outp.write(reinterpret_cast<const char*>(&m_positions.data()), m_positions.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(&m_indices.data()), m_indices.size() * sizeof(std::uint32_t));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_uvs.data()), m_attributes.m_uvs.size() * sizeof(glm::vec2));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_norms.data()), m_attributes.m_norms.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_tangs.data()), m_attributes.m_tangs.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(&m_attributes.m_bitangents.data()), m_attributes.m_bitangents.size() * sizeof(glm::vec3));
	}
}