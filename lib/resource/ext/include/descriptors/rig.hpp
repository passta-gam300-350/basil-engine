#ifndef RESOURCE_DESCRIPTOR_RIG
#define RESOURCE_DESCRIPTOR_RIG

#include <rsc-ext/descriptor.hpp>
#include <native/rig.h>

struct RigDescriptor {
	rp::descriptor_base base;

	RigResourceData rig;
};

/*
inline RigResourceData CreateRig(RigDescriptor const& rigDesc, std::string const& path = {}, std::string const& serialisedescpath = {}) {
	SerializeBinary(rigDesc.rig, rigDesc.base.m_guid, ".skel", path);
	if (!serialisedescpath.empty())
		rp::serialization::yaml_serializer::serialize(rigDesc, serialisedescpath);
	return rigDesc.rig;
}
RegisterResourceTypeImporter(RigDescriptor, RigResourceData, "skeleton", ".skel", CreateRig, "")
*/

#endif