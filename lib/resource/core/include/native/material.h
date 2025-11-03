#ifndef LIB_RESOURCE_CORE_NATIVE_MATERIAL_H
#define LIB_RESOURCE_CORE_NATIVE_MATERIAL_H

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <rsc-core/guid.hpp>
#include "serialization/native_serializer.h"

struct MaterialResourceData {
	std::string vert_name;
	std::string frag_name;
	std::string material_name;

	glm::vec3 albedo = glm::vec3(0.8f, 0.7f, 0.6f);
	float metallic = 0.7f;
	float roughness = 0.3f;

	std::unordered_map<std::string, float> float_properties;
	std::unordered_map<std::string, glm::vec3> vec3_properties;
	std::unordered_map<std::string, glm::vec4> vec4_properties;
	std::unordered_map<std::string, rp::Guid> texture_properties;
};

#endif