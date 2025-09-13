#include "native/mesh.h"
#include "native/model.h"
#include "native/texture.h"
#include "native/material.h"
#include <locale>
#include <codecvt>
#include <tinyddsloader.h>

namespace Resource {
	MeshResource load_native_mesh(std::string const& mesh_name) {
		std::ifstream ifs(mesh_name, std::ios::binary);
		assert(ifs && "file error!");

		MeshResource tmp{};

		std::uint64_t pos_ct{};
		std::uint64_t idx_ct{};
		std::uint64_t uv_ct{};
		std::uint64_t color_ct{};
		std::uint64_t norm_ct{};
		std::uint64_t tang_ct{};
		std::uint64_t bitang_ct{};
		std::uint64_t bone_index_ct{};
		std::uint64_t bone_influence_ct{};

		ifs.read(reinterpret_cast<char*>(&tmp.m_guid), sizeof(tmp.m_guid));
		ifs.read(reinterpret_cast<char*>(&pos_ct), sizeof(tmp.m_positions.size()));
		ifs.read(reinterpret_cast<char*>(&idx_ct), sizeof(tmp.m_indices.size()));
		ifs.read(reinterpret_cast<char*>(&uv_ct), sizeof(tmp.m_attributes.m_uvs.size()));
		ifs.read(reinterpret_cast<char*>(&color_ct), sizeof(tmp.m_attributes.m_color.size()));
		ifs.read(reinterpret_cast<char*>(&norm_ct), sizeof(tmp.m_attributes.m_norms.size()));
		ifs.read(reinterpret_cast<char*>(&tang_ct), sizeof(tmp.m_attributes.m_uvs.size()));
		ifs.read(reinterpret_cast<char*>(&bitang_ct), sizeof(tmp.m_attributes.m_bitangents.size()));
		ifs.read(reinterpret_cast<char*>(&bone_index_ct), sizeof(tmp.m_attributes.m_bone_indices.size()));
		ifs.read(reinterpret_cast<char*>(&bone_influence_ct), sizeof(tmp.m_attributes.m_bone_influence.size()));
		ifs.read(reinterpret_cast<char*>(&tmp.m_properties.m_mat_idx), sizeof(tmp.m_properties.m_mat_idx));

		tmp.m_positions.resize(pos_ct);
		tmp.m_indices.resize(idx_ct);
		tmp.m_attributes.m_uvs.resize(uv_ct);
		tmp.m_attributes.m_color.resize(color_ct);
		tmp.m_attributes.m_norms.resize(norm_ct);
		tmp.m_attributes.m_tangs.resize(tang_ct);
		tmp.m_attributes.m_bitangents.resize(bitang_ct);
		tmp.m_attributes.m_bone_indices.resize(bone_index_ct);
		tmp.m_attributes.m_bone_influence.resize(bone_influence_ct);

		ifs.read(reinterpret_cast<char*>(tmp.m_positions.data()), pos_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(tmp.m_indices.data()), idx_ct * sizeof(std::uint32_t));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_uvs.data()), uv_ct * sizeof(glm::vec2));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_color.data()), color_ct * sizeof(glm::vec4));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_norms.data()), norm_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_tangs.data()), tang_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_bitangents.data()), bitang_ct * sizeof(glm::vec3));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_bone_indices.data()), bone_index_ct * sizeof(glm::vec4));
		ifs.read(reinterpret_cast<char*>(tmp.m_attributes.m_bone_influence.data()), bone_influence_ct * sizeof(glm::vec4));

		return tmp;
	}

	MaterialResource load_native_material(std::string const& material_name) {
		MaterialResource tmp;
		std::ifstream ifs(material_name, std::ios::binary);
		assert(ifs && "file error!");

		/*
		ifs.read(reinterpret_cast<char*>(&tmp.m_guid), sizeof(tmp.m_guid));
		ifs.read(reinterpret_cast<char*>(&tmp.m_color), sizeof(tmp.m_color));
		ifs.read(reinterpret_cast<char*>(&tmp.m_specular_color), sizeof(tmp.m_specular_color));
		ifs.read(reinterpret_cast<char*>(&tmp.m_opacity), sizeof(tmp.m_opacity));
		ifs.read(reinterpret_cast<char*>(&tmp.m_metallic), sizeof(tmp.m_metallic));
		ifs.read(reinterpret_cast<char*>(&tmp.m_shininess), sizeof(tmp.m_shininess));
		ifs.read(reinterpret_cast<char*>(&tmp.m_roughness), sizeof(tmp.m_roughness));
		ifs.read(reinterpret_cast<char*>(&tmp.m_color_tex), sizeof(tmp.m_color_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_diffuse_tex), sizeof(tmp.m_diffuse_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_normal_tex), sizeof(tmp.m_normal_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_height_tex), sizeof(tmp.m_height_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_roughness_tex), sizeof(tmp.m_roughness_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_occlusion_tex), sizeof(tmp.m_occlusion_tex));
		ifs.read(reinterpret_cast<char*>(&tmp.m_emissive_tex), sizeof(tmp.m_emissive_tex));
		*/

		//short but potentially dangerous
		ifs.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));

		return tmp;
	}

	TextureResource load_dds_texture(std::string const& file_name) {
		tinyddsloader::DDSFile ddsfile;
		ddsfile.Load(file_name.c_str());
		assert(tinyddsloader::Result::Success && "fail to load dds for " && file_name.c_str());
		return ddsfile;
	}
}