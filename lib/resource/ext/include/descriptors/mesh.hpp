/******************************************************************************/
/*!
\file   mesh.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Mesh descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RESOURCE_DESCRIPTOR_MESH
#define RESOURCE_DESCRIPTOR_MESH

#include <rsc-ext/descriptor.hpp>
#include "serialization/serializer.h"

struct ModelDescriptor {
	rp::descriptor_base base;
	bool merge_mesh{ true };
	bool extract_material{ true }; //extracts material in pbr. if false, materials are not created but material slots are still allocated
	bool extract_animation{ true }; //extracts material in pbr. if false, materials are not created but material slots are still allocated
	bool extract_textures{ true }; //extracts material in pbr. if false, materials are not created but material slots are still allocated
	bool is_skinned{ false };
	//bool merge_material{ false }; //merge all material slots into 1 material slot with material atlas (texture baking) //TODO: m3 maybe
	glm::vec3 scale{1.f};
	glm::vec3 rotate{}; //in degrees, euler rotation
	glm::vec3 translate{};
	//bool generate_lod{ false }; //do not generate lod if disabled //TODO: m3 maybe
	//float lod_reduction;
	//unsigned int lod_count;
};

#endif