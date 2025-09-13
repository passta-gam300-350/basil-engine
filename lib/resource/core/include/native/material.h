#ifndef LIB_RESOURCE_CORE_NATIVE_MATERIAL_H
#define LIB_RESOURCE_CORE_NATIVE_MATERIAL_H

#include <string>
#include <glm/glm.hpp>
#include "serialisation/guid.h"

namespace Resource {
	struct MaterialResource {
		Guid m_guid;
		glm::vec3 m_color;
		glm::vec3 m_specular_color;
		float m_opacity;
		float m_metallic;
		float m_shininess;
		float m_roughness;
		Guid m_color_tex = null_guid;
		Guid m_diffuse_tex = null_guid;
		Guid m_normal_tex = null_guid;
		Guid m_height_tex = null_guid;
		Guid m_metallic_tex = null_guid;
		Guid m_roughness_tex = null_guid;
		Guid m_occlusion_tex = null_guid;
		Guid m_emissive_tex = null_guid;

		MaterialResource& operator>>(std::ofstream& outp);
		MaterialResource const& operator>>(std::ofstream& outp) const;
	};

	MaterialResource load_native_material(std::uint32_t guid);
	MaterialResource load_native_material(std::string const& mesh_name);
}

#endif