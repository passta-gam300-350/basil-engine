#ifndef RESOURCE_DESCRIPTOR_MATERIAL
#define RESOURCE_DESCRIPTOR_MATERIAL

#include <rsc-ext/descriptor.hpp>
#include <native/material.h>

struct MaterialDescriptor {
	rp::descriptor_base base;

	MaterialResourceData material;
};

inline MaterialResourceData CreateMaterial(MaterialDescriptor const& matDesc, std::string const& path = {}) {
	SerializeBinary(matDesc.material, matDesc.base.m_guid, ".material", path);
	return matDesc.material;
}

// Register material descriptor importer
// .mtl extension is for Wavefront material files (imported from .obj models)
// Manually created materials have no source file (descriptor IS the source)
RegisterResourceTypeImporter(MaterialDescriptor, MaterialResourceData, "material", ".material", CreateMaterial, ".mtl")

#endif