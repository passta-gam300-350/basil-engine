#ifndef RESOURCE_DESCRIPTOR_MESH
#define RESOURCE_DESCRIPTOR_MESH

#include <rsc-ext/descriptor.hpp>

struct ModelDescriptor {
	rp::descriptor_base base;
	bool merge_mesh;
	std::string mesh_asset_source;
	std::vector<rp::Guid> meshes;
};

#endif