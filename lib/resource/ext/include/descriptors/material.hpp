#ifndef RESOURCE_DESCRIPTOR_MATERIAL
#define RESOURCE_DESCRIPTOR_MATERIAL

#include <rsc-ext/descriptor.hpp>
#include <native/material.h>

struct MaterialDescriptor {
	rp::descriptor_base base;

	MaterialResourceData material;
};

inline void CreateMaterial(MaterialDescriptor const& matDesc) {
	SerializeBinary(matDesc.material, matDesc.base.m_guid, ".material");
}

#endif