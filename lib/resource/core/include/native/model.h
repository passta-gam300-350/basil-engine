#ifndef LIB_RESOURCE_CORE_NATIVE_MODEL_H
#define LIB_RESOURCE_CORE_NATIVE_MODEL_H

#include "native/mesh.h"
#include "native/material.h"
#include "native/skeletal.h"
#include "native/animation.h"

namespace Resource {
	struct ModelResource {
		Guid m_guid;
		std::vector<Guid> m_meshes;
		std::vector<Guid> m_mats;
		//SkeletalResource m_skel;

		ModelResource& operator>>(std::ofstream& outp);
		ModelResource const& operator>>(std::ofstream& outp) const;
	};

	ModelResource load_native_model(std::uint32_t guid);
	ModelResource load_native_model(std::string const& mesh_name);
}

#endif