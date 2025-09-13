#include "native/mesh.h"
#include "native/material.h"
#include "native/model.h"

namespace Resource {
	MeshResource& MeshResource::operator>>(std::ofstream& outp) {
		assert(outp && "file error!");

		std::uint64_t pos_ct{ m_positions.size() };
		std::uint64_t idx_ct{ m_indices.size() };
		std::uint64_t uv_ct{ m_attributes.m_uvs.size() };
		std::uint64_t color_ct{ m_attributes.m_color.size() };
		std::uint64_t norm_ct{ m_attributes.m_norms.size() };
		std::uint64_t tang_ct{ m_attributes.m_tangs.size() };
		std::uint64_t bitang_ct{ m_attributes.m_bitangents.size() };
		std::uint64_t bone_index_ct{ m_attributes.m_bone_indices.size() };
		std::uint64_t bone_influence_ct{ m_attributes.m_bone_influence.size() };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&pos_ct), sizeof(m_positions.size()));
		outp.write(reinterpret_cast<const char*>(&idx_ct), sizeof(m_indices.size()));
		outp.write(reinterpret_cast<const char*>(&uv_ct), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&color_ct), sizeof(m_attributes.m_color.size()));
		outp.write(reinterpret_cast<const char*>(&norm_ct), sizeof(m_attributes.m_norms.size()));
		outp.write(reinterpret_cast<const char*>(&tang_ct), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&bitang_ct), sizeof(m_attributes.m_bitangents.size()));
		outp.write(reinterpret_cast<const char*>(&bone_index_ct), sizeof(m_attributes.m_bone_indices.size()));
		outp.write(reinterpret_cast<const char*>(&bone_influence_ct), sizeof(m_attributes.m_bone_influence.size()));
		outp.write(reinterpret_cast<const char*>(&m_properties.m_mat_idx), sizeof(m_properties.m_mat_idx));

		outp.write(reinterpret_cast<const char*>(m_positions.data()), m_positions.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_indices.data()), m_indices.size() * sizeof(std::uint32_t));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_uvs.data()), m_attributes.m_uvs.size() * sizeof(glm::vec2));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_color.data()), m_attributes.m_color.size() * sizeof(glm::vec4));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_norms.data()), m_attributes.m_norms.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_tangs.data()), m_attributes.m_tangs.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bitangents.data()), m_attributes.m_bitangents.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bone_indices.data()), m_attributes.m_bone_indices.size() * sizeof(std::uint32_t));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bone_influence.data()), m_attributes.m_bone_influence.size() * sizeof(float));

		return *this;
	}

	MeshResource const& MeshResource::operator>>(std::ofstream& outp) const {
		assert(outp && "file error!");

		std::uint64_t pos_ct{ m_positions.size() };
		std::uint64_t idx_ct{ m_indices.size() };
		std::uint64_t uv_ct{ m_attributes.m_uvs.size() };
		std::uint64_t color_ct{ m_attributes.m_color.size() };
		std::uint64_t norm_ct{ m_attributes.m_norms.size() };
		std::uint64_t tang_ct{ m_attributes.m_tangs.size() };
		std::uint64_t bitang_ct{ m_attributes.m_bitangents.size() };
		std::uint64_t bone_index_ct{ m_attributes.m_bone_indices.size() };
		std::uint64_t bone_influence_ct{ m_attributes.m_bone_influence.size() };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&pos_ct), sizeof(m_positions.size()));
		outp.write(reinterpret_cast<const char*>(&idx_ct), sizeof(m_indices.size()));
		outp.write(reinterpret_cast<const char*>(&uv_ct), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&color_ct), sizeof(m_attributes.m_color.size()));
		outp.write(reinterpret_cast<const char*>(&norm_ct), sizeof(m_attributes.m_norms.size()));
		outp.write(reinterpret_cast<const char*>(&tang_ct), sizeof(m_attributes.m_uvs.size()));
		outp.write(reinterpret_cast<const char*>(&bitang_ct), sizeof(m_attributes.m_bitangents.size()));
		outp.write(reinterpret_cast<const char*>(&bone_index_ct), sizeof(m_attributes.m_bone_indices.size()));
		outp.write(reinterpret_cast<const char*>(&bone_influence_ct), sizeof(m_attributes.m_bone_influence.size()));
		outp.write(reinterpret_cast<const char*>(&m_properties.m_mat_idx), sizeof(m_properties.m_mat_idx));

		outp.write(reinterpret_cast<const char*>(m_positions.data()), m_positions.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_indices.data()), m_indices.size() * sizeof(std::uint32_t));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_uvs.data()), m_attributes.m_uvs.size() * sizeof(glm::vec2));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_color.data()), m_attributes.m_color.size() * sizeof(glm::vec4));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_norms.data()), m_attributes.m_norms.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_tangs.data()), m_attributes.m_tangs.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bitangents.data()), m_attributes.m_bitangents.size() * sizeof(glm::vec3));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bone_indices.data()), m_attributes.m_bone_indices.size() * sizeof(std::uint32_t));
		outp.write(reinterpret_cast<const char*>(m_attributes.m_bone_influence.data()), m_attributes.m_bone_influence.size() * sizeof(float));

		return *this;
	}

	MaterialResource& MaterialResource::operator>>(std::ofstream& outp) {
		/*
		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&m_color), sizeof(m_color));
		outp.write(reinterpret_cast<const char*>(&m_specular_color), sizeof(m_specular_color));
		outp.write(reinterpret_cast<const char*>(&m_opacity), sizeof(m_opacity));
		outp.write(reinterpret_cast<const char*>(&m_metallic), sizeof(m_metallic));
		outp.write(reinterpret_cast<const char*>(&m_shininess), sizeof(m_shininess));
		outp.write(reinterpret_cast<const char*>(&m_roughness), sizeof(m_roughness));
		outp.write(reinterpret_cast<const char*>(&m_color_tex), sizeof(m_color_tex));
		outp.write(reinterpret_cast<const char*>(&m_diffuse_tex), sizeof(m_diffuse_tex));
		outp.write(reinterpret_cast<const char*>(&m_normal_tex), sizeof(m_normal_tex));
		outp.write(reinterpret_cast<const char*>(&m_height_tex), sizeof(m_height_tex));
		outp.write(reinterpret_cast<const char*>(&m_roughness_tex), sizeof(m_roughness_tex));
		outp.write(reinterpret_cast<const char*>(&m_occlusion_tex), sizeof(m_occlusion_tex));
		outp.write(reinterpret_cast<const char*>(&m_emissive_tex), sizeof(m_emissive_tex));
		*/

		//fast serialisation
		outp.write(reinterpret_cast<const char*>(this), sizeof(*this));

		return *this;
	}

	MaterialResource const& MaterialResource::operator>>(std::ofstream& outp) const {
		/*
		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&m_color), sizeof(m_color));
		outp.write(reinterpret_cast<const char*>(&m_specular_color), sizeof(m_specular_color));
		outp.write(reinterpret_cast<const char*>(&m_opacity), sizeof(m_opacity));
		outp.write(reinterpret_cast<const char*>(&m_metallic), sizeof(m_metallic));
		outp.write(reinterpret_cast<const char*>(&m_shininess), sizeof(m_shininess));
		outp.write(reinterpret_cast<const char*>(&m_roughness), sizeof(m_roughness));
		outp.write(reinterpret_cast<const char*>(&m_color_tex), sizeof(m_color_tex));
		outp.write(reinterpret_cast<const char*>(&m_diffuse_tex), sizeof(m_diffuse_tex));
		outp.write(reinterpret_cast<const char*>(&m_normal_tex), sizeof(m_normal_tex));
		outp.write(reinterpret_cast<const char*>(&m_height_tex), sizeof(m_height_tex));
		outp.write(reinterpret_cast<const char*>(&m_roughness_tex), sizeof(m_roughness_tex));
		outp.write(reinterpret_cast<const char*>(&m_occlusion_tex), sizeof(m_occlusion_tex));
		outp.write(reinterpret_cast<const char*>(&m_emissive_tex), sizeof(m_emissive_tex));
		*/

		//fast serialisation
		outp.write(reinterpret_cast<const char*>(this), sizeof(*this));

		return *this;
	}

	ModelResource const& ModelResource::operator>>(std::ofstream& outp) const {
		std::uint32_t mesh_ct{ static_cast<std::uint32_t>(m_meshes.size()) };
		std::uint32_t material_ct{ static_cast<std::uint32_t>(m_mats.size()) };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&mesh_ct), sizeof(mesh_ct));
		outp.write(reinterpret_cast<const char*>(&material_ct), sizeof(material_ct));

		for (auto const& mesres : m_meshes) {
			outp.write(reinterpret_cast<const char*>(&mesres.m_guid), sizeof(mesres.m_guid));
		}

		for (auto const& matres : m_mats) {
			outp.write(reinterpret_cast<const char*>(&matres.m_guid), sizeof(matres.m_guid));
		}
		return *this;
	}

	ModelResource& ModelResource::operator>>(std::ofstream& outp) {
		std::uint32_t mesh_ct{ static_cast<std::uint32_t>(m_meshes.size()) };
		std::uint32_t material_ct{ static_cast<std::uint32_t>(m_mats.size()) };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&mesh_ct), sizeof(mesh_ct));
		outp.write(reinterpret_cast<const char*>(&material_ct), sizeof(material_ct));

		for (auto const& mesres : m_meshes) {
			outp.write(reinterpret_cast<const char*>(&mesres.m_guid), sizeof(mesres.m_guid));
		}

		for (auto const& matres : m_mats) {
			outp.write(reinterpret_cast<const char*>(&matres.m_guid), sizeof(matres.m_guid));
		}
		return *this;
	}
}