#ifndef RESOURCE_DESCRIPTOR_TEXTURE
#define RESOURCE_DESCRIPTOR_TEXTURE

#include <rsc-ext/descriptor.hpp>

struct MaterialDescriptor {
	rp::descriptor_base base;

	std::string vert_name;
	std::string frag_name;

	std::unordered_map<std::string, float> FloatProperties;
	std::unordered_map<std::string, glm::vec3> Vec3Properties;
	std::unordered_map<std::string, glm::vec4> Vec4Properties;
	std::unordered_map<std::string, rp::Guid> TextureProperties;
};

#endif